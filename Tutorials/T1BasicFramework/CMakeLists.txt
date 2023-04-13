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

# Source files
set(T1BasicFrameWork_PROJECT_SOURCE  "./src/BasicFrameWork.cpp" )

add_executable(T1BasicFrameWork ${T1BasicFrameWork_PROJECT_SOURCE} ${T1BasicFrameWork_SHADER_SOURCE})

target_link_libraries(T1BasicFrameWork PRIVATE project_options project_warnings) # link project_options/warnings

# Includes
# because T1BasicFrameWork includes are private, it uses absolute paths.
set(T1BasicFrameWork_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    CACHE STRING "")
target_include_directories(T1BasicFrameWork PRIVATE "${T1BasicFrameWork_INCLUDE_DIR}")

# Find dependencies:
set(T1BasicFrameWork_DEPENDENCIES_CONFIGURED    
    glm
    CACHE STRING "")

foreach(DEPENDENCY ${T1BasicFrameWork_DEPENDENCIES_CONFIGURED})
  find_package(${DEPENDENCY} CONFIG REQUIRED)
endforeach()

# Link dependencies
set(T1BasicFrameWork_LINKED_LIBRARIES glm::glm gimslib "d3d12.lib" "dxgi.lib")
target_link_system_libraries(T1BasicFrameWork PRIVATE ${T1BasicFrameWork_LINKED_LIBRARIES})

