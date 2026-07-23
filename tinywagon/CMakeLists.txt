cmake_minimum_required(VERSION 3.5...4.2)
project(tinywagon)

add_library(tinywagon)
target_sources(tinywagon PRIVATE "src/tinywagon.cpp")
target_include_directories(tinywagon PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(tinywagon PUBLIC glm glad stb_image stb_truetype)