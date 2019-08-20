# Copyright (C) 2018, 2019
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

cmake_minimum_required(VERSION 3.5)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Project
project(libtad)
set(TAD_VERSION 0.0.0)
set(TAD_LIBVERSION 0.0.0)
set(TAD_SOVERSION 0)

# Build options
option(TAD_BUILD_DOCUMENTATION "Build API reference documentation (requires Doxygen)" OFF)

# The TAD library (header only)
install(FILES taglist.hpp array.hpp foreach.hpp operators.hpp tools.hpp io.hpp DESTINATION include/tad)

# Compiler and system (works with gcc and clang)
set(CMAKE_CXX_STANDARD 14)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# Optional libraries
find_package(GTA QUIET)
find_package(NetPBM QUIET)
find_package(OpenEXR QUIET)
find_package(PFS QUIET)
find_package(PNG QUIET)
find_package(JPEG QUIET)
#find_package(HDF5 COMPONENTS CXX QUIET)

# The TAD library (only required for input/output)
add_library(libtad SHARED
	taglist.hpp
	array.hpp
	foreach.hpp
	operators.hpp
	tools.hpp
	io.hpp io.cpp
	io-tad.hpp io-tad.cpp
	io-raw.hpp io-raw.cpp
	dl.hpp)
target_link_libraries(libtad dl)
set_target_properties(libtad PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set_target_properties(libtad PROPERTIES OUTPUT_NAME tad)
set_target_properties(libtad PROPERTIES VERSION ${TAD_LIBVERSION})
set_target_properties(libtad PROPERTIES SOVERSION ${TAD_SOVERSION})
install(TARGETS libtad
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
include(CMakePackageConfigHelpers)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
configure_package_config_file(
    libtadConfig.cmake.in ${CMAKE_BINARY_DIR}/libtadConfig.cmake
    INSTALL_DESTINATION lib${LIB_SUFFIX}/cmake/libtad-{TAD_VERSION}
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/libtadConfigVersion.cmake
    VERSION ${TAD_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_BINARY_DIR}/libtadConfig.cmake
    ${CMAKE_BINARY_DIR}/libtadConfigVersion.cmake
    DESTINATION ${LIB_INSTALL_DIR}/cmake/libtad-${TAD_VERSION}
)

# Optional importers/exporters
if(GTA_FOUND)
    add_library(libtadio-gta SHARED io-gta.hpp io-gta.cpp)
    set_target_properties(libtadio-gta PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set_target_properties(libtadio-gta PROPERTIES OUTPUT_NAME tadio-gta)
    include_directories(${GTA_INCLUDE_DIRS})
    target_link_libraries(libtadio-gta ${GTA_LIBRARIES})
    install(TARGETS libtadio-gta
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        ARCHIVE DESTINATION lib${LIB_SUFFIX})
endif()
if(NETPBM_FOUND)
    add_library(libtadio-pnm SHARED io-pnm.hpp io-pnm.cpp)
    set_target_properties(libtadio-pnm PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set_target_properties(libtadio-pnm PROPERTIES OUTPUT_NAME tadio-pnm)
    include_directories(${NETPBM_INCLUDE_DIRS})
    target_link_libraries(libtadio-pnm ${NETPBM_LIBRARIES})
    install(TARGETS libtadio-pnm
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        ARCHIVE DESTINATION lib${LIB_SUFFIX})
endif()
if(OPENEXR_FOUND)
    add_library(libtadio-exr SHARED io-exr.hpp io-exr.cpp)
    set_target_properties(libtadio-exr PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set_target_properties(libtadio-exr PROPERTIES OUTPUT_NAME tadio-exr)
    include_directories(${OPENEXR_INCLUDE_DIRS})
    target_link_libraries(libtadio-exr ${OPENEXR_LIBRARIES})
    install(TARGETS libtadio-exr
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        ARCHIVE DESTINATION lib${LIB_SUFFIX})
endif()
if(PFS_FOUND)
    add_library(libtadio-pfs SHARED io-pfs.hpp io-pfs.cpp)
    set_target_properties(libtadio-pfs PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set_target_properties(libtadio-pfs PROPERTIES OUTPUT_NAME tadio-pfs)
    include_directories(${PFS_INCLUDE_DIRS})
    target_link_libraries(libtadio-pfs ${PFS_LIBRARIES})
    install(TARGETS libtadio-pfs
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        ARCHIVE DESTINATION lib${LIB_SUFFIX})
endif()
if(PNG_FOUND)
    add_library(libtadio-png SHARED io-png.hpp io-png.cpp)
    set_target_properties(libtadio-png PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set_target_properties(libtadio-png PROPERTIES OUTPUT_NAME tadio-png)
    add_definitions(${PNG_DEFINITIONS})
    include_directories(${PNG_INCLUDE_DIRS})
    target_link_libraries(libtadio-png ${PNG_LIBRARIES})
    install(TARGETS libtadio-png
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        ARCHIVE DESTINATION lib${LIB_SUFFIX})
endif()
if(JPEG_FOUND)
    add_library(libtadio-jpeg SHARED io-jpeg.hpp io-jpeg.cpp)
    set_target_properties(libtadio-jpeg PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set_target_properties(libtadio-jpeg PROPERTIES OUTPUT_NAME tadio-jpeg)
    include_directories(${JPEG_INCLUDE_DIR})
    target_link_libraries(libtadio-jpeg ${JPEG_LIBRARIES})
    install(TARGETS libtadio-jpeg
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib${LIB_SUFFIX}
        ARCHIVE DESTINATION lib${LIB_SUFFIX})
endif()
#if(HDF5_FOUND)
#    add_definitions(-DHAVE_HDF5 ${HDF5_CXX_DEFINITIONS})
#    include_directories(${HDF5_CXX_INCLUDE_DIRS})
#    include_directories(${HDF5_INCLUDE_DIRS})
#    target_link_libraries(libtadio ${HDF5_CXX_LIBRARIES})
#endif()

# Optional target: reference documentation
if(TAD_BUILD_DOCUMENTATION)
  find_package(Doxygen REQUIRED)
  configure_file("${CMAKE_SOURCE_DIR}/Doxyfile.in" "${CMAKE_BINARY_DIR}/Doxyfile" @ONLY)
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/html")
  add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/html/index.html"
    COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/Doxyfile"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS "${CMAKE_SOURCE_DIR}/Doxyfile.in"
            "${CMAKE_SOURCE_DIR}/array.hpp"
	    "${CMAKE_SOURCE_DIR}/io.hpp"
	    "${CMAKE_SOURCE_DIR}/taglist.hpp"
	    "${CMAKE_SOURCE_DIR}/foreach.hpp"
	    "${CMAKE_SOURCE_DIR}/operators.hpp"
	    "${CMAKE_SOURCE_DIR}/tools.hpp"
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
  add_custom_target(doc ALL DEPENDS "${CMAKE_BINARY_DIR}/html/index.html")
  install(DIRECTORY "${CMAKE_BINARY_DIR}/html" DESTINATION share/doc/libtad)
endif()