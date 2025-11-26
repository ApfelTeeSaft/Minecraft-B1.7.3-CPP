#pragma once

#include "util/types.hpp"
#include "world/chunk/chunk.hpp"
#include "world/generation/noise.hpp"
#include <memory>

namespace mcserver {

enum class GeneratorType {
    Flat,        // Flat world for testing
    Default,     // Beta 1.7.3 style terrain
    Superflat    // Completely flat bedrock + grass
};

enum class BiomeType {
    Plains,      // Grass, normal terrain
    Desert,      // Sand, flat
    Forest,      // Trees, grass
    Ocean,       // Water, gravel
    Hills        // Stone, elevated
};

class WorldGenerator {
public:
    explicit WorldGenerator(i64 seed, GeneratorType type = GeneratorType::Default);

    // Generate a chunk's terrain
    void generate_chunk(Chunk& chunk);

    // Set generator type
    void set_generator_type(GeneratorType type) { generator_type_ = type; }
    GeneratorType get_generator_type() const { return generator_type_; }

    i64 get_seed() const { return seed_; }

private:
    i64 seed_;
    GeneratorType generator_type_;
    std::unique_ptr<PerlinNoise> noise_;

    // Different generation methods
    void generate_flat(Chunk& chunk);
    void generate_default(Chunk& chunk);
    void generate_superflat(Chunk& chunk);

    // Helper methods for default generation
    i32 calculate_height(i32 world_x, i32 world_z);
    BiomeType get_biome(i32 world_x, i32 world_z);
    void generate_caves(Chunk& chunk, i32 chunk_x, i32 chunk_z);
    void place_trees(Chunk& chunk, i32 chunk_x, i32 chunk_z);
    void generate_oak_tree(Chunk& chunk, i32 x, i32 base_y, i32 z, i32 height);
};

} // namespace mcserver
