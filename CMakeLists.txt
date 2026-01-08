cmake_minimum_required(VERSION 3.28)
# DevTools
project(he2-rt VERSION 0.2.5
                 DESCRIPTION "Hedgehog Engine 2 Raytracer"
                 LANGUAGES CXX)

find_package(rip)

include(FetchContent)
FetchContent_Declare(eigen GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git EXCLUDE_FROM_ALL)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git EXCLUDE_FROM_ALL)
FetchContent_Declare(
    vmahpp
    URL      https://github.com/YaaZ/VulkanMemoryAllocator-Hpp/releases/download/v3.3.0+1/VulkanMemoryAllocator-Hpp-3.3.0.tar.gz
    URL_HASH SHA256=8eab33e9d4a088470d2391667b48df693a6398bb76ccea20f3d4275ad85fab8a
)
FetchContent_MakeAvailable(eigen glfw vmahpp)

set(PROJECT_AUTHOR "angryzor")
set(PROJECT_COPYRIGHT "Copyright (c) 2024 angryzor - Licensed under GPL3")
set(PROJECT_DOWNLOAD_SIZE "2.42 MB")
string(TIMESTAMP PROJECT_BUILD_DATE "%Y-%m-%d")
option(DEVTOOLS_ENABLE_IMGUI_DEMO_WINDOW "Enable ImGui demo window" OFF)
option(DEVTOOLS_ENABLE_IMPLOT_DEMO_WINDOW "Enable ImPlot demo window" OFF)
option(DEVTOOLS_INSTALL_TO_GAME_FOLDER "Install to game folder" ON)
option(DEVTOOLS_INSTALL_TO_UPDATE_SERVER "Install to update server" OFF)
set(DEVTOOLS_UPDATE_SERVER_PATH "" CACHE PATH "Path to update server")

#add_subdirectory(shaders)
set(SHADERS
    src/raytracing.slang
    src/post-processing/tonemapper/tonemapper.slang
)

foreach(source IN LISTS SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      "C:/VulkanSDK/1.4.335.0/Bin/slangc.exe"
      -source-embed-style u8
      -o ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv
      ${CMAKE_CURRENT_SOURCE_DIR}/${source}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv.h)
endforeach()

add_executable(he2-rt ${SPV_SHADERS})
target_include_directories(he2-rt PRIVATE "C:/VulkanSDK/1.4.335.0/Include" ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(he2-rt PRIVATE "C:/VulkanSDK/1.4.335.0/Lib")
target_link_libraries(he2-rt PRIVATE vulkan-1 glfw VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp Eigen3::Eigen rip-hl)
target_compile_features(he2-rt PRIVATE cxx_std_20)
target_compile_definitions(he2-rt PRIVATE
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

add_subdirectory(src)
