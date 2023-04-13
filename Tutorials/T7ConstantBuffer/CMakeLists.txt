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
set(T7ConstantBuffer_SHADER_SOURCE "./shaders/ConstantBuffer.hlsl")
source_group("Shaders" FILES ${T7ConstantBuffer_SHADER_SOURCE})
set_source_files_properties(${T7ConstantBuffer_SHADER_SOURCE} PROPERTIES VS_TOOL_OVERRIDE "None")

# Source files
set(T7ConstantBuffer_PROJECT_SOURCE  "./src/ConstantBufferApp.cpp" )

add_executable(T7ConstantBuffer ${T7ConstantBuffer_PROJECT_SOURCE} ${T7ConstantBuffer_SHADER_SOURCE})

target_link_libraries(T7ConstantBuffer PRIVATE project_options project_warnings) # link project_options/warnings

# Includes
# because T7ConstantBuffer includes are private, it uses absolute paths.
set(T7ConstantBuffer_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    CACHE STRING "")
target_include_directories(T7ConstantBuffer PRIVATE "${T7ConstantBuffer_INCLUDE_DIR}")

# Find dependencies:
set(T7ConstantBuffer_DEPENDENCIES_CONFIGURED    
    glm
    CACHE STRING "")

foreach(DEPENDENCY ${T7ConstantBuffer_DEPENDENCIES_CONFIGURED})
  find_package(${DEPENDENCY} CONFIG REQUIRED)
endforeach()

# Link dependencies
set(T7ConstantBuffer_LINKED_LIBRARIES glm::glm gimslib "d3d12.lib" "dxgi.lib")
target_link_system_libraries(T7ConstantBuffer PRIVATE ${T7ConstantBuffer_LINKED_LIBRARIES})

