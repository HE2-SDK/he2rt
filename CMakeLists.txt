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
FetchContent_Declare(
    entt
    GIT_REPOSITORY  https://github.com/skypjack/entt.git
    GIT_TAG         v3.16.0
)
FetchContent_Declare(
    dds_image
    GIT_REPOSITORY  https://github.com/spnda/dds_image.git
    GIT_TAG         84a7bd47293a87214d454992deb8fdf3cba91c48
)
FetchContent_MakeAvailable(eigen glfw vmahpp entt dds_image)

set(PROJECT_AUTHOR "angryzor")
set(PROJECT_COPYRIGHT "Copyright (c) 2024 angryzor - Licensed under GPL3")
set(PROJECT_DOWNLOAD_SIZE "2.42 MB")
string(TIMESTAMP PROJECT_BUILD_DATE "%Y-%m-%d")
option(DEVTOOLS_ENABLE_IMGUI_DEMO_WINDOW "Enable ImGui demo window" OFF)
option(DEVTOOLS_ENABLE_IMPLOT_DEMO_WINDOW "Enable ImPlot demo window" OFF)
option(DEVTOOLS_INSTALL_TO_GAME_FOLDER "Install to game folder" ON)
option(DEVTOOLS_INSTALL_TO_UPDATE_SERVER "Install to update server" OFF)
set(DEVTOOLS_UPDATE_SERVER_PATH "" CACHE PATH "Path to update server")

add_subdirectory(vendor EXCLUDE_FROM_ALL)

#add_subdirectory(shaders)
set(SHADERS
    src/raytracing.slang
    src/common_dpn.slang
    src/post-processing/tonemapper/tonemapper.slang
)

foreach(source IN LISTS SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  get_filename_component(BASENAME ${source} NAME_WE)
  add_custom_command(
    COMMAND
      "C:/VulkanSDK/1.4.335.0/Bin/slangc.exe"
      -depfile ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv.dep
      -o ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv
      ${CMAKE_CURRENT_SOURCE_DIR}/${source}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv.dep
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source}
    DEPFILE ${FILENAME}.spv.dep
    COMMENT "Compiling ${FILENAME}"
  )
  add_custom_command(
    COMMAND
      "C:/VulkanSDK/1.4.335.0/Bin/slangc.exe"
      -depfile ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv.h.dep
      -source-embed-style u32
      -source-embed-name ${BASENAME}_shader
      -o ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv
      ${CMAKE_CURRENT_SOURCE_DIR}/${source}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv.h ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv.h.dep
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source}
    DEPFILE ${FILENAME}.spv.h.dep
    COMMENT "Compiling ${FILENAME} embedded version"
  )
  list(APPEND SPV_SHADERS ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv.h)
endforeach()

add_executable(he2-rt ${SPV_SHADERS})
target_include_directories(he2-rt PRIVATE "C:/VulkanSDK/1.4.335.0/Include" "C:/Program Files/NVIDIA Corporation/Nsight Graphics 2025.5.0/SDKs/NsightAftermathSDK/2025.5.0.251114/include" ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(he2-rt PRIVATE "C:/VulkanSDK/1.4.335.0/Lib" "C:/Program Files/NVIDIA Corporation/Nsight Graphics 2025.5.0/SDKs/NsightAftermathSDK/2025.5.0.251114/lib/x64")
target_link_libraries(he2-rt PRIVATE vulkan-1 GFSDK_Aftermath_Lib.x64 glfw EnTT::EnTT VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp Eigen3::Eigen rip-hl dds_image imgui-backend-glfw imgui-backend-vulkan)
target_compile_features(he2-rt PRIVATE cxx_std_20)
target_compile_definitions(he2-rt PRIVATE
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

add_subdirectory(src)
