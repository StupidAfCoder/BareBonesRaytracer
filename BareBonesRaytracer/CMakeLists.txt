cmake_minimum_required(VERSION 3.20)
project(BareBonesRaytracer VERSION 1.0 LANGUAGES CXX)

# Lock to C++14 as requested
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable compiler-specific extensions (enforces strict cross-platform C++)
set(CMAKE_CXX_EXTENSIONS OFF) 

# Automatically download and build SDL3 so users don't have to
include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0 # Pulls the stable release
)
FetchContent_MakeAvailable(SDL3)

# Find all your C++ source files, AND the ImGui source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "external/ImgGui/*.cpp"
    "external/SDL3-3.4.8/*.cpp"
)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Tell the compiler where to look for your #include statements
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/ImgGui
    ${CMAKE_SOURCE_DIR}/external/SDL3-3.4.8
)

# Link SDL3 to your engine
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)