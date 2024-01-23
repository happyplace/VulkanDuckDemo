cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0025 NEW)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(VulkanDuckDemo)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64-bit builds supported.")
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
    "src/ImGuiRenderPass.cpp"
    "src/MeshRenderPass.cpp"
    "src/VulkanBuffer.cpp"
    "src/VulkanTexture.cpp"
)

include_directories(SYSTEM external/glm)

include_directories(SYSTEM external/stb)

include_directories("external/imgui")
include_directories("external/imgui/backends")
set(vdd-src ${vdd-src}
    "external/imgui/imgui.cpp"
    "external/imgui/imgui_draw.cpp"
    "external/imgui/imgui_widgets.cpp"
    "external/imgui/imgui_tables.cpp"
    "external/imgui/backends/imgui_impl_vulkan.cpp"
    "external/imgui/backends/imgui_impl_sdl2.cpp"
)

if (${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
    add_executable(VulkanDuckDemo WIN32 ${vdd-src})
else()
    add_executable(VulkanDuckDemo ${vdd-src})
endif()

target_compile_features(VulkanDuckDemo PRIVATE cxx_std_14)
#set_property(TARGET VulkanDuckDemo PROPERTY COMPILE_WARNING_AS_ERROR ON)

if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(VulkanDuckDemo PRIVATE -Wall)
elseif(${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(VulkanDuckDemo PRIVATE -Wall -Wextra)
elseif (${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
    target_compile_options(VulkanDuckDemo PRIVATE /nologo /W4 /MP /GL /EHs)
endif()

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

set(SHADERC_SKIP_TESTS YES)
set(SHADERC_SKIP_EXAMPLES YES)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/shaderc)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external/shaderc/libshaderc/include)
target_link_libraries(VulkanDuckDemo shaderc)

include_directories(VulkanDuckDemo ${CMAKE_CURRENT_SOURCE_DIR}/external/glm)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/MeshLoader)
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external/MeshLoader/include)
target_link_libraries(VulkanDuckDemo MeshLoader)
