cmake_minimum_required(VERSION 3.8)
project(svdag)
add_subdirectory(deps/glad)
add_subdirectory(deps/glfw)

add_executable(precompute-dag dag.cpp precompute-dag.cpp)
target_compile_features(precompute-dag PUBLIC cxx_std_20)

add_executable(view-dag dag.cpp view-dag.cpp)
target_compile_features(precompute-dag PUBLIC cxx_std_20)
target_link_libraries(view-dag glad glfw)
