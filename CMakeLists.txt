cmake_minimum_required(VERSION 3.16)

project(archiveTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(archiveTool main.cpp
    archiver.hpp
    miniJson.hpp
    fnv1a.hpp
    endianHelpers.hpp
    zstdCtxWrapper.hpp
)

#find_package(ZSTD REQUIRED)

find_path(ZSTD_INCLUDE_DIR NAMES zstd.h)
find_library(ZSTD_LIBRARY NAMES zstd)

if (NOT ZSTD_INCLUDE_DIR OR NOT ZSTD_LIBRARY)
    message(FATAL_ERROR "Zstandard library not found! Please install libzstd-dev.")
endif()

target_link_libraries(archiveTool
    PRIVATE
    ${ZSTD_LIBRARY}
)

include(GNUInstallDirs)
install(TARGETS archiveTool
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
