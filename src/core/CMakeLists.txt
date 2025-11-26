add_library(core STATIC
    tick/tick_manager.cpp
    tick/tick_manager.hpp
    scheduler/job_system.cpp
    scheduler/job_system.hpp
    config/server_config.cpp
    config/server_config.hpp
    rng/random.cpp
    rng/random.hpp
)

target_include_directories(core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(core PUBLIC util platform)

set_target_properties(core PROPERTIES FOLDER "Core")
