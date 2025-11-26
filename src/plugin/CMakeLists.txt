add_library(plugin STATIC
    plugin.hpp
    event/event.hpp
    event/event_manager.hpp
    event/event_manager.cpp
    event/player_events.hpp
    event/block_events.hpp
    event/entity_events.hpp
    event/server_events.hpp
    loader/plugin_loader.hpp
    loader/plugin_loader.cpp
)

target_include_directories(plugin PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(plugin PUBLIC util)

# Platform-specific libraries for dynamic loading
if(UNIX)
    target_link_libraries(plugin PRIVATE ${CMAKE_DL_LIBS})
endif()

set_target_properties(plugin PROPERTIES FOLDER "Plugin")
