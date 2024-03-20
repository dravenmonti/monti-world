project(monti-world C)
set (CMAKE_C_STANDARD 99)

cmake_minimum_required(VERSION 3.10)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_executable(monti-world main.c)

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(monti-world PUBLIC ${MATH_LIBRARY})
endif()

target_link_libraries(monti-world PRIVATE PkgConfig::GTK4)