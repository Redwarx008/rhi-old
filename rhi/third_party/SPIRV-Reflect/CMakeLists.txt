cmake_minimum_required(VERSION 3.16)

project(spirv-reflect)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 14)

add_library(spirv-reflect STATIC ${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect.h
                                     ${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect.cpp)

target_include_directories(spirv-reflect
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS spirv-reflect
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
