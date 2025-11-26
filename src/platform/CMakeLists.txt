add_library(platform STATIC
    net/socket.cpp
    net/socket.hpp
    net/tcp_listener.cpp
    net/tcp_listener.hpp
    thread/thread.cpp
    thread/thread.hpp
    thread/mutex.cpp
    thread/mutex.hpp
    time/clock.cpp
    time/clock.hpp
    fs/file.cpp
    fs/file.hpp
    fs/path.cpp
    fs/path.hpp
)

target_include_directories(platform PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(platform PROPERTIES FOLDER "Core")
