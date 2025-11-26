#include "world_generator.hpp"
#include "core/rng/random.hpp"
#include "util/log/logger.hpp"
#include <cmath>
#include <random>

namespace mcserver {

WorldGenerator::WorldGenerator(i64 seed, GeneratorType type)
    : seed_(seed), generator_type_(type), noise_(std::make_unique<PerlinNoise>(seed)) {
    const char* type_name = "Unknown";
    switch (type) {
        case GeneratorType::Flat: type_name = "Flat"; break;
        case GeneratorType::Default: type_name = "Default"; break;
        case GeneratorType::Superflat: type_name = "Superflat"; break;
    }
    LOG_INFO_CAT("World generator initialized: seed=" + std::to_string(seed) +
                 ", type=" + std::string(type_name), LogCategory::World);
}

void WorldGenerator::generate_chunk(Chunk& chunk) {
    switch (generator_type_) {
        case GeneratorType::Flat:
            generate_flat(chunk);
            break;
        case GeneratorType::Default:
            generate_default(chunk);
            break;
        case GeneratorType::Superflat:
            generate_superflat(chunk);
            break;
    }

    chunk.mark_generated();
}

void WorldGenerator::generate_flat(Chunk& chunk) {
    // Simple flat world: bedrock, stone, dirt, grass
    for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
        for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
            // Bedrock at y=0
            chunk.set_block(x, 0, z, BlockId::Bedrock);

            // Stone from y=1 to y=59
            for (i32 y = 1; y < 60; ++y) {
                chunk.set_block(x, y, z, BlockId::Stone);
            }

            // Dirt from y=60 to y=62
            for (i32 y = 60; y < 63; ++y) {
                chunk.set_block(x, y, z, BlockId::Dirt);
            }

            // Grass at y=63
            chunk.set_block(x, 63, z, BlockId::Grass);

            // Update sky light (full light from grass level up)
            for (i32 y = 0; y <= 63; ++y) {
                chunk.set_sky_light(x, y, z, 0); // Underground, no sky light
            }
            for (i32 y = 64; y < CHUNK_SIZE_Y; ++y) {
                chunk.set_sky_light(x, y, z, 15); // Above ground, full sky light
            }
        }
    }
}

void WorldGenerator::generate_default(Chunk& chunk) {
    i32 chunk_x = chunk.get_x();
    i32 chunk_z = chunk.get_z();

    LOG_INFO_CAT("Generating chunk (" + std::to_string(chunk_x) + ", " + std::to_string(chunk_z) + ")",
                 LogCategory::World);

    // Track height statistics for debugging
    i32 min_height = CHUNK_SIZE_Y;
    i32 max_height = 0;
    i32 total_height = 0;
    i32 sample_count = 0;

    for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
        for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
            i32 world_x = chunk_x * CHUNK_SIZE_X + x;
            i32 world_z = chunk_z * CHUNK_SIZE_Z + z;

            // Calculate height and biome
            i32 height = calculate_height(world_x, world_z);
            BiomeType biome = get_biome(world_x, world_z);

            // Track statistics
            if (height < min_height) min_height = height;
            if (height > max_height) max_height = height;
            total_height += height;
            ++sample_count;

            // Bedrock at bottom
            chunk.set_block(x, 0, z, BlockId::Bedrock);

            // Stone up to near surface
            i32 stone_height = height - 4;
            for (i32 y = 1; y <= stone_height && y < CHUNK_SIZE_Y; ++y) {
                chunk.set_block(x, y, z, BlockId::Stone);
            }

            // Determine surface blocks based on biome
            BlockId surface_block = BlockId::Grass;
            BlockId subsurface_block = BlockId::Dirt;

            switch (biome) {
                case BiomeType::Desert:
                    surface_block = BlockId::Sand;
                    subsurface_block = BlockId::Sand;
                    break;
                case BiomeType::Ocean:
                    surface_block = BlockId::Gravel;
                    subsurface_block = BlockId::Gravel;
                    break;
                case BiomeType::Hills:
                    // Some exposed stone on hills
                    if (height > 75) {
                        surface_block = BlockId::Stone;
                        subsurface_block = BlockId::Stone;
                    } else {
                        surface_block = BlockId::Grass;
                        subsurface_block = BlockId::Dirt;
                    }
                    break;
                case BiomeType::Plains:
                case BiomeType::Forest:
                default:
                    surface_block = BlockId::Grass;
                    subsurface_block = BlockId::Dirt;
                    break;
            }

            // Subsurface layer
            for (i32 y = stone_height + 1; y < height && y < CHUNK_SIZE_Y; ++y) {
                chunk.set_block(x, y, z, subsurface_block);
            }

            // Surface block
            if (height > 0 && height < CHUNK_SIZE_Y) {
                chunk.set_block(x, height, z, surface_block);
            }

            // Fill ocean areas with water (sea level = 62)
            constexpr i32 sea_level = 62;
            if (height < sea_level && biome == BiomeType::Ocean) {
                for (i32 y = height + 1; y <= sea_level && y < CHUNK_SIZE_Y; ++y) {
                    chunk.set_block(x, y, z, BlockId::WaterStill);
                    // Water blocks some light
                    chunk.set_sky_light(x, y, z, 10);
                }
            }

            // Update sky light
            for (i32 y = 0; y <= height && y < CHUNK_SIZE_Y; ++y) {
                chunk.set_sky_light(x, y, z, 0); // Underground
            }
            for (i32 y = height + 1; y < CHUNK_SIZE_Y; ++y) {
                // Skip if already set (water)
                if (chunk.get_block(x, y, z) == static_cast<u8>(BlockId::Air)) {
                    chunk.set_sky_light(x, y, z, 15); // Above ground
                }
            }
        }
    }

    // Generate caves (carve out stone)
    generate_caves(chunk, chunk_x, chunk_z);

    // Generate trees after terrain is complete
    place_trees(chunk, chunk_x, chunk_z);

    // Log terrain generation statistics (commented out to reduce log clutter)
    // i32 avg_height = sample_count > 0 ? total_height / sample_count : 0;
    // LOG_DEBUG_CAT("Generated chunk (" + std::to_string(chunk_x) + ", " + std::to_string(chunk_z) +
    //               ") - Heights: min=" + std::to_string(min_height) +
    //               ", max=" + std::to_string(max_height) +
    //               ", avg=" + std::to_string(avg_height), LogCategory::World);
}

