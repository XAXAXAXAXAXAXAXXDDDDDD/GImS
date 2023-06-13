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
file(GLOB_RECURSE P1BezierPatch_SHADER_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.hlsl)
source_group("Shaders" FILES ${P1BezierPatch_SHADER_SOURCE})
set_source_files_properties(${P1BezierPatch_SHADER_SOURCE} PROPERTIES VS_TOOL_OVERRIDE "None")

# Source files
set(P1BezierPatch_PROJECT_SOURCE "./src/main.cpp" "./src/PatchApp.cpp" "./include/PatchApp.h" "./src/BezierPatchD3D12.cpp" "./include/BezierPatchD3D12.h" "./include/MorbiusStrip.h")

add_executable(P1BezierPatch ${P1BezierPatch_PROJECT_SOURCE} ${P1BezierPatch_SHADER_SOURCE})

target_link_libraries(P1BezierPatch PRIVATE project_options project_warnings) # link project_options/warnings

# Includes
# because P1BezierPatch includes are private, it uses absolute paths.
set(P1BezierPatch_INCLUDE_DIR
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	CACHE STRING "")
target_include_directories(P1BezierPatch PRIVATE "${P1BezierPatch_INCLUDE_DIR}")


# Find dependencies:
set(P1BezierPatch_DEPENDENCIES_CONFIGURED		   
	glm
	imgui
	CACHE STRING "")

foreach(DEPENDENCY ${P1BezierPatch_DEPENDENCIES_CONFIGURED})
  find_package(${DEPENDENCY} CONFIG REQUIRED)
endforeach()

# Link dependencies
set(P1BezierPatch_LINKED_LIBRARIES glm::glm gimslib)
target_link_system_libraries(P1BezierPatch PRIVATE ${P1BezierPatch_LINKED_LIBRARIES} "d3d12.lib" "dxgi.lib" "d3dcompiler.lib")


