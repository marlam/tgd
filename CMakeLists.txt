# Copyright (C) 2018, 2019, 2020, 2021, 2022
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
project(tgd)
set(TGD_VERSION 2.4)
set(TGD_LIBVERSION 2.0.0)
set(TGD_SOVERSION 2)

# Build options
option(TGD_BUILD_TOOL "Build the tgd command line tool" ON)
option(TGD_BUILD_TOOL_MANPAGE "Build the manual page for the tgd command line tool (requires pandoc)" ON)
option(TGD_BUILD_DOCUMENTATION "Build API reference documentation (requires Doxygen)" OFF)
option(TGD_STATIC "Build static libtgd" OFF)
if(TGD_STATIC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

# Disable optional build features if required packages are not available
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
else()
    set(TGD_BUILD_DOCUMENTATION OFF)
endif()
find_package(PANDOC QUIET)
if(PANDOC_FOUND)
else()
    set(TGD_BUILD_TOOL_MANPAGE OFF)
endif()

# The TGD library (headers only)
install(FILES
	core/taglist.hpp
	core/array.hpp
	core/foreach.hpp
	core/operators.hpp
	core/io.hpp
	DESTINATION include/tgd)

# Compiler and system
set(CMAKE_CXX_STANDARD 17)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()
if(MSVC)
    add_definitions(-Dftello=_ftelli64)
    add_definitions(-Dfseeko=_fseeki64)
    set(TGD_BUILD_TOOL OFF) # because there is no getopt.h in MSVC
    set(TGD_BUILD_TOOL_MANPAGE OFF)
endif()

# Optional libraries for input/output modules
find_package(GTA QUIET)
find_package(OpenEXR QUIET)
if(OpenEXR_FOUND)
  # Version 3.x breaks stuff like this:
  # /usr/include/OpenEXR/ImfHeader.h:22:10: fatal error: ImathVec.h: No such file or directory
  # We must add the Imath include directory ourselves as far as I can see...
  FIND_PATH(IMATH_INCLUDE_DIR NAMES Imath/ImathVec.h)
  include_directories(${IMATH_INCLUDE_DIR}/Imath)
endif()
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
find_package(ImageMagick COMPONENTS Magick++ QUIET)

# Optional libraries for the command line tool
find_package(MUPARSER QUIET)

# The input/output library
include_directories(${CMAKE_SOURCE_DIR}/core)
add_definitions(-DTGD_VERSION="${TGD_VERSION}")
set(LIBTGD_SOURCES
	core/taglist.hpp
	core/array.hpp
	core/foreach.hpp
	core/operators.hpp
	core/io.hpp
	io/io.cpp
	io/io-utils.hpp
	io/io-tgd.hpp io/io-tgd.cpp
	io/io-csv.hpp io/io-csv.cpp
	io/io-raw.hpp io/io-raw.cpp
	io/io-pnm.hpp io/io-pnm.cpp
	io/io-rgbe.hpp io/io-rgbe.cpp
	io/io-stb.hpp io/io-stb.cpp
	io/io-tinyexr.hpp io/io-tinyexr.cpp
	io/io-exiv2.hpp
	io/dl.hpp
	ext/stb_image.h
	ext/stb_image_write.h
	ext/tinyexr.h)
if(TGD_STATIC)
    add_definitions(-DTGD_STATIC)
    set(LIBTGD_STATIC_EXTRA_SOURCES "")
    set(LIBTGD_STATIC_EXTRA_LIBRARIES "")
    if(GTA_FOUND)
	add_definitions(-DTGD_WITH_GTA)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-gta.hpp io/io-gta.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${GTA_LIBRARIES})
	include_directories(${GTA_INCLUDE_DIRS})
    endif()
    if(OpenEXR_FOUND)
	add_definitions(-DTGD_WITH_OPENEXR)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-exr.hpp io/io-exr.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${OpenEXR_LIBRARIES})
	include_directories(${OpenEXR_INCLUDE_DIRS} "${OpenEXR_INCLUDE_DIRS}/Imath")
    endif()
    if(PFS_FOUND)
	add_definitions(-DTGD_WITH_PFS)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-pfs.hpp io/io-pfs.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${PFS_LIBRARIES})
	include_directories(${PFS_INCLUDE_DIRS})
    endif()
    if(PNG_FOUND)
	add_definitions(-DTGD_WITH_PNG ${PNG_DEFINITIONS})
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-png.hpp io/io-png.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${PNG_LIBRARIES})
	include_directories(${PNG_INCLUDE_DIRS})
        if(EXIV2_FOUND)
	    add_definitions(-DTGD_WITH_EXIV2)
	    set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${EXIV2_LIBRARIES})
	    include_directories(${EXIV2_INCLUDE_DIR})
        endif()
    endif()
    if(JPEG_FOUND)
	add_definitions(-DTGD_WITH_JPEG)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-jpeg.hpp io/io-jpeg.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${JPEG_LIBRARIES})
	include_directories(${JPEG_INCLUDE_DIR})
        if(EXIV2_FOUND)
	    add_definitions(-DTGD_WITH_EXIV2)
	    set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${EXIV2_LIBRARIES})
	    include_directories(${EXIV2_INCLUDE_DIR})
        endif()
    endif()
    if(HDF5_FOUND)
	add_definitions(-DTGD_WITH_HDF5 ${HDF5_CXX_DEFINITIONS})
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-hdf5.hpp io/io-hdf5.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${HDF5_CXX_LIBRARIES})
	include_directories(${HDF5_CXX_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    endif()
    if(MATIO_FOUND)
	add_definitions(-DTGD_WITH_MATIO)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-mat.hpp io/io-mat.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${MATIO_LIBRARIES})
	include_directories(${MATIO_INCLUDE_DIRS})
    endif()
    if(TIFF_FOUND)
	add_definitions(-DTGD_WITH_TIFF)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-tiff.hpp io/io-tiff.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${TIFF_LIBRARIES})
	include_directories(${TIFF_INCLUDE_DIRS})
    endif()
    if(FFMPEG_FOUND)
	add_definitions(-DTGD_WITH_FFMPEG)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-ffmpeg.hpp io/io-ffmpeg.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${FFMPEG_LIBRARIES})
	include_directories(${FFMPEG_INCLUDE_DIRS})
    endif()
    if(DCMTK_FOUND)
	add_definitions(-DTGD_WITH_DCMTK)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-dcmtk.hpp io/io-dcmtk.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${DCMTK_LIBRARIES})
	include_directories(${DCMTK_INCLUDE_DIRS})
    endif()
    if(GDAL_FOUND)
	add_definitions(-DTGD_WITH_GDAL)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-gdal.hpp io/io-gdal.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${GDAL_LIBRARIES})
	include_directories(${GDAL_INCLUDE_DIRS})
    endif()
    if(CFITSIO_FOUND)
	add_definitions(-DTGD_WITH_CFITSIO)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-fits.hpp io/io-fits.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${CFITSIO_LIBRARIES})
	include_directories(${CFITSIO_INCLUDE_DIRS})
    endif()
    if(POPPLER_FOUND)
	add_definitions(-DTGD_WITH_POPPLER)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-pdf.hpp io/io-pdf.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${POPPLER_LIBRARIES})
	include_directories(${POPPLER_INCLUDE_DIRS})
    endif()
    if(ImageMagick_FOUND)
	add_definitions(-DTGD_WITH_MAGICK)
	set(LIBTGD_STATIC_EXTRA_SOURCES ${LIBTGD_STATIC_EXTRA_SOURCES} io/io-magick.hpp io/io-magick.cpp)
	set(LIBTGD_STATIC_EXTRA_LIBRARIES ${LIBTGD_STATIC_EXTRA_LIBRARIES} ${ImageMagick_LIBRARIES})
	include_directories(${ImageMagick_INCLUDE_DIRS})
    endif()
    add_library(libtgd STATIC ${LIBTGD_SOURCES} ${LIBTGD_STATIC_EXTRA_SOURCES})
    target_link_libraries(libtgd ${LIBTGD_STATIC_EXTRA_LIBRARIES} "-static")
