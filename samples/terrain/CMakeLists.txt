cmake_minimum_required (VERSION 3.13)

set(project terrain)
set(folder "Samples/Terrain")


add_executable(${project}  "main.cpp"  "terrain.h" "app_base.h" "app_base.cpp" "terrain.cpp" "frustum.h")


target_link_libraries(${project} rhi glfw glm stb_image) 

set_target_properties(${project} PROPERTIES FOLDER ${folder})

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()
