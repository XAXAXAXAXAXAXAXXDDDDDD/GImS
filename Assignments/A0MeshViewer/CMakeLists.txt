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

# Shader files
file(GLOB_RECURSE A0MeshViewer_SHADER_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.hlsl)
source_group("Shaders" FILES ${A0MeshViewer_SHADER_SOURCE})
set_source_files_properties(${A0MeshViewer_SHADER_SOURCE} PROPERTIES VS_TOOL_OVERRIDE "None")

# Source files
set(A0MeshViewer_PROJECT_SOURCE "./src/main.cpp" "./src/TriangleApp.cpp" "./include/TriangleApp.h")

add_executable(A0MeshViewer ${A0MeshViewer_PROJECT_SOURCE} ${A0MeshViewer_SHADER_SOURCE})

target_link_libraries(A0MeshViewer PRIVATE project_options project_warnings) # link project_options/warnings

# Includes
# because A0MeshViewer includes are private, it uses absolute paths.
set(A0MeshViewer_INCLUDE_DIR
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	CACHE STRING "")
target_include_directories(A0MeshViewer PRIVATE "${A0MeshViewer_INCLUDE_DIR}")


# Find dependencies:
set(A0MeshViewer_DEPENDENCIES_CONFIGURED		   
	glm
	imgui
	CACHE STRING "")

foreach(DEPENDENCY ${A0MeshViewer_DEPENDENCIES_CONFIGURED})
  find_package(${DEPENDENCY} CONFIG REQUIRED)
endforeach()

# Link dependencies
set(A0MeshViewer_LINKED_LIBRARIES glm::glm gimslib)
target_link_system_libraries(A0MeshViewer PRIVATE ${A0MeshViewer_LINKED_LIBRARIES} "d3d12.lib" "dxgi.lib" "d3dcompiler.lib")


