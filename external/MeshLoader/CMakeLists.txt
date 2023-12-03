cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0025 NEW)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(MESH_LOADER_SHARED_LIBRARY "should be built as a shared library" ON)
option(MESH_LOADER_ENABLED_ASSIMP "should be built with assimp support" ON)

project(MeshLoader)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64-bit builds supported.")
endif()

add_definitions(-DNOMINMAX)

if (${MESH_LOADER_SHARED_LIBRARY})
    add_definitions(-DBUILDING_ML_DLL)
endif()

if (${MESH_LOADER_ENABLED_ASSIMP})
    add_definitions(-DML_ENABLE_ASSIMP)
endif()

include_directories(include)
set(meshloader-src
    "src/MeshLoader.cpp"
)

if (${MESH_LOADER_ENABLED_ASSIMP})
    set(meshloader-src ${meshloader-src}
        "src/AssimpLoader.cpp"
    )
endif()

if (${MESH_LOADER_SHARED_LIBRARY})
    add_library(MeshLoader SHARED ${meshloader-src})
else()
    add_library(MeshLoader ${meshloader-src})
endif()

target_compile_features(MeshLoader PRIVATE cxx_std_14)

if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(MeshLoader PRIVATE -fvisibility=hidden -Wall)
elseif(${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(MeshLoader PRIVATE -fvisibility=hidden -Wall -Wextra)
elseif (${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
    target_compile_options(MeshLoader PRIVATE /nologo /W4 /MP /GL /EHs)
endif()

if (${MESH_LOADER_ENABLED_ASSIMP})
    find_package(assimp REQUIRED)
    include_directories(${assimp_INCLUDE_DIRS})
    target_link_libraries(MeshLoader ${assimp_LIBRARIES})
endif()
