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
file(GLOB_RECURSE P0TessellationExample_SHADER_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.hlsl)
source_group("Shaders" FILES ${P0TessellationExample_SHADER_SOURCE})
set_source_files_properties(${P0TessellationExample_SHADER_SOURCE} PROPERTIES VS_TOOL_OVERRIDE "None")

# Source files
set(P0TessellationExample_PROJECT_SOURCE "./src/main.cpp" "./src/PatchApp.cpp" "./include/PatchApp.h")

add_executable(P0TessellationExample ${P0TessellationExample_PROJECT_SOURCE} ${P0TessellationExample_SHADER_SOURCE})

target_link_libraries(P0TessellationExample PRIVATE project_options project_warnings) # link project_options/warnings

# Includes
# because P0TessellationExample includes are private, it uses absolute paths.
set(P0TessellationExample_INCLUDE_DIR
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	CACHE STRING "")
target_include_directories(P0TessellationExample PRIVATE "${P0TessellationExample_INCLUDE_DIR}")


# Find dependencies:
set(P0TessellationExample_DEPENDENCIES_CONFIGURED		   
	glm
	imgui
	CACHE STRING "")

foreach(DEPENDENCY ${P0TessellationExample_DEPENDENCIES_CONFIGURED})
  find_package(${DEPENDENCY} CONFIG REQUIRED)
endforeach()

# Link dependencies
set(P0TessellationExample_LINKED_LIBRARIES glm::glm gimslib)
target_link_system_libraries(P0TessellationExample PRIVATE ${P0TessellationExample_LINKED_LIBRARIES} "d3d12.lib" "dxgi.lib" "d3dcompiler.lib")


