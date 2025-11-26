#include "util/log/logger.hpp"
#include "util/types.hpp"
#include "platform/net/socket.hpp"
#include "core/config/server_config.hpp"
#include "core/tick/tick_manager.hpp"
#include "core/scheduler/job_system.hpp"
#include "net/transport/network_manager.hpp"
#include "world/chunk/chunk_manager.hpp"
#include "world/generation/world_generator.hpp"
#include "storage/chunk/chunk_storage.hpp"
#include "entity/entity_manager.hpp"

#include <iostream>
#include <filesystem>
#include <csignal>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <random>

using namespace mcserver;

static std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    (void)signal;
    g_running = false;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Initialize logging
    Logger::instance().init("server.log");
    Logger::instance().set_min_level(LogLevel::Debug);

    LOG_INFO("=== Minecraft Beta 1.7.3 Server - Modern C++ Implementation ===");
    LOG_INFO("Starting server...");

    // Initialize platform networking
    auto net_init_result = init_networking();
    if (!net_init_result) {
        LOG_FATAL("Failed to initialize networking");
        return 1;
    }

    // Load server configuration
    ServerConfig config;
    auto config_result = config.load("server.properties");
    if (!config_result) {
        LOG_WARNING("Failed to load server.properties, using defaults");
    }

    // Save default config if it doesn't exist
    config.save("server.properties");

    // Log configuration
    LOG_INFO(std::string("Server IP: ") + (config.server_ip().empty() ? "*" : config.server_ip()));
    LOG_INFO(std::string("Server Port: ") + std::to_string(config.server_port()));
    LOG_INFO(std::string("Level Name: ") + config.level_name());
    LOG_INFO(std::string("Online Mode: ") + (config.online_mode() ? "true" : "false"));
    LOG_INFO(std::string("Max Players: ") + std::to_string(config.max_players()));

    // Initialize job system
    JobSystem job_system;
    job_system.start();
    LOG_INFO(std::string("Job system started with ") + std::to_string(job_system.thread_count()) + " threads");

    // Create world directory structure
    std::filesystem::path world_path = config.level_name();
    std::filesystem::create_directories(world_path);
    LOG_INFO(std::string("World directory: ") + world_path.string());

    // Create necessary subdirectories
    std::filesystem::path players_dir = world_path / "players";
    std::filesystem::create_directories(players_dir);
    LOG_INFO(std::string("Players directory: ") + players_dir.string());

    // Create plugins directory
    std::filesystem::path plugins_dir = "plugins";
    std::filesystem::create_directories(plugins_dir);
    LOG_INFO(std::string("Plugins directory: ") + plugins_dir.string());

    // Initialize world seed with persistence
    i64 seed = 0;
    std::filesystem::path seed_file = world_path / "seed.txt";
    bool seed_from_config = !config.level_seed().empty();
    bool seed_file_exists = std::filesystem::exists(seed_file);

    if (seed_from_config) {
        // Seed specified in config - use it
        const char* str = config.level_seed().c_str();
        char* end;
        long long value = std::strtoll(str, &end, 10);
        if (end != str && *end == '\0') {
            seed = static_cast<i64>(value);
        } else {
            // Use string hash as seed
            std::hash<std::string> hasher;
            seed = static_cast<i64>(hasher(config.level_seed()));
        }
        LOG_INFO(std::string("Using seed from config: ") + std::to_string(seed));
    } else if (seed_file_exists) {
        // Load existing seed from file
        std::ifstream seed_input(seed_file);
        if (seed_input >> seed) {
            LOG_INFO(std::string("Loaded existing world seed: ") + std::to_string(seed));
        } else {
            LOG_WARNING("Failed to read seed file, generating new seed");
            seed_file_exists = false;  // Force new seed generation
        }
    }

    if (!seed_from_config && !seed_file_exists) {
        // Generate new random seed using random_device for better randomness
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<i64> dis;
        seed = dis(gen);

        LOG_INFO(std::string("Generated new world seed: ") + std::to_string(seed));

        // Save seed to file for persistence
        std::ofstream seed_output(seed_file);
        if (seed_output) {
            seed_output << seed << std::endl;
            LOG_INFO("Saved seed to seed.txt");
        } else {
            LOG_WARNING("Failed to save seed to seed.txt");
        }
    }

    LOG_INFO(std::string("World seed: ") + std::to_string(seed));

    // Initialize world storage
    ChunkStorage chunk_storage(world_path.string());

    // Initialize world generator and chunk manager
    WorldGenerator world_gen(seed);
    ChunkManager chunk_manager(&world_gen, &chunk_storage);
    EntityManager entity_manager;

    // Start network listening
    NetworkManager network(&chunk_manager, world_path.string());
    auto network_result = network.start(config.server_ip(), config.server_port());
    if (!network_result) {
        LOG_FATAL("Failed to bind to port");
        shutdown_networking();
        return 1;
    }

    LOG_INFO("Server started successfully!");

    // Natural mob spawning is now enabled
    // (Test mob spawning disabled - mobs will spawn naturally based on light level)
    // Uncomment below to spawn test mobs at startup:
    // network.get_mob_manager()->spawn_test_mobs(0.0, 0.0);
    // network.get_mob_manager()->spawn_test_hostile_mobs(0.0, 0.0);

    if (network.get_mob_manager()->get_spawner()) {
        LOG_INFO("Natural mob spawning enabled (spawn limit: " +
                 std::to_string(network.get_mob_manager()->get_spawner()->get_spawn_limit()) + ")");
    }

    LOG_INFO("Ready to accept connections");

    // Set up signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Main server loop
    TickManager tick_manager;
    tick_manager.reset();

    i64 tick_count = 0;
    constexpr i64 auto_save_interval = 6000; // Auto-save every 5 minutes (6000 ticks)

    while (g_running) {
        i64 ticks_to_run = 0;

        if (tick_manager.should_tick(ticks_to_run)) {
            for (i64 i = 0; i < ticks_to_run && g_running; ++i) {
                tick_manager.tick_started();

                // Network tick
                network.tick();

                // World tick
                chunk_manager.tick();

                // Entity tick
                entity_manager.tick();

                // Mob tick
                network.get_mob_manager()->update_all();

                tick_manager.tick_finished();
                ++tick_count;

                // Auto-save world every 5 minutes
                if (tick_count % auto_save_interval == 0) {
                    LOG_INFO("Auto-saving world...");
                    chunk_manager.save_all_dirty();
                    LOG_INFO("World saved successfully");
                }

                // Log status every 20 seconds (400 ticks)
                if (tick_count % 400 == 0) {
                    LOG_INFO_CAT(
                        std::string("Tick: ") + std::to_string(tick_count) +
                        " | Clients: " + std::to_string(network.client_count()) +
                        " | Chunks: " + std::to_string(chunk_manager.get_loaded_chunk_count()) +
                        " | Avg tick: " + std::to_string(tick_manager.average_tick_time_ms()) + "ms",
                        LogCategory::Performance
                    );
                }
            }
        } else {
            // Sleep a bit to avoid busy waiting
            Clock::sleep_ms(1);
        }
    }

    LOG_INFO("Shutting down server...");

    // Save all chunks before shutdown
    LOG_INFO("Saving world...");
    chunk_manager.save_all();
    chunk_storage.close_all();
    LOG_INFO("World saved successfully");

    // Stop network
    network.stop();

    // Stop job system
    job_system.stop();

    // Shutdown networking
    shutdown_networking();

    LOG_INFO("Server shut down cleanly");
    Logger::instance().shutdown();

    return 0;
}
