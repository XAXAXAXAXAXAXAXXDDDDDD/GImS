# Defintions
add_definitions(-D_UNICODE)
add_definitions(-DUNICODE)
add_definitions(-DNOMINMAX)
add_definitions(-DNODRAWTEXT)
add_definitions(-DNOBITMAP)
add_definitions(-DNOMCX)
add_definitions(-DNOSERVICE)
add_definitions(-DNOHELP)
add_definitions(-DWIN32_LEAN_AND_MEAN)

#Shader files
set(T4EllipsisApp_SHADER_SOURCE "./shaders/EllipsisApp.hlsl")
source_group("Shaders" FILES ${T4EllipsisApp_SHADER_SOURCE})
set_source_files_properties(${T4EllipsisApp_SHADER_SOURCE} PROPERTIES VS_TOOL_OVERRIDE "None")

# Source files
set(T4EllipsisApp_PROJECT_SOURCE  "./src/EllipsisApp.cpp" )

add_executable(T4EllipsisApp ${T4EllipsisApp_PROJECT_SOURCE} ${T4EllipsisApp_SHADER_SOURCE})

target_link_libraries(T4EllipsisApp PRIVATE project_options project_warnings) # link project_options/warnings

# Includes
# because T4EllipsisApp includes are private, it uses absolute paths.
set(T4EllipsisApp_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    CACHE STRING "")
target_include_directories(T4EllipsisApp PRIVATE "${T4EllipsisApp_INCLUDE_DIR}")

# Find dependencies:
set(T4EllipsisApp_DEPENDENCIES_CONFIGURED    
    glm
    CACHE STRING "")

foreach(DEPENDENCY ${T4EllipsisApp_DEPENDENCIES_CONFIGURED})
  find_package(${DEPENDENCY} CONFIG REQUIRED)
endforeach()

# Link dependencies
set(T4EllipsisApp_LINKED_LIBRARIES glm::glm gimslib "d3d12.lib" "dxgi.lib")
target_link_system_libraries(T4EllipsisApp PRIVATE ${T4EllipsisApp_LINKED_LIBRARIES})

