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
file(GLOB_RECURSE A1SceneGraphViewer_SHADER_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.hlsl)
source_group("Shaders" FILES ${A1SceneGraphViewer_SHADER_SOURCE})
set_source_files_properties(${A1SceneGraphViewer_SHADER_SOURCE} PROPERTIES VS_TOOL_OVERRIDE "None")

# Source files
set(A1SceneGraphViewer_PROJECT_SOURCE "./src/main.cpp" 
                                "./src/SceneGraphViewerApp.cpp" 
								"./src/AABB.cpp" 
								"./src/Scene.cpp" 
								"./src/SceneFactory.cpp" 
								"./src/TriangleMeshD3D12.cpp" 
								"./src/Texture2DD3D12.cpp" 
								"./src/ConstantBufferD3D12.cpp" 
								"./include/AABB.hpp" 
								"./include/Scene.hpp" 
								"./include/SceneFactory.hpp" 
								"./include/TriangleMeshD3D12.hpp" 								
								"./include/Texture2DD3D12.hpp" 								
								"./include/SceneGraphViewerApp.hpp"
								"./include/ConstantBufferD3D12.hpp")

add_executable(A1SceneGraphViewer ${A1SceneGraphViewer_PROJECT_SOURCE} ${A1SceneGraphViewer_SHADER_SOURCE})

target_link_libraries(A1SceneGraphViewer PRIVATE project_options project_warnings) # link project_options/warnings

# Includes
# because A1SceneGraphViewer includes are private, it uses absolute paths.
set(A1SceneGraphViewer_INCLUDE_DIR
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	CACHE STRING "")
target_include_directories(A1SceneGraphViewer PRIVATE "${A1SceneGraphViewer_INCLUDE_DIR}")


# Find dependencies:
set(A1SceneGraphViewer_DEPENDENCIES_CONFIGURED		   
	glm
	imgui
	assimp
	CACHE STRING "")

foreach(DEPENDENCY ${A1SceneGraphViewer_DEPENDENCIES_CONFIGURED})
  find_package(${DEPENDENCY} CONFIG REQUIRED)
endforeach()

# Link dependencies
set(A1SceneGraphViewer_LINKED_LIBRARIES assimp::assimp glm::glm gimslib)
target_link_system_libraries(A1SceneGraphViewer PRIVATE ${A1SceneGraphViewer_LINKED_LIBRARIES} "d3d12.lib" "dxgi.lib" "d3dcompiler.lib")