void WorldGenerator::generate_superflat(Chunk& chunk) {
    // Extremely flat: bedrock + grass only
    for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
        for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
            chunk.set_block(x, 0, z, BlockId::Bedrock);
            chunk.set_block(x, 1, z, BlockId::Grass);

            // Sky light
            for (i32 y = 0; y <= 1; ++y) {
                chunk.set_sky_light(x, y, z, 0);
            }
            for (i32 y = 2; y < CHUNK_SIZE_Y; ++y) {
                chunk.set_sky_light(x, y, z, 15);
            }
        }
    }
}

i32 WorldGenerator::calculate_height(i32 world_x, i32 world_z) {
    BiomeType biome = get_biome(world_x, world_z);

    // Use multi-scale noise for Beta 1.7.3-style terrain
    // Large-scale features (continental)
    f64 large_scale = 0.002;
    f64 large_noise = noise_->octave_noise_2d(
        world_x * large_scale,
        world_z * large_scale,
        4,    // Fewer octaves for smooth large features
        0.6   // Higher persistence for rolling terrain
    );

    // Medium-scale features (hills and valleys)
    f64 medium_scale = 0.008;
    f64 medium_noise = noise_->octave_noise_2d(
        world_x * medium_scale,
        world_z * medium_scale,
        5,    // Medium detail
        0.5
    );

    // Small-scale details (local variation)
    f64 small_scale = 0.03;
    f64 small_noise = noise_->octave_noise_2d(
        world_x * small_scale,
        world_z * small_scale,
        3,    // Fine details
        0.4
    );

    // Combine noise layers with different weights
    f64 combined_noise = (large_noise * 0.5) + (medium_noise * 0.35) + (small_noise * 0.15);

    // Different height ranges per biome (increased variations for more dramatic terrain)
    i32 base_height = 64;
    i32 height_variation = 16;

    switch (biome) {
        case BiomeType::Plains:
            base_height = 66;
            height_variation = 8;  // Gentle rolling plains
            break;
        case BiomeType::Desert:
            base_height = 67;
            height_variation = 12; // Sand dunes with more variation
            break;
        case BiomeType::Forest:
            base_height = 68;
            height_variation = 16; // More varied, hilly forests
            break;
        case BiomeType::Ocean:
            base_height = 48;      // Below sea level (62)
            height_variation = 10; // Ocean floor variation
            break;
        case BiomeType::Hills:
            base_height = 76;      // Much more elevated
            height_variation = 32; // Very dramatic hills and mountains
            break;
    }

    i32 height = base_height + static_cast<i32>(combined_noise * height_variation);

    // Clamp to valid range
    if (height < 1) height = 1;
    if (height >= CHUNK_SIZE_Y - 1) height = CHUNK_SIZE_Y - 2;

    return height;
}

