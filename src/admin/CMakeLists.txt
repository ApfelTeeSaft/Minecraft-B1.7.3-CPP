add_library(admin STATIC
    admin_manager.cpp
    admin_manager.hpp
)

target_include_directories(admin PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(admin PUBLIC util entity world)

set_target_properties(admin PROPERTIES FOLDER "Core")
