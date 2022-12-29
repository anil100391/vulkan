cmake_minimum_required(VERSION 3.13)
project(opengl CXX)
set (CMAKE_CXX_STANDARD 17)

# for youcompleteme to work
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
    include_directories ("C:/VulkanSDK/1.3.236.0/Include")
    link_directories ("C:/VulkanSDK/1.3.236.0/Lib")
else()
    find_package(Vulkan REQUIRED)
endif()

find_package(glfw3 CONFIG REQUIRED)

# include_directories ("${PROJECT_SOURCE_DIR}/vendor/imgui")
include_directories ("${PROJECT_SOURCE_DIR}/vendor/glm")
# include_directories ("${PROJECT_SOURCE_DIR}/vendor/stb_image")
# include_directories ("${PROJECT_SOURCE_DIR}/vendor/irrKlang/include")

include_directories ("${PROJECT_SOURCE_DIR}/vendor/glm")

# file (GLOB utils_src "${PROJECT_SOURCE_DIR}/utils/*.cpp" )
file (GLOB main_src "${PROJECT_SOURCE_DIR}/*.cpp" )

# undefine min/max macro from msvc minwindef.h
add_compile_options(-DNOMINMAX)

# for definition of M_PI for msvc
add_compile_options(-D_USE_MATH_DEFINES)

add_executable (app ${main_src})

if (MSVC)
    target_link_libraries (app PRIVATE glfw vulkan-1.lib)
else()
    target_link_libraries (app GL glfw GLEW vulkan)
endif()