BiomeType WorldGenerator::get_biome(i32 world_x, i32 world_z) {
    // Use two noise functions for temperature and moisture
    f64 temp_scale = 0.003; // Large-scale biome features
    f64 temp = noise_->octave_noise_2d(
        world_x * temp_scale,
        world_z * temp_scale,
        4, 0.6
    );

    f64 moisture_scale = 0.004; // Different scale for variation
    f64 moisture = noise_->octave_noise_2d(
        (world_x + 10000) * moisture_scale,  // Offset to decorrelate from temp
        (world_z + 10000) * moisture_scale,
        4, 0.6
    );

    // Map temperature and moisture to biomes
    // temp: -1 to 1, moisture: -1 to 1
    if (temp < -0.3) {
        // Cold biomes
        if (moisture < 0.0) {
            return BiomeType::Plains;  // Cold plains
        } else {
            return BiomeType::Forest;  // Cold forest
        }
    } else if (temp > 0.3) {
        // Hot biomes
        if (moisture < -0.2) {
            return BiomeType::Desert;  // Hot and dry
        } else {
            return BiomeType::Plains;  // Hot plains
        }
    } else {
        // Temperate biomes
        if (moisture < -0.4) {
            return BiomeType::Plains;
        } else if (moisture > 0.4) {
            return BiomeType::Ocean;
        } else {
            return BiomeType::Forest;
        }
    }
}

void WorldGenerator::generate_caves(Chunk& chunk, i32 chunk_x, i32 chunk_z) {
    // Use 3D Perlin noise to create cave systems
    f64 cave_scale = 0.05;  // Controls cave feature size
    f64 cave_threshold = 0.6;  // Higher = fewer/smaller caves

    for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
        for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
            i32 world_x = chunk_x * CHUNK_SIZE_X + x;
            i32 world_z = chunk_z * CHUNK_SIZE_Z + z;

            // Generate caves from y=5 to y=60 (not too close to surface or bedrock)
            for (i32 y = 5; y < 60; ++y) {
                // Get current block - only carve through stone/dirt/gravel
                u8 current_block = chunk.get_block(x, y, z);
                if (current_block != static_cast<u8>(BlockId::Stone) &&
                    current_block != static_cast<u8>(BlockId::Dirt) &&
                    current_block != static_cast<u8>(BlockId::Gravel)) {
                    continue;  // Don't carve through air, water, or special blocks
                }

                // Sample 3D noise at this position
                // Note: PerlinNoise only has 2D functions, so we'll simulate 3D
                // by combining two 2D noise samples with y offset
                f64 noise1 = noise_->octave_noise_2d(
                    world_x * cave_scale,
                    (world_z + y * 16) * cave_scale,  // Offset by y
                    4, 0.5
                );
                f64 noise2 = noise_->octave_noise_2d(
                    (world_x + y * 16) * cave_scale,  // Offset by y differently
                    world_z * cave_scale,
                    4, 0.5
                );

                // Combine noise values to create 3D-like cave structure
                f64 cave_noise = (noise1 + noise2) * 0.5;

                // Carve out cave if noise exceeds threshold
                if (cave_noise > cave_threshold) {
                    chunk.set_block(x, y, z, BlockId::Air);
                    // Update sky light for cave
                    chunk.set_sky_light(x, y, z, 0);
                }
            }
        }
    }
}

