cmake_minimum_required(VERSION 3.21)
project(fixup-x C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable( fixup-x
                fixup-x.c fixup-x.h )

target_link_libraries( fixup-x PUBLIC magic )