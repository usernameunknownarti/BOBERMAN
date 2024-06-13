cmake_minimum_required(VERSION 3.27)
project(projekt)

set(CMAKE_C_STANDARD 11)

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#find_package(GLEW REQUIRED)
find_package(freeglut REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(projekt main.c)

target_link_libraries(projekt ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES}  libfreeglut.a)
