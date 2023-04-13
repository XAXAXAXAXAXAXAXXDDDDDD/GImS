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
set(T0GLM_PROJECT_SOURCE  "./src/GLMExample.cpp" )

add_executable(T0GLM ${T0GLM_PROJECT_SOURCE} ${T0GLM_SHADER_SOURCE})

target_link_libraries(T0GLM PRIVATE project_options project_warnings) # link project_options/warnings

# Includes
# because T0GLM includes are private, it uses absolute paths.
set(T0GLM_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    CACHE STRING "")
target_include_directories(T0GLM PRIVATE "${T0GLM_INCLUDE_DIR}")

# Find dependencies:
set(T0GLM_DEPENDENCIES_CONFIGURED    
    glm    
    CACHE STRING "")

foreach(DEPENDENCY ${T0GLM_DEPENDENCIES_CONFIGURED})
  find_package(${DEPENDENCY} CONFIG REQUIRED)
endforeach()

# Link dependencies
set(T0GLM_LINKED_LIBRARIES gimslib glm::glm)
target_link_system_libraries(T0GLM PRIVATE ${T0GLM_LINKED_LIBRARIES})

