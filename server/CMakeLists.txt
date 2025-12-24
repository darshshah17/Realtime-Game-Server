cmake_minimum_required(VERSION 3.15)
project(GameServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
pkg_check_modules(LIBWEBSOCKETS libwebsockets)

# If jsoncpp is not found via pkg-config, try to find it manually
if(NOT JSONCPP_FOUND)
    find_path(JSONCPP_INCLUDE_DIR json/json.h
        PATHS
        /opt/homebrew/include
        /usr/local/include
        /usr/include
    )
    find_library(JSONCPP_LIBRARY jsoncpp
        PATHS
        /opt/homebrew/lib
        /opt/homebrew/opt/jsoncpp/lib
        /usr/local/lib
        /usr/lib
        NAMES jsoncpp libjsoncpp
    )
    if(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARY)
        set(JSONCPP_FOUND TRUE)
        set(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIR})
        set(JSONCPP_LIBRARIES ${JSONCPP_LIBRARY})
    endif()
endif()

if(NOT JSONCPP_FOUND)
    message(FATAL_ERROR "jsoncpp not found. Please install it: brew install jsoncpp")
endif()

# If libwebsockets is not found via pkg-config, try to find it manually
if(NOT LIBWEBSOCKETS_FOUND)
    find_path(LIBWEBSOCKETS_INCLUDE_DIR libwebsockets.h
        PATHS
        /usr/local/include
        /opt/homebrew/include
        /usr/include
    )
    find_library(LIBWEBSOCKETS_LIBRARY websockets
        PATHS
        /usr/local/lib
        /opt/homebrew/lib
        /usr/lib
    )
    if(LIBWEBSOCKETS_INCLUDE_DIR AND LIBWEBSOCKETS_LIBRARY)
        set(LIBWEBSOCKETS_FOUND TRUE)
        set(LIBWEBSOCKETS_INCLUDE_DIRS ${LIBWEBSOCKETS_INCLUDE_DIR})
        set(LIBWEBSOCKETS_LIBRARIES ${LIBWEBSOCKETS_LIBRARY})
    endif()
endif()

if(NOT LIBWEBSOCKETS_FOUND)
    message(WARNING "libwebsockets not found. Please install it:")
    message(WARNING "  macOS: brew install libwebsockets")
    message(WARNING "  Linux: sudo apt-get install libwebsockets-dev")
endif()

# WebSocket library options
# Option 1: Use uWebSockets (recommended for production)
# Option 2: Use libwebsockets
# Option 3: Use a simple implementation (current placeholder)

# For now, we'll use a placeholder that needs to be replaced with a real WebSocket library
# In production, uncomment one of these:
# find_package(uWebSockets)
# find_package(Libwebsockets)

# Source files
set(SOURCES
    main.cpp
    GameServer.cpp
    WebSocketServer.cpp
    PlayerManager.cpp
    MatchmakingSystem.cpp
    ChatSystem.cpp
    GameStateManager.cpp
)

# Header files
set(HEADERS
    GameServer.h
    WebSocketServer.h
    PlayerManager.h
    MatchmakingSystem.h
    ChatSystem.h
    GameStateManager.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Find OpenSSL (required by libwebsockets)
find_package(OpenSSL REQUIRED)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    /opt/homebrew/opt/openssl@3/include
)

# Link directories
link_directories(
    /opt/homebrew/lib
    /opt/homebrew/opt/jsoncpp/lib
    /opt/homebrew/opt/openssl@3/lib
)

# Link libraries - ensure full paths
if(JSONCPP_LIBRARIES AND NOT JSONCPP_LIBRARIES MATCHES "^/")
    # If it's just a name, find the full path
    find_library(JSONCPP_FULL_PATH jsoncpp
        PATHS
        /opt/homebrew/lib
        /opt/homebrew/opt/jsoncpp/lib
        /usr/local/lib
    )
    if(JSONCPP_FULL_PATH)
        set(JSONCPP_LIBRARIES ${JSONCPP_FULL_PATH})
    else()
        set(JSONCPP_LIBRARIES "/opt/homebrew/opt/jsoncpp/lib/libjsoncpp.dylib")
    endif()
endif()

# Fix websockets library path if needed
if(LIBWEBSOCKETS_LIBRARIES AND NOT LIBWEBSOCKETS_LIBRARIES MATCHES "^/")
    find_library(LIBWEBSOCKETS_FULL_PATH websockets
        PATHS
        /opt/homebrew/lib
        /usr/local/lib
    )
    if(LIBWEBSOCKETS_FULL_PATH)
        set(LIBWEBSOCKETS_LIBRARIES ${LIBWEBSOCKETS_FULL_PATH})
    else()
        set(LIBWEBSOCKETS_LIBRARIES "/opt/homebrew/lib/libwebsockets.dylib")
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${JSONCPP_LIBRARIES}
    ${LIBWEBSOCKETS_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    pthread
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O2)
endif()

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

