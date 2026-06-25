cmake_minimum_required(VERSION 3.15)

project(Kino VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(spdlog_DIR "C:/Dev/Libs/spdlog/build")

find_package(spdlog REQUIRED)

add_executable(Kino
    vendor/stb/stb_vendor.c
    vendor/stb/stb_image.h
    vendor/stb/stb_rect_pack.h
    vendor/stb/stb_sprintf.h
    vendor/stb/stb_truetype.h
)

add_subdirectory(src)

target_include_directories(Kino PRIVATE
    vendor
)

target_link_libraries(Kino PRIVATE spdlog::spdlog)

set_property(TARGET Kino PROPERTY RUNTIME_OUTPUT_DIRECTORY        "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set_property(TARGET Kino PROPERTY RUNTIME_OUTPUT_DIRECTORY_DEBUG  "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set_property(TARGET Kino PROPERTY VS_DEBUGGER_WORKING_DIRECTORY            "${CMAKE_CURRENT_SOURCE_DIR}")


