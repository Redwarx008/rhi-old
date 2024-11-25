cmake_minimum_required (VERSION 3.13)

set(project terrain)
set(folder "Samples/Terrain")


add_executable(${project}  "main.cpp"  "terrain.h" "app_base.h" "app_base.cpp" "terrain.cpp" "frustum.h" "utils.h")


target_link_libraries(${project} rhi glfw glm stb_image) 

set_target_properties(${project} PROPERTIES FOLDER ${folder})

set(PORJCET_BINARY_DIR "${EXAMPLES_BINARY_OUTPUT_DIR}/${project}")
set_target_properties(${project} 
                PROPERTIES
                FOLDER ${folder}
                RUNTIME_OUTPUT_DIRECTORY ${PORJCET_BINARY_DIR}
)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/*.comp"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/*.geom"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/*.tesc"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/*.tese"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/*.mesh"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/*.task"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/*.rgen"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/*.rchit"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/*.rmiss"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/*.rcall")

SOURCE_GROUP("shaders" FILES ${SHADER_FILES})

compile_shader(
    "${project}_SHADERS"
    "${SHADER_FILES}"
    ""
    "${PORJCET_BINARY_DIR}"
    "${GLSL_VALIDATOR}")

add_dependencies(${project} ${project}_SHADERS)