#pragma once

#include "util/types.hpp"
#include <vector>
#include <unordered_map>
#include <queue>
#include <cmath>

namespace mcserver {

class ChunkManager;

// 3D position for pathfinding
struct PathNode {
    i32 x, y, z;

    bool operator==(const PathNode& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const PathNode& other) const {
        return !(*this == other);
    }

    f64 distance_to(const PathNode& other) const {
        f64 dx = static_cast<f64>(x - other.x);
        f64 dy = static_cast<f64>(y - other.y);
        f64 dz = static_cast<f64>(z - other.z);
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    // Hash function for unordered_map
    struct Hash {
        usize operator()(const PathNode& node) const {
            // Simple hash combining x, y, z coordinates
            usize h1 = std::hash<i32>{}(node.x);
            usize h2 = std::hash<i32>{}(node.y);
            usize h3 = std::hash<i32>{}(node.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
};

// Pathfinding result
struct PathfindingResult {
    std::vector<PathNode> path;
    bool success;
    i32 nodes_evaluated;
};

// A* pathfinding for mobs
class Pathfinder {
public:
    explicit Pathfinder(ChunkManager* chunk_manager);

    // Find a path from start to goal
    // max_distance: maximum path length to search
    // can_jump: whether the mob can jump up 1 block
    // can_swim: whether the mob can move through water
    PathfindingResult find_path(const PathNode& start, const PathNode& goal,
                                 f64 max_distance = 32.0,
                                 bool can_jump = true,
                                 bool can_swim = false);

    // Check if a block is walkable
    bool is_walkable(i32 x, i32 y, i32 z, bool can_swim = false) const;

    // Check if a block is solid
    bool is_solid(i32 x, i32 y, i32 z) const;

    // Check if there's a solid block below (ground check)
    bool has_ground_below(i32 x, i32 y, i32 z) const;

private:
    ChunkManager* chunk_manager_;

    // A* node for priority queue
    struct AStarNode {
        PathNode position;
        f64 g_cost;  // Cost from start
        f64 h_cost;  // Heuristic cost to goal
        f64 f_cost() const { return g_cost + h_cost; }
        PathNode parent;

        // For priority queue (lower f_cost has higher priority)
        bool operator>(const AStarNode& other) const {
            return f_cost() > other.f_cost();
        }
    };

    // Heuristic function (Manhattan distance with diagonal cost)
    f64 heuristic(const PathNode& from, const PathNode& to) const;

    // Get neighboring positions (includes jumping)
    std::vector<PathNode> get_neighbors(const PathNode& node, bool can_jump, bool can_swim) const;

    // Reconstruct path from parent map
    std::vector<PathNode> reconstruct_path(const std::unordered_map<PathNode, PathNode, PathNode::Hash>& came_from,
                                            const PathNode& start, const PathNode& goal) const;
};

// Path follower for mobs
class PathFollower {
public:
    PathFollower();

    // Set a new path to follow
    void set_path(const std::vector<PathNode>& path);

    // Get the next waypoint to move towards
    // Returns false if no more waypoints
    bool get_next_waypoint(f64 current_x, f64 current_y, f64 current_z,
                           f64& target_x, f64& target_y, f64& target_z);

    // Check if the path is complete
    bool is_path_complete() const { return current_waypoint_ >= path_.size(); }

    // Clear the current path
    void clear_path();

    // Check if there's an active path
    bool has_path() const { return !path_.empty() && !is_path_complete(); }

    // Get remaining waypoints
    usize remaining_waypoints() const {
        return path_.size() > current_waypoint_ ? path_.size() - current_waypoint_ : 0;
    }

private:
    std::vector<PathNode> path_;
    usize current_waypoint_ = 0;
};

} // namespace mcserver
