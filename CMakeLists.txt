# Copyright (C) 2018, 2019, 2020, 2021
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

cmake_minimum_required(VERSION 3.7)
set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Project
project(tad)
set(TAD_VERSION 2.2)
set(TAD_LIBVERSION 1.0.0)
set(TAD_SOVERSION 1)

# Build options
option(TAD_BUILD_TOOL "Build the tad command line tool" ON)
option(TAD_BUILD_TOOL_MANPAGE "Build the manual page for the tad command line tool (requires pandoc)" ON)
option(TAD_BUILD_DOCUMENTATION "Build API reference documentation (requires Doxygen)" OFF)
option(TAD_STATIC "Build static libtad" OFF)
if(TAD_STATIC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

# Disable optional build features if required packages are not available
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
else()
    set(TAD_BUILD_DOCUMENTATION OFF)
endif()
find_package(PANDOC QUIET)
if(PANDOC_FOUND)
else()
    set(TAD_BUILD_TOOL_MANPAGE OFF)
endif()

# The TAD library (headers only)
install(FILES
	core/taglist.hpp
	core/array.hpp
	core/foreach.hpp
	core/operators.hpp
	core/io.hpp
	DESTINATION include/tad)

# Compiler and system
set(CMAKE_CXX_STANDARD 17)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()
if(MSVC)
    add_definitions(-Dftello=_ftelli64)
    add_definitions(-Dfseeko=_fseeki64)
    set(TAD_BUILD_TOOL OFF) # because there is no getopt.h in MSVC
    set(TAD_BUILD_TOOL_MANPAGE OFF)
endif()

# Optional libraries for input/output modules
find_package(GTA QUIET)
find_package(OpenEXR QUIET)
find_package(PFS QUIET)
find_package(PNG QUIET)
find_package(JPEG QUIET)
find_package(EXIV2 QUIET)
find_package(HDF5 COMPONENTS CXX QUIET)
find_package(MATIO QUIET)
find_package(TIFF QUIET)
find_package(FFMPEG QUIET)
find_package(DCMTK QUIET)
find_package(GDAL QUIET)
find_package(CFITSIO QUIET)
find_package(POPPLER QUIET)

# Optional libraries for the command line tool
find_package(MUPARSER QUIET)

# The input/output library
include_directories(${CMAKE_SOURCE_DIR}/core)
add_definitions(-DTAD_VERSION="${TAD_VERSION}")
set(libtad_SOURCES
	core/taglist.hpp
	core/array.hpp
	core/foreach.hpp
	core/operators.hpp
	core/io.hpp
	io/io.cpp
	io/io-utils.hpp
	io/io-tad.hpp io/io-tad.cpp
	io/io-csv.hpp io/io-csv.cpp
	io/io-raw.hpp io/io-raw.cpp
	io/io-pnm.hpp io/io-pnm.cpp
        io/io-rgbe.hpp io/io-rgbe.cpp
	io/io-exiv2.hpp
	io/dl.hpp)
if(TAD_STATIC)
    add_definitions(-DTAD_STATIC)
    set(libtad_STATIC_EXTRA_SOURCES "")
    set(libtad_STATIC_EXTRA_LIBRARIES "")
    if(GTA_FOUND)
	add_definitions(-DTAD_WITH_GTA)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-gta.hpp io/io-gta.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${GTA_LIBRARIES})
	include_directories(${GTA_INCLUDE_DIRS})
    endif()
    if(OPENEXR_FOUND)
	add_definitions(-DTAD_WITH_OPENEXR)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-exr.hpp io/io-exr.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${OPENEXR_LIBRARIES})
	include_directories(${OPENEXR_INCLUDE_DIRS})
    endif()
    if(PFS_FOUND)
	add_definitions(-DTAD_WITH_PFS)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-pfs.hpp io/io-pfs.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${PFS_LIBRARIES})
	include_directories(${PFS_INCLUDE_DIRS})
    endif()
    if(PNG_FOUND)
	add_definitions(-DTAD_WITH_PNG ${PNG_DEFINITIONS})
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-png.hpp io/io-png.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${PNG_LIBRARIES})
	include_directories(${PNG_INCLUDE_DIRS})
        if(EXIV2_FOUND)
	    add_definitions(-DTAD_WITH_EXIV2)
	    set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${EXIV2_LIBRARIES})
	    include_directories(${EXIV2_INCLUDE_DIR})
        endif()
    endif()
    if(JPEG_FOUND)
	add_definitions(-DTAD_WITH_JPEG)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-jpeg.hpp io/io-jpeg.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${JPEG_LIBRARIES})
	include_directories(${JPEG_INCLUDE_DIR})
        if(EXIV2_FOUND)
	    add_definitions(-DTAD_WITH_EXIV2)
	    set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${EXIV2_LIBRARIES})
	    include_directories(${EXIV2_INCLUDE_DIR})
        endif()
    endif()
    if(HDF5_FOUND)
	add_definitions(-DTAD_WITH_HDF5 ${HDF5_CXX_DEFINITIONS})
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-hdf5.hpp io/io-hdf5.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${HDF5_CXX_LIBRARIES})
	include_directories(${HDF5_CXX_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    endif()
    if(MATIO_FOUND)
	add_definitions(-DTAD_WITH_MATIO)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-mat.hpp io/io-mat.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${MATIO_LIBRARIES})
	include_directories(${MATIO_INCLUDE_DIRS})
    endif()
    if(TIFF_FOUND)
	add_definitions(-DTAD_WITH_TIFF)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-tiff.hpp io/io-tiff.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${TIFF_LIBRARIES})
	include_directories(${TIFF_INCLUDE_DIRS})
    endif()
    if(FFMPEG_FOUND)
	add_definitions(-DTAD_WITH_FFMPEG)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-ffmpeg.hpp io/io-ffmpeg.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${FFMPEG_LIBRARIES})
	include_directories(${FFMPEG_INCLUDE_DIRS})
    endif()
    if(DCMTK_FOUND)
	add_definitions(-DTAD_WITH_DCMTK)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-dcmtk.hpp io/io-dcmtk.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${DCMTK_LIBRARIES})
	include_directories(${DCMTK_INCLUDE_DIRS})
    endif()
    if(GDAL_FOUND)
	add_definitions(-DTAD_WITH_GDAL)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-gdal.hpp io/io-gdal.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${GDAL_LIBRARIES})
	include_directories(${GDAL_INCLUDE_DIRS})
    endif()
    if(CFITSIO_FOUND)
	add_definitions(-DTAD_WITH_CFITSIO)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-fits.hpp io/io-fits.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${CFITSIO_LIBRARIES})
	include_directories(${CFITSIO_INCLUDE_DIRS})
    endif()
    if(POPPLER_FOUND)
	add_definitions(-DTAD_WITH_POPPLER)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io/io-pdf.hpp io/io-pdf.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${POPPLER_LIBRARIES})
	include_directories(${POPPLER_INCLUDE_DIRS})
    endif()
    add_library(libtad STATIC ${libtad_SOURCES} ${libtad_STATIC_EXTRA_SOURCES})
    target_link_libraries(libtad ${libtad_STATIC_EXTRA_LIBRARIES} "-static")
