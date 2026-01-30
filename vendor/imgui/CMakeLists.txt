cmake_minimum_required(VERSION 3.28)
project(imgui LANGUAGES CXX)

add_library(imgui STATIC)
target_compile_features(imgui PRIVATE cxx_std_20)
target_compile_definitions(imgui PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
target_sources(imgui
    PRIVATE
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS imgui FILES
        imgui/imgui.h
    PRIVATE FILE_SET private_headers TYPE HEADERS BASE_DIRS imgui FILES
        imgui/imgui_internal.h
)

if(${IMGUI_DEMO_WINDOW})
	target_sources(imgui
		PRIVATE
			imgui/imgui_demo.cpp
	)
endif()

if(${IMGUI_BUILD_BACKEND_VULKAN})
    add_library(imgui-backend-vulkan STATIC)
    target_include_directories(imgui-backend-vulkan PRIVATE "C:/VulkanSDK/1.4.335.0/Include")
    target_link_directories(imgui-backend-vulkan PRIVATE "C:/VulkanSDK/1.4.335.0/Lib")
    target_compile_features(imgui-backend-vulkan PRIVATE cxx_std_20)
    target_link_libraries(imgui-backend-vulkan PUBLIC imgui)
    target_sources(imgui-backend-vulkan
        PRIVATE
            imgui/backends/imgui_impl_vulkan.cpp
        PUBLIC FILE_SET HEADERS BASE_DIRS imgui/backends FILES
            imgui/backends/imgui_impl_vulkan.h
    )
endif()

if(${IMGUI_BUILD_BACKEND_GLFW})
    add_library(imgui-backend-glfw STATIC)
    target_compile_features(imgui-backend-glfw PRIVATE cxx_std_20)
    target_link_libraries(imgui-backend-glfw PUBLIC imgui glfw)
    target_sources(imgui-backend-glfw
        PRIVATE
            imgui/backends/imgui_impl_glfw.cpp
        PUBLIC FILE_SET HEADERS BASE_DIRS imgui/backends FILES
            imgui/backends/imgui_impl_glfw.h
    )
endif()
