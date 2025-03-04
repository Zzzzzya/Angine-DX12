cmake_minimum_required(VERSION 3.20)

project(DX12)

add_definitions(-w)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

# 在CMakeLists.txt文件中添加
add_definitions(-DUNICODE -D_UNICODE)

find_package(fmt CONFIG REQUIRED)
find_path(D3DX12_INCLUDE_DIRS "d3dx12.h")
find_package(directx-headers CONFIG REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/Include)

file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/Src/*.cc)

# ...existing code...
file(GLOB_RECURSE TEST_SOURCES ${PROJECT_SOURCE_DIR}/Test/*.cc)

foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE} ${SRC_FILES})

    # target_include_directories(${TEST_NAME} PRIVATE ${D3DX12_INCLUDE_DIRS})
    target_link_libraries(${TEST_NAME} PRIVATE fmt::fmt)
    target_link_libraries(${TEST_NAME} PRIVATE Microsoft::DirectX-Headers)

    target_link_libraries(${TEST_NAME} PRIVATE dxgi.lib d3d12.lib d3dcompiler.lib)
endforeach()

# ...existing code...