void WorldGenerator::place_trees(Chunk& chunk, i32 chunk_x, i32 chunk_z) {
    // Use chunk coordinates to seed tree placement
    // This ensures trees generate consistently for the same chunk
    std::mt19937 rng(static_cast<unsigned int>(seed_ + chunk_x * 341873128712LL + chunk_z * 132897987541LL));
    std::uniform_int_distribution<i32> pos_dist(0, CHUNK_SIZE_X - 1);
    std::uniform_int_distribution<i32> chance_dist(0, 99);
    std::uniform_int_distribution<i32> height_dist(4, 6);

    // Try to place trees in this chunk (about 1-2 trees per chunk on average)
    i32 tree_attempts = 3;
    for (i32 attempt = 0; attempt < tree_attempts; ++attempt) {
        // Random position within chunk
        i32 local_x = pos_dist(rng);
        i32 local_z = pos_dist(rng);
        i32 world_x = chunk_x * CHUNK_SIZE_X + local_x;
        i32 world_z = chunk_z * CHUNK_SIZE_Z + local_z;

        // Check biome - only place trees in forests and plains
        BiomeType biome = get_biome(world_x, world_z);
        i32 tree_chance = 0;
        switch (biome) {
            case BiomeType::Forest:
                tree_chance = 60;  // 60% chance in forests (lots of trees)
                break;
            case BiomeType::Plains:
                tree_chance = 20;  // 20% chance in plains (sparse trees)
                break;
            default:
                continue;  // No trees in deserts, oceans, hills
        }

        // Random chance to spawn tree based on biome
        if (chance_dist(rng) > tree_chance) {
            continue;
        }

        // Find surface height at this position
        i32 surface_y = -1;
        for (i32 y = CHUNK_SIZE_Y - 1; y >= 0; --y) {
            u8 block = chunk.get_block(local_x, y, local_z);
            if (block == static_cast<u8>(BlockId::Grass)) {
                surface_y = y;
                break;
            }
        }

        // Skip if no grass block found or too close to chunk borders
        if (surface_y < 0 || surface_y > CHUNK_SIZE_Y - 10) {
            continue;
        }

        // Skip if too close to chunk edges (need 3 blocks on each side for full leaf coverage)
        // This prevents leaves from being cut off at chunk boundaries
        if (local_x < 3 || local_x >= CHUNK_SIZE_X - 3 ||
            local_z < 3 || local_z >= CHUNK_SIZE_Z - 3) {
            continue;
        }

        // Random tree height (4-6 blocks)
        i32 tree_height = height_dist(rng);

        // Generate oak tree
        generate_oak_tree(chunk, local_x, surface_y + 1, local_z, tree_height);
    }
}

void WorldGenerator::generate_oak_tree(Chunk& chunk, i32 x, i32 base_y, i32 z, i32 height) {
    // Place trunk (wood blocks)
    for (i32 y = 0; y < height; ++y) {
        i32 trunk_y = base_y + y;
        if (trunk_y >= 0 && trunk_y < CHUNK_SIZE_Y) {
            chunk.set_block(x, trunk_y, z, BlockId::Wood);
            // Update sky light - trunk blocks light
            chunk.set_sky_light(x, trunk_y, z, 0);
        }
    }

    // Place leaves (3x3x3 cube on top with variations)
    i32 leaves_base_y = base_y + height - 2;  // Start 2 blocks from top

    // Layer 1 (top of tree) - smaller
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            i32 leaf_x = x + dx;
            i32 leaf_z = z + dz;
            i32 leaf_y = base_y + height;

            // Skip corners on top layer
            if ((dx == -1 || dx == 1) && (dz == -1 || dz == 1)) {
                continue;
            }

            if (leaf_x >= 0 && leaf_x < CHUNK_SIZE_X &&
                leaf_z >= 0 && leaf_z < CHUNK_SIZE_Z &&
                leaf_y >= 0 && leaf_y < CHUNK_SIZE_Y) {

                u8 existing = chunk.get_block(leaf_x, leaf_y, leaf_z);
                if (existing == static_cast<u8>(BlockId::Air)) {
                    chunk.set_block(leaf_x, leaf_y, leaf_z, BlockId::Leaves);
                    // Leaves let some light through (light level 1 below)
                    chunk.set_sky_light(leaf_x, leaf_y, leaf_z, 1);
                }
            }
        }
    }

    // Layers 2-3 (middle and lower leaves) - full 3x3
    for (i32 layer = 0; layer < 2; ++layer) {
        i32 leaf_y = leaves_base_y + layer;

        for (i32 dx = -2; dx <= 2; ++dx) {
            for (i32 dz = -2; dz <= 2; ++dz) {
                // Skip far corners for more natural shape
                if ((dx == -2 || dx == 2) && (dz == -2 || dz == 2)) {
                    continue;
                }

                i32 leaf_x = x + dx;
                i32 leaf_z = z + dz;

                if (leaf_x >= 0 && leaf_x < CHUNK_SIZE_X &&
                    leaf_z >= 0 && leaf_z < CHUNK_SIZE_Z &&
                    leaf_y >= 0 && leaf_y < CHUNK_SIZE_Y) {

                    u8 existing = chunk.get_block(leaf_x, leaf_y, leaf_z);
                    // Don't replace trunk or solid blocks with leaves
                    if (existing == static_cast<u8>(BlockId::Air)) {
                        chunk.set_block(leaf_x, leaf_y, leaf_z, BlockId::Leaves);
                        chunk.set_sky_light(leaf_x, leaf_y, leaf_z, 1);
                    }
                }
            }
        }
    }
}

} // namespace mcserver
