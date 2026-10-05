cmake_minimum_required(VERSION 3.16)

project(azzammar 
        VERSION 1.0.0 
        LANGUAGES CXX)

# Enforce Modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic Qt code generation tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Required Qt6 Modules (Core and Network are essential for headless/server)
set(CMAKE_PREFIX_PATH "/Volumes/MacHD/Qt/6.7.3/macos")
find_package(Qt6 COMPONENTS Core Network NetworkAuth Sql REQUIRED)
find_package(Qt6 COMPONENTS Widgets Gui WebEngineWidgets OPTIONAL_COMPONENTS)

# Include Subdirectories (Order matters: Core must be built before things that depend on it)
add_subdirectory(src/common)
add_subdirectory(src/core)
add_subdirectory(src/apps)
#add_subdirectory(src/apps/server)
#add_subdirectory(src/apps/desktop)
#add_subdirectory(src/apps/mobile)
#add_subdirectory(src/apps/cli)
add_subdirectory(src/web)

enable_testing()
add_subdirectory(tests)