else()
    add_library(libtad SHARED ${libtad_SOURCES})
    if(UNIX)
        target_link_libraries(libtad dl)
    endif()
    if(GTA_FOUND)
	add_library(libtadio-gta SHARED io/io-gta.hpp io/io-gta.cpp)
	set_target_properties(libtadio-gta PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtadio-gta PROPERTIES OUTPUT_NAME tadio-gta)
	include_directories(${GTA_INCLUDE_DIRS})
	target_link_libraries(libtadio-gta ${GTA_LIBRARIES})
	install(TARGETS libtadio-gta
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})
    endif()
    if(OPENEXR_FOUND)
	add_library(libtadio-exr SHARED io/io-exr.hpp io/io-exr.cpp)
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
	add_library(libtadio-pfs SHARED io/io-pfs.hpp io/io-pfs.cpp)
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
	add_library(libtadio-png SHARED io/io-png.hpp io/io-png.cpp)
	set_target_properties(libtadio-png PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtadio-png PROPERTIES OUTPUT_NAME tadio-png)
	add_definitions(${PNG_DEFINITIONS})
	include_directories(${PNG_INCLUDE_DIRS})
	target_link_libraries(libtadio-png ${PNG_LIBRARIES})
	install(TARGETS libtadio-png
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})
        if(EXIV2_FOUND)
	    add_definitions(-DTAD_WITH_EXIV2)
	    include_directories(${EXIV2_INCLUDE_DIR})
	    target_link_libraries(libtadio-png ${EXIV2_LIBRARIES})
        endif()
    endif()
    if(JPEG_FOUND)
	add_library(libtadio-jpeg SHARED io/io-jpeg.hpp io/io-jpeg.cpp)
	set_target_properties(libtadio-jpeg PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtadio-jpeg PROPERTIES OUTPUT_NAME tadio-jpeg)
	include_directories(${JPEG_INCLUDE_DIR})
	target_link_libraries(libtadio-jpeg ${JPEG_LIBRARIES})
	install(TARGETS libtadio-jpeg
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})
        if(EXIV2_FOUND)
	    add_definitions(-DTAD_WITH_EXIV2)
	    include_directories(${EXIV2_INCLUDE_DIR})
	    target_link_libraries(libtadio-jpeg ${EXIV2_LIBRARIES})
        endif()
    endif()
    if(HDF5_FOUND)
        add_library(libtadio-hdf5 SHARED io/io-hdf5.hpp io/io-hdf5.cpp)
	set_target_properties(libtadio-hdf5 PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtadio-hdf5 PROPERTIES OUTPUT_NAME tadio-hdf5)
	add_definitions(${HDF5_CXX_DEFINITIONS})
	include_directories(${HDF5_CXX_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
	target_link_libraries(libtadio-hdf5 ${HDF5_CXX_LIBRARIES})
	install(TARGETS libtadio-hdf5
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(MATIO_FOUND)
        add_library(libtadio-mat SHARED io/io-mat.hpp io/io-mat.cpp)
	set_target_properties(libtadio-mat PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtadio-mat PROPERTIES OUTPUT_NAME tadio-mat)
	include_directories(${MATIO_INCLUDE_DIRS})
	target_link_libraries(libtadio-mat ${MATIO_LIBRARIES})
	install(TARGETS libtadio-mat
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(TIFF_FOUND)
        add_library(libtadio-tiff SHARED io/io-tiff.hpp io/io-tiff.cpp)
	set_target_properties(libtadio-tiff PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtadio-tiff PROPERTIES OUTPUT_NAME tadio-tiff)
	include_directories(${TIFF_INCLUDE_DIRS})
	target_link_libraries(libtadio-tiff ${TIFF_LIBRARIES})
	install(TARGETS libtadio-tiff
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(FFMPEG_FOUND)
        add_library(libtadio-ffmpeg SHARED io/io-ffmpeg.hpp io/io-ffmpeg.cpp)
	set_target_properties(libtadio-ffmpeg PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtadio-ffmpeg PROPERTIES OUTPUT_NAME tadio-ffmpeg)
	include_directories(${FFMPEG_INCLUDE_DIRS})
	target_link_libraries(libtadio-ffmpeg ${FFMPEG_LIBRARIES})
	install(TARGETS libtadio-ffmpeg
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(DCMTK_FOUND)
        add_library(libtadio-dcmtk SHARED io/io-dcmtk.hpp io/io-dcmtk.cpp)
	set_target_properties(libtadio-dcmtk PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtadio-dcmtk PROPERTIES OUTPUT_NAME tadio-dcmtk)
	include_directories(${DCMTK_INCLUDE_DIRS})
	target_link_libraries(libtadio-dcmtk ${DCMTK_LIBRARIES})
	install(TARGETS libtadio-dcmtk
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(GDAL_FOUND)
        add_library(libtadio-gdal SHARED io/io-gdal.hpp io/io-gdal.cpp)
	set_target_properties(libtadio-gdal PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtadio-gdal PROPERTIES OUTPUT_NAME tadio-gdal)
	include_directories(${GDAL_INCLUDE_DIRS})
	target_link_libraries(libtadio-gdal ${GDAL_LIBRARIES})
	install(TARGETS libtadio-gdal
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(CFITSIO_FOUND)
        add_library(libtadio-fits SHARED io/io-fits.hpp io/io-fits.cpp)
	set_target_properties(libtadio-fits PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtadio-fits PROPERTIES OUTPUT_NAME tadio-fits)
	include_directories(${CFITSIO_INCLUDE_DIRS})
	target_link_libraries(libtadio-fits ${CFITSIO_LIBRARIES})
	install(TARGETS libtadio-fits
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(POPPLER_FOUND)
        add_library(libtadio-pdf SHARED io/io-pdf.hpp io/io-pdf.cpp)
	set_target_properties(libtadio-pdf PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtadio-pdf PROPERTIES OUTPUT_NAME tadio-pdf)
	include_directories(${POPPLER_INCLUDE_DIRS})
	target_link_libraries(libtadio-pdf ${POPPLER_LIBRARIES})
	install(TARGETS libtadio-pdf
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
endif()
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

# Optional target: library reference documentation
if(TAD_BUILD_DOCUMENTATION)
  configure_file("${CMAKE_SOURCE_DIR}/Doxyfile.in" "${CMAKE_BINARY_DIR}/Doxyfile" @ONLY)
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/html")
  add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/html/index.html"
    COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/Doxyfile"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS "${CMAKE_SOURCE_DIR}/Doxyfile.in"
	    "${CMAKE_SOURCE_DIR}/core/array.hpp"
	    "${CMAKE_SOURCE_DIR}/core/io.hpp"
	    "${CMAKE_SOURCE_DIR}/core/taglist.hpp"
	    "${CMAKE_SOURCE_DIR}/core/foreach.hpp"
	    "${CMAKE_SOURCE_DIR}/core/operators.hpp"
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
  add_custom_target(doc ALL DEPENDS "${CMAKE_BINARY_DIR}/html/index.html")
  install(DIRECTORY "${CMAKE_BINARY_DIR}/html" DESTINATION share/doc/libtad)
endif()

# The tool
if(TAD_BUILD_TOOL)
    add_executable(tad util/tad.cpp util/cmdline.hpp util/cmdline.cpp)
    target_link_libraries(tad libtad)
    if(MUPARSER_FOUND)
        add_definitions(-DTAD_WITH_MUPARSER)
        include_directories(${MUPARSER_INCLUDE_DIRS})
        link_directories(${MUPARSER_LIBRARY_DIRS})
        target_link_libraries(tad ${MUPARSER_LIBRARIES})
    endif()
    install(TARGETS tad RUNTIME DESTINATION bin)
endif()
if(TAD_BUILD_TOOL_MANPAGE)
    add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/tad.1"
        COMMAND ${PANDOC_EXECUTABLE} "-s" "-t" "man" "${CMAKE_SOURCE_DIR}/doc/tad-manual.txt" "-o" "${CMAKE_BINARY_DIR}/tad.1"
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	DEPENDS "${CMAKE_SOURCE_DIR}/doc/tad-manual.txt"
	COMMENT "Generating man page with pandoc" VERBATIM)
    add_custom_target(manpage ALL DEPENDS "${CMAKE_BINARY_DIR}/tad.1")
    install(FILES "${CMAKE_BINARY_DIR}/tad.1" DESTINATION share/man/man1)
endif()

# Testing
enable_testing()
add_executable(test-basic tests/test-basic.cpp)
target_link_libraries(test-basic libtad)
add_test(test-basic test-basic)
if(TAD_BUILD_TOOL)
    if(GTA_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_GTA")
    endif()
    if(OPENEXR_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_OPENEXR")
    endif()
    if(PFS_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_PFS")
    endif()
    if(PNG_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_PNG")
    endif()
    if(JPEG_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_JPEG")
    endif()
    if(HDF5_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_HDF5")
    endif()
    if(MATIO_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_MATIO")
    endif()
    if(TIFF_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_TIFF")
    endif()
    if(FFMPEG_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_FFMPEG")
    endif()
    if(DCMTK_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_DCMTK")
    endif()
    if(GDAL_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_GDAL")
    endif()
    if(CFITSIO_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_CFITSIO")
    endif()
    if(POPPLER_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_POPPLER")
    endif()
    if(MUPARSER_FOUND)
        list(APPEND TAD_TOOL_TEST_FLAGS "WITH_MUPARSER")
    endif()
    list(JOIN TAD_TOOL_TEST_FLAGS " " TAD_TOOL_TEST_FLAGS)
    add_test(test-tad ${CMAKE_SOURCE_DIR}/tests/tad-tool.sh "${TAD_TOOL_TEST_FLAGS}")
endif()
