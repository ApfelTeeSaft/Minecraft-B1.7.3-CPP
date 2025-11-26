#include "pathfinding.hpp"
#include "world/chunk/chunk_manager.hpp"
#include "world/chunk/chunk.hpp"
#include <algorithm>

namespace mcserver {

Pathfinder::Pathfinder(ChunkManager* chunk_manager)
    : chunk_manager_(chunk_manager) {
}

PathfindingResult Pathfinder::find_path(const PathNode& start, const PathNode& goal,
                                         f64 max_distance, bool can_jump, bool can_swim) {
    PathfindingResult result;
    result.success = false;
    result.nodes_evaluated = 0;

    // Quick distance check
    if (start.distance_to(goal) > max_distance) {
        return result;
    }

    // Check if start and goal are walkable
    if (!is_walkable(start.x, start.y, start.z, can_swim) ||
        !is_walkable(goal.x, goal.y, goal.z, can_swim)) {
        return result;
    }

    // A* algorithm
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
    std::unordered_map<PathNode, PathNode, PathNode::Hash> came_from;
    std::unordered_map<PathNode, f64, PathNode::Hash> g_score;
    std::unordered_map<PathNode, bool, PathNode::Hash> closed_set;

    // Initialize start node
    AStarNode start_node;
    start_node.position = start;
    start_node.g_cost = 0.0;
    start_node.h_cost = heuristic(start, goal);
    start_node.parent = start;

    open_set.push(start_node);
    g_score[start] = 0.0;

    // Maximum nodes to evaluate (prevent infinite loops)
    constexpr i32 MAX_NODES = 1000;

    while (!open_set.empty() && result.nodes_evaluated < MAX_NODES) {
        AStarNode current = open_set.top();
        open_set.pop();

        // Skip if already processed
        if (closed_set[current.position]) {
            continue;
        }

        closed_set[current.position] = true;
        result.nodes_evaluated++;

        // Check if we reached the goal
        if (current.position == goal) {
            result.path = reconstruct_path(came_from, start, goal);
            result.success = true;
            return result;
        }

        // Get neighbors
        auto neighbors = get_neighbors(current.position, can_jump, can_swim);

        for (const auto& neighbor : neighbors) {
            // Skip if already in closed set
            if (closed_set[neighbor]) {
                continue;
            }

            // Calculate tentative g_score
            f64 tentative_g = current.g_cost + current.position.distance_to(neighbor);

            // Skip if this path is worse than a previously found one
            auto g_it = g_score.find(neighbor);
            if (g_it != g_score.end() && tentative_g >= g_it->second) {
                continue;
            }

            // This path is better, record it
            came_from[neighbor] = current.position;
            g_score[neighbor] = tentative_g;

            // Add to open set
            AStarNode neighbor_node;
            neighbor_node.position = neighbor;
            neighbor_node.g_cost = tentative_g;
            neighbor_node.h_cost = heuristic(neighbor, goal);
            neighbor_node.parent = current.position;
            open_set.push(neighbor_node);
        }
    }

    // No path found
    return result;
}

bool Pathfinder::is_walkable(i32 x, i32 y, i32 z, bool can_swim) const {
    // Check bounds
    if (y < 0 || y >= CHUNK_SIZE_Y - 1) {
        return false;
    }

    // Get chunk
    i32 chunk_x = x >> 4;
    i32 chunk_z = z >> 4;
    Chunk* chunk = chunk_manager_->get_chunk_if_loaded(chunk_x, chunk_z);
    if (!chunk) {
        return false;  // Chunk not loaded
    }

    i32 local_x = x & 0xF;
    i32 local_z = z & 0xF;

    // Check blocks at feet and head level
    u8 block_below = chunk->get_block(local_x, y - 1, local_z);
    u8 block_feet = chunk->get_block(local_x, y, local_z);
    u8 block_head = chunk->get_block(local_x, y + 1, local_z);

    // Need solid ground below (unless swimming)
    bool is_liquid_below = (block_below == static_cast<u8>(BlockId::WaterFlowing) ||
                            block_below == static_cast<u8>(BlockId::WaterStill) ||
                            block_below == static_cast<u8>(BlockId::LavaFlowing) ||
                            block_below == static_cast<u8>(BlockId::LavaStill));

    if (!can_swim && is_liquid_below) {
        return false;  // Can't walk on liquid
    }

    if (!is_liquid_below && is_solid(x, y - 1, z) == false) {
        return false;  // No ground below
    }

    // Feet and head level must be passable
    bool feet_passable = (block_feet == static_cast<u8>(BlockId::Air)) ||
                         (can_swim && (block_feet == static_cast<u8>(BlockId::WaterFlowing) ||
                                       block_feet == static_cast<u8>(BlockId::WaterStill)));

    bool head_passable = (block_head == static_cast<u8>(BlockId::Air)) ||
                         (can_swim && (block_head == static_cast<u8>(BlockId::WaterFlowing) ||
                                       block_head == static_cast<u8>(BlockId::WaterStill)));

    return feet_passable && head_passable;
}

bool Pathfinder::is_solid(i32 x, i32 y, i32 z) const {
    if (y < 0 || y >= CHUNK_SIZE_Y) {
        return false;
    }

    i32 chunk_x = x >> 4;
    i32 chunk_z = z >> 4;
    Chunk* chunk = chunk_manager_->get_chunk_if_loaded(chunk_x, chunk_z);
    if (!chunk) {
        return false;
    }

    i32 local_x = x & 0xF;
    i32 local_z = z & 0xF;
    u8 block = chunk->get_block(local_x, y, local_z);

    return block != static_cast<u8>(BlockId::Air) &&
           block != static_cast<u8>(BlockId::WaterFlowing) &&
           block != static_cast<u8>(BlockId::WaterStill) &&
           block != static_cast<u8>(BlockId::LavaFlowing) &&
           block != static_cast<u8>(BlockId::LavaStill);
}

bool Pathfinder::has_ground_below(i32 x, i32 y, i32 z) const {
    return is_solid(x, y - 1, z);
}

f64 Pathfinder::heuristic(const PathNode& from, const PathNode& to) const {
    // Manhattan distance with diagonal cost
    i32 dx = std::abs(from.x - to.x);
    i32 dy = std::abs(from.y - to.y);
    i32 dz = std::abs(from.z - to.z);

    // Diagonal move cost
    i32 straight = std::abs(dx - dz);
    i32 diagonal = std::max(dx, dz) - straight;

    return static_cast<f64>(straight) + diagonal * 1.414 + dy * 1.5;  // Y movement is more costly
}

std::vector<PathNode> Pathfinder::get_neighbors(const PathNode& node, bool can_jump, bool can_swim) const {
    std::vector<PathNode> neighbors;

    // 8 horizontal directions + up/down
    const i32 dx[] = {-1, 0, 1, -1, 1, -1, 0, 1, 0, 0};
    const i32 dy[] = { 0, 0, 0,  0, 0,  0, 0, 0, 1,-1};
    const i32 dz[] = { 0,-1, 0,  1, 1, -1,-1,-1, 0, 0};

    for (usize i = 0; i < 10; ++i) {
        PathNode neighbor;
        neighbor.x = node.x + dx[i];
        neighbor.y = node.y + dy[i];
        neighbor.z = node.z + dz[i];

        // Check if neighbor is walkable
        if (is_walkable(neighbor.x, neighbor.y, neighbor.z, can_swim)) {
            neighbors.push_back(neighbor);
        }

        // Try jumping up 1 block (if moving horizontally and can_jump)
        if (can_jump && dy[i] == 0 && (dx[i] != 0 || dz[i] != 0)) {
            PathNode jump_neighbor;
            jump_neighbor.x = node.x + dx[i];
            jump_neighbor.y = node.y + 1;
            jump_neighbor.z = node.z + dz[i];

            if (is_walkable(jump_neighbor.x, jump_neighbor.y, jump_neighbor.z, can_swim)) {
                neighbors.push_back(jump_neighbor);
            }
        }
    }

    return neighbors;
}

std::vector<PathNode> Pathfinder::reconstruct_path(
    const std::unordered_map<PathNode, PathNode, PathNode::Hash>& came_from,
    const PathNode& start, const PathNode& goal) const {

    std::vector<PathNode> path;
    PathNode current = goal;

    // Build path backwards from goal to start
    while (current != start) {
        path.push_back(current);
        auto it = came_from.find(current);
        if (it == came_from.end()) {
            // Path broken, shouldn't happen
            break;
        }
        current = it->second;
    }

    // Reverse to get path from start to goal
    std::reverse(path.begin(), path.end());

    return path;
}

// PathFollower implementation
PathFollower::PathFollower() = default;

void PathFollower::set_path(const std::vector<PathNode>& path) {
    path_ = path;
    current_waypoint_ = 0;
}

bool PathFollower::get_next_waypoint(f64 current_x, f64 current_y, f64 current_z,
                                      f64& target_x, f64& target_y, f64& target_z) {
    if (is_path_complete()) {
        return false;
    }

    const PathNode& waypoint = path_[current_waypoint_];

    // Check if we're close enough to the current waypoint to move to the next one
    f64 dx = current_x - static_cast<f64>(waypoint.x);
    f64 dy = current_y - static_cast<f64>(waypoint.y);
    f64 dz = current_z - static_cast<f64>(waypoint.z);
    f64 distance_squared = dx * dx + dy * dy + dz * dz;

    // If within 0.5 blocks, move to next waypoint
    if (distance_squared < 0.25) {
        current_waypoint_++;
        if (is_path_complete()) {
            return false;
        }
        return get_next_waypoint(current_x, current_y, current_z, target_x, target_y, target_z);
    }

    // Return current waypoint as target
    target_x = static_cast<f64>(waypoint.x) + 0.5;  // Center of block
    target_y = static_cast<f64>(waypoint.y);
    target_z = static_cast<f64>(waypoint.z) + 0.5;  // Center of block

    return true;
}

void PathFollower::clear_path() {
    path_.clear();
    current_waypoint_ = 0;
}

} // namespace mcserver
