add_library(vma INTERFACE)


#both vma and glm and header only libs so we only need the include path
target_include_directories(vma INTERFACE vma)