else()
    add_library(libtgd SHARED ${LIBTGD_SOURCES})
    if(UNIX)
        target_link_libraries(libtgd dl)
    endif()
    if(GTA_FOUND)
	add_library(libtgdio-gta SHARED io/io-gta.hpp io/io-gta.cpp)
	set_target_properties(libtgdio-gta PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-gta PROPERTIES OUTPUT_NAME tgdio-gta)
	include_directories(${GTA_INCLUDE_DIRS})
	target_link_libraries(libtgdio-gta ${GTA_LIBRARIES})
	install(TARGETS libtgdio-gta
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})
    endif()
    if(OpenEXR_FOUND)
	add_library(libtgdio-exr SHARED io/io-exr.hpp io/io-exr.cpp)
	set_target_properties(libtgdio-exr PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-exr PROPERTIES OUTPUT_NAME tgdio-exr)
	include_directories(${OpenEXR_INCLUDE_DIRS})
	target_link_libraries(libtgdio-exr ${OpenEXR_LIBRARIES})
	install(TARGETS libtgdio-exr
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})
    endif()
    if(PFS_FOUND)
	add_library(libtgdio-pfs SHARED io/io-pfs.hpp io/io-pfs.cpp)
	set_target_properties(libtgdio-pfs PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-pfs PROPERTIES OUTPUT_NAME tgdio-pfs)
	include_directories(${PFS_INCLUDE_DIRS})
	target_link_libraries(libtgdio-pfs ${PFS_LIBRARIES})
	install(TARGETS libtgdio-pfs
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})
    endif()
    if(PNG_FOUND)
	add_library(libtgdio-png SHARED io/io-png.hpp io/io-png.cpp)
	set_target_properties(libtgdio-png PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-png PROPERTIES OUTPUT_NAME tgdio-png)
	add_definitions(${PNG_DEFINITIONS})
	include_directories(${PNG_INCLUDE_DIRS})
	target_link_libraries(libtgdio-png ${PNG_LIBRARIES})
	install(TARGETS libtgdio-png
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})
        if(EXIV2_FOUND)
	    add_definitions(-DTGD_WITH_EXIV2)
	    include_directories(${EXIV2_INCLUDE_DIR})
	    target_link_libraries(libtgdio-png ${EXIV2_LIBRARIES})
        endif()
    endif()
    if(JPEG_FOUND)
	add_library(libtgdio-jpeg SHARED io/io-jpeg.hpp io/io-jpeg.cpp)
	set_target_properties(libtgdio-jpeg PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-jpeg PROPERTIES OUTPUT_NAME tgdio-jpeg)
	include_directories(${JPEG_INCLUDE_DIR})
	target_link_libraries(libtgdio-jpeg ${JPEG_LIBRARIES})
	install(TARGETS libtgdio-jpeg
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})
        if(EXIV2_FOUND)
	    add_definitions(-DTGD_WITH_EXIV2)
	    include_directories(${EXIV2_INCLUDE_DIR})
	    target_link_libraries(libtgdio-jpeg ${EXIV2_LIBRARIES})
        endif()
    endif()
    if(HDF5_FOUND)
        add_library(libtgdio-hdf5 SHARED io/io-hdf5.hpp io/io-hdf5.cpp)
	set_target_properties(libtgdio-hdf5 PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-hdf5 PROPERTIES OUTPUT_NAME tgdio-hdf5)
	add_definitions(${HDF5_CXX_DEFINITIONS})
	include_directories(${HDF5_CXX_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
	target_link_libraries(libtgdio-hdf5 ${HDF5_CXX_LIBRARIES})
	install(TARGETS libtgdio-hdf5
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(MATIO_FOUND)
        add_library(libtgdio-mat SHARED io/io-mat.hpp io/io-mat.cpp)
	set_target_properties(libtgdio-mat PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-mat PROPERTIES OUTPUT_NAME tgdio-mat)
	include_directories(${MATIO_INCLUDE_DIRS})
	target_link_libraries(libtgdio-mat ${MATIO_LIBRARIES})
	install(TARGETS libtgdio-mat
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(TIFF_FOUND)
        add_library(libtgdio-tiff SHARED io/io-tiff.hpp io/io-tiff.cpp)
	set_target_properties(libtgdio-tiff PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-tiff PROPERTIES OUTPUT_NAME tgdio-tiff)
	include_directories(${TIFF_INCLUDE_DIRS})
	target_link_libraries(libtgdio-tiff ${TIFF_LIBRARIES})
	install(TARGETS libtgdio-tiff
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(FFMPEG_FOUND)
        add_library(libtgdio-ffmpeg SHARED io/io-ffmpeg.hpp io/io-ffmpeg.cpp)
	set_target_properties(libtgdio-ffmpeg PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-ffmpeg PROPERTIES OUTPUT_NAME tgdio-ffmpeg)
	include_directories(${FFMPEG_INCLUDE_DIRS})
	target_link_libraries(libtgdio-ffmpeg ${FFMPEG_LIBRARIES})
	install(TARGETS libtgdio-ffmpeg
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(DCMTK_FOUND)
        add_library(libtgdio-dcmtk SHARED io/io-dcmtk.hpp io/io-dcmtk.cpp)
	set_target_properties(libtgdio-dcmtk PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-dcmtk PROPERTIES OUTPUT_NAME tgdio-dcmtk)
	include_directories(${DCMTK_INCLUDE_DIRS})
	target_link_libraries(libtgdio-dcmtk ${DCMTK_LIBRARIES})
	install(TARGETS libtgdio-dcmtk
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(GDAL_FOUND)
        add_library(libtgdio-gdal SHARED io/io-gdal.hpp io/io-gdal.cpp)
	set_target_properties(libtgdio-gdal PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-gdal PROPERTIES OUTPUT_NAME tgdio-gdal)
	include_directories(${GDAL_INCLUDE_DIRS})
	target_link_libraries(libtgdio-gdal ${GDAL_LIBRARIES})
	install(TARGETS libtgdio-gdal
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(CFITSIO_FOUND)
        add_library(libtgdio-fits SHARED io/io-fits.hpp io/io-fits.cpp)
	set_target_properties(libtgdio-fits PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-fits PROPERTIES OUTPUT_NAME tgdio-fits)
	include_directories(${CFITSIO_INCLUDE_DIRS})
	target_link_libraries(libtgdio-fits ${CFITSIO_LIBRARIES})
	install(TARGETS libtgdio-fits
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(POPPLER_FOUND)
        add_library(libtgdio-pdf SHARED io/io-pdf.hpp io/io-pdf.cpp)
	set_target_properties(libtgdio-pdf PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-pdf PROPERTIES OUTPUT_NAME tgdio-pdf)
	include_directories(${POPPLER_INCLUDE_DIRS})
	target_link_libraries(libtgdio-pdf ${POPPLER_LIBRARIES})
	install(TARGETS libtgdio-pdf
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
    if(ImageMagick_FOUND)
        add_library(libtgdio-magick SHARED io/io-magick.hpp io/io-magick.cpp)
	set_target_properties(libtgdio-magick PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	set_target_properties(libtgdio-magick PROPERTIES OUTPUT_NAME tgdio-magick)
	include_directories(${ImageMagick_INCLUDE_DIRS})
	target_link_libraries(libtgdio-magick ${ImageMagick_LIBRARIES})
	install(TARGETS libtgdio-magick
	    RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib${LIB_SUFFIX}
	    ARCHIVE DESTINATION lib${LIB_SUFFIX})

    endif()
endif()
set_target_properties(libtgd PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set_target_properties(libtgd PROPERTIES OUTPUT_NAME tgd)
set_target_properties(libtgd PROPERTIES VERSION ${TGD_LIBVERSION})
set_target_properties(libtgd PROPERTIES SOVERSION ${TGD_SOVERSION})
install(TARGETS libtgd
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
include(CMakePackageConfigHelpers)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
configure_package_config_file(
    libtgdConfig.cmake.in ${CMAKE_BINARY_DIR}/libtgdConfig.cmake
    INSTALL_DESTINATION lib${LIB_SUFFIX}/cmake/libtgd-{TGD_VERSION}
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/libtgdConfigVersion.cmake
    VERSION ${TGD_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_BINARY_DIR}/libtgdConfig.cmake
    ${CMAKE_BINARY_DIR}/libtgdConfigVersion.cmake
    DESTINATION ${LIB_INSTALL_DIR}/cmake/libtgd-${TGD_VERSION}
)

# Optional target: library reference documentation
if(TGD_BUILD_DOCUMENTATION)
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
  install(DIRECTORY "${CMAKE_BINARY_DIR}/html" DESTINATION share/doc/libtgd)
endif()

# The tool
if(TGD_BUILD_TOOL)
    add_executable(tgd util/tgd.cpp util/cmdline.hpp util/cmdline.cpp)
    target_link_libraries(tgd libtgd)
    if(MUPARSER_FOUND)
        add_definitions(-DTGD_WITH_MUPARSER)
        include_directories(${MUPARSER_INCLUDE_DIRS})
        link_directories(${MUPARSER_LIBRARY_DIRS})
        target_link_libraries(tgd ${MUPARSER_LIBRARIES})
    endif()
    install(TARGETS tgd RUNTIME DESTINATION bin)
endif()
if(TGD_BUILD_TOOL_MANPAGE)
    add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/tgd.1"
        COMMAND ${PANDOC_EXECUTABLE} "-s" "-t" "man" "${CMAKE_SOURCE_DIR}/doc/tgd-manual.txt" "-o" "${CMAKE_BINARY_DIR}/tgd.1"
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	DEPENDS "${CMAKE_SOURCE_DIR}/doc/tgd-manual.txt"
	COMMENT "Generating man page with pandoc" VERBATIM)
    add_custom_target(manpage ALL DEPENDS "${CMAKE_BINARY_DIR}/tgd.1")
    install(FILES "${CMAKE_BINARY_DIR}/tgd.1" DESTINATION share/man/man1)
endif()

# Testing
enable_testing()
add_executable(test-basic tests/test-basic.cpp)
target_link_libraries(test-basic libtgd)
add_test(test-basic test-basic)
if(TGD_BUILD_TOOL)
    if(GTA_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_GTA")
    endif()
    if(OpenEXR_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_OPENEXR")
    endif()
    if(PFS_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_PFS")
    endif()
    if(PNG_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_PNG")
    endif()
    if(JPEG_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_JPEG")
    endif()
    if(HDF5_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_HDF5")
    endif()
    if(MATIO_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_MATIO")
    endif()
    if(TIFF_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_TIFF")
    endif()
    if(FFMPEG_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_FFMPEG")
    endif()
    if(DCMTK_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_DCMTK")
    endif()
    if(GDAL_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_GDAL")
    endif()
    if(CFITSIO_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_CFITSIO")
    endif()
    if(POPPLER_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_POPPLER")
    endif()
    if(ImageMagick_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_MAGICK")
    endif()
    if(MUPARSER_FOUND)
        list(APPEND TGD_TOOL_TEST_FLAGS "WITH_MUPARSER")
    endif()
    list(JOIN TGD_TOOL_TEST_FLAGS " " TGD_TOOL_TEST_FLAGS)
    add_test(test-tgd ${CMAKE_SOURCE_DIR}/tests/tgd-tool.sh "${TGD_TOOL_TEST_FLAGS}")
endif()
