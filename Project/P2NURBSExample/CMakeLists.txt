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
file(GLOB_RECURSE P2NURBSExample_SHADER_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.hlsl)
source_group("Shaders" FILES ${P2NURBSExample_SHADER_SOURCE})
set_source_files_properties(${P2NURBSExample_SHADER_SOURCE} PROPERTIES VS_TOOL_OVERRIDE "None")

# Source files
set(P2NURBSExample_PROJECT_SOURCE "./src/main.cpp" "./src/PatchApp.cpp" "./include/PatchApp.h" "./src/NURBSPatchD3D12.cpp" "./include/NURBSPatchD3D12.h" "./include/MorbiusStrip.h" "./include/Teapot.h" "./include/UnitCircle.h")

add_executable(P2NURBSExample ${P2NURBSExample_PROJECT_SOURCE} ${P2NURBSExample_SHADER_SOURCE})

target_link_libraries(P2NURBSExample PRIVATE project_options project_warnings) # link project_options/warnings

# Includes
# because P2NURBSExample includes are private, it uses absolute paths.
set(P2NURBSExample_INCLUDE_DIR
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	CACHE STRING "")
target_include_directories(P2NURBSExample PRIVATE "${P2NURBSExample_INCLUDE_DIR}")


# Find dependencies:
set(P2NURBSExample_DEPENDENCIES_CONFIGURED		   
	glm
	imgui
	CACHE STRING "")

foreach(DEPENDENCY ${P2NURBSExample_DEPENDENCIES_CONFIGURED})
  find_package(${DEPENDENCY} CONFIG REQUIRED)
endforeach()

# Link dependencies
set(P2NURBSExample_LINKED_LIBRARIES glm::glm gimslib)
target_link_system_libraries(P2NURBSExample PRIVATE ${P2NURBSExample_LINKED_LIBRARIES} "d3d12.lib" "dxgi.lib" "d3dcompiler.lib")


