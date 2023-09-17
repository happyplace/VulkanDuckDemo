cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0025 NEW)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(VulkanDuckDemo)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64-bit builds supported.")
endif()

if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall -Werror")
elseif(${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall")
elseif (${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
    # /GL and /EHs are required for boost.context
    set(MSVC_COMMON_FLAGS_PS "/nologo /W4 /MP /GL /EHs")
    string(REPLACE ";" " " MSVC_COMMON_FLAGS_PS ${MSVC_COMMON_FLAGS_PS})
    set(CMAKE_CXX_FLAGS ${MSVC_COMMON_FLAGS_PS} CACHE STRING "C++ Common Flags" FORCE)
endif()

add_definitions(-DNOMINMAX)
add_definitions(-DDUCK_DEMO_VULKAN_DEBUG)
#add_definitions(-DDUCK_DEMO_VULKAN_PORTABILITY)

include_directories(src)
set(vdd-src
    "src/main.cpp"
    "src/DuckDemoUtils.cpp"
    "src/Game.cpp"
    "src/GameTimer.cpp"
    "src/DuckDemoGame.cpp"
)

include_directories(SYSTEM external/glm)

if (${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
    add_executable(VulkanDuckDemo WIN32 ${vdd-src})
else()
    add_executable(VulkanDuckDemo ${vdd-src})
endif()

target_compile_features(VulkanDuckDemo PRIVATE cxx_std_14)

if (${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
    # use the SDL 2 included in the project for windows builds
    set(SDL2_PATH "external/SDL2")

    # DLL needs to be copied to the output directory for it to be found on windows
    file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) DEST_DIR)
    file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2/lib/x64/SDL2.dll SDL2_DLL_PATH)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND xcopy ${SDL2_DLL_PATH} ${DEST_DIR} /yis)
endif()

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
target_link_libraries(VulkanDuckDemo ${SDL2_LIBRARY} ${SDL2MAIN_LIBRARY})

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})
target_link_libraries(VulkanDuckDemo ${Vulkan_LIBRARIES})
