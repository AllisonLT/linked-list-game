cmake_minimum_required(VERSION 3.20)

# We enable C and CXX because glad.c is a C file, while our code is C++
project(LinkedListGame VERSION 0.1.0 LANGUAGES C CXX)
include(CTest)

# Target modern C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands so language servers (like clangd) understand our headers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# 1. External Dependencies via FetchContent
# ---------------------------------------------------------------------------
include(FetchContent)

# GLFW configuration (disable docs, examples, and tests so build is fast)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # Build statically

message(STATUS "Fetching GLFW...")
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# GLM configuration (header-only math library)
message(STATUS "Fetching GLM...")
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

message(STATUS "Fetching FreeType...")
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-3
)
FetchContent_MakeAvailable(freetype)

message(STATUS "Fetching nlohmann/json...")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------------------------
# 2. Local GLAD Loader (compiled as a static C library)
# ---------------------------------------------------------------------------
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# ---------------------------------------------------------------------------
# 3. Main Executable Target
# ---------------------------------------------------------------------------
# Collect all our project source files
file(GLOB_RECURSE APP_SOURCES
    CONFIGURE_DEPENDS
    src/*.cpp
    include/*.hpp
    include/*.h
)

# For app icon on Windows
set(APP_RESOURCES "")
if(WIN32)
    enable_language(RC)
    set(APP_RESOURCES assets/icon/app.rc)
endif()

add_executable(${PROJECT_NAME} ${APP_SOURCES} ${APP_RESOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_NONE)

# Include headers from our own include/ and third-party headers
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    external/glad/include
    external/stb
)

# Link GLFW, GLAD, GLM, and Windows OpenGL driver
target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    glfw
    glm::glm
    freetype
    nlohmann_json::nlohmann_json
    opengl32
)

# ---------------------------------------------------------------------------
# 4. Standalone Windows Binary Flags (MinGW)
# ---------------------------------------------------------------------------
# Statically links the C/C++ runtimes so the .exe runs without MSYS2 DLLs
if(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

if(BUILD_TESTING)
    add_executable(SinglyLinkedListTests
        test/SinglyLinkedListTests.cpp
        src/core/SandboxState.cpp
        src/core/Theme.cpp
    )
    target_include_directories(SinglyLinkedListTests PRIVATE include)
    target_link_libraries(SinglyLinkedListTests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME SinglyLinkedListTests COMMAND SinglyLinkedListTests)
    set_tests_properties(SinglyLinkedListTests PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()
