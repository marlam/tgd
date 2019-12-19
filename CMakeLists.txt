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
project(tad)
set(TAD_VERSION 1.0)
set(TAD_LIBVERSION 1.0.0)
set(TAD_SOVERSION 1)

# Build options
option(TAD_BUILD_DOCUMENTATION "Build API reference documentation (requires Doxygen)" OFF)
option(TAD_STATIC "Build static libtad" OFF)
if(TAD_STATIC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

# The TAD library (headers only)
install(FILES taglist.hpp array.hpp foreach.hpp operators.hpp tools.hpp io.hpp DESTINATION include/tad)

# Compiler and system (works with gcc and clang)
set(CMAKE_CXX_STANDARD 14)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
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

# The input/output library
add_definitions(-DTAD_VERSION="${TAD_VERSION}")
set(libtad_SOURCES
	taglist.hpp
	array.hpp
	foreach.hpp
	operators.hpp
	tools.hpp
	io.hpp io.cpp io-utils.hpp
	io-tad.hpp io-tad.cpp
	io-csv.hpp io-csv.cpp
	io-raw.hpp io-raw.cpp
	io-pnm.hpp io-pnm.cpp
        io-rgbe.hpp io-rgbe.cpp
	io-exiv2.hpp
	dl.hpp)
if(TAD_STATIC)
    add_definitions(-DTAD_STATIC)
    set(libtad_STATIC_EXTRA_SOURCES "")
    set(libtad_STATIC_EXTRA_LIBRARIES "")
    if(GTA_FOUND)
	add_definitions(-DTAD_WITH_GTA)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-gta.hpp io-gta.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${GTA_LIBRARIES})
	include_directories(${GTA_INCLUDE_DIRS})
    endif()
    if(OPENEXR_FOUND)
	add_definitions(-DTAD_WITH_OPENEXR)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-exr.hpp io-exr.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${OPENEXR_LIBRARIES})
	include_directories(${OPENEXR_INCLUDE_DIRS})
    endif()
    if(PFS_FOUND)
	add_definitions(-DTAD_WITH_PFS)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-pfs.hpp io-pfs.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${PFS_LIBRARIES})
	include_directories(${PFS_INCLUDE_DIRS})
    endif()
    if(PNG_FOUND)
	add_definitions(-DTAD_WITH_PNG ${PNG_DEFINITIONS})
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-png.hpp io-png.cpp)
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
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-jpeg.hpp io-jpeg.cpp)
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
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-hdf5.hpp io-hdf5.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${HDF5_CXX_LIBRARIES})
	include_directories(${HDF5_CXX_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    endif()
    if(MATIO_FOUND)
	add_definitions(-DTAD_WITH_MATIO)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-mat.hpp io-mat.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${MATIO_LIBRARIES})
	include_directories(${MATIO_INCLUDE_DIRS})
    endif()
    if(TIFF_FOUND)
	add_definitions(-DTAD_WITH_TIFF)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-tiff.hpp io-tiff.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${TIFF_LIBRARIES})
	include_directories(${TIFF_INCLUDE_DIRS})
    endif()
    if(FFMPEG_FOUND)
	add_definitions(-DTAD_WITH_FFMPEG)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-ffmpeg.hpp io-ffmpeg.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${FFMPEG_LIBRARIES})
	include_directories(${FFMPEG_INCLUDE_DIRS})
    endif()
    if(DCMTK_FOUND)
	add_definitions(-DTAD_WITH_DCMTK)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-dcmtk.hpp io-dcmtk.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${DCMTK_LIBRARIES})
	include_directories(${DCMTK_INCLUDE_DIRS})
    endif()
    if(GDAL_FOUND)
	add_definitions(-DTAD_WITH_GDAL)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-gdal.hpp io-gdal.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${GDAL_LIBRARIES})
	include_directories(${GDAL_INCLUDE_DIRS})
    endif()
    if(CFITSIO_FOUND)
	add_definitions(-DTAD_WITH_CFITSIO)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-fits.hpp io-fits.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${CFITSIO_LIBRARIES})
	include_directories(${CFITSIO_INCLUDE_DIRS})
    endif()
    if(POPPLER_FOUND)
	add_definitions(-DTAD_WITH_POPPLER)
	set(libtad_STATIC_EXTRA_SOURCES ${libtad_STATIC_EXTRA_SOURCES} io-pdf.hpp io-pdf.cpp)
	set(libtad_STATIC_EXTRA_LIBRARIES ${libtad_STATIC_EXTRA_LIBRARIES} ${POPPLER_LIBRARIES})
	include_directories(${POPPLER_INCLUDE_DIRS})
    endif()
    add_library(libtad STATIC ${libtad_SOURCES} ${libtad_STATIC_EXTRA_SOURCES})
    target_link_libraries(libtad ${libtad_STATIC_EXTRA_LIBRARIES} "-static")
else()
    add_library(libtad SHARED ${libtad_SOURCES})
    target_link_libraries(libtad dl)
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
        if(EXIV2_FOUND)
	    add_definitions(-DTAD_WITH_EXIV2)
	    include_directories(${EXIV2_INCLUDE_DIR})
	    target_link_libraries(libtadio-png ${EXIV2_LIBRARIES})
        endif()
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
        if(EXIV2_FOUND)
	    add_definitions(-DTAD_WITH_EXIV2)
	    include_directories(${EXIV2_INCLUDE_DIR})
	    target_link_libraries(libtadio-jpeg ${EXIV2_LIBRARIES})
        endif()
    endif()
    if(HDF5_FOUND)
        add_library(libtadio-hdf5 SHARED io-hdf5.hpp io-hdf5.cpp)
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
        add_library(libtadio-mat SHARED io-mat.hpp io-mat.cpp)
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
        add_library(libtadio-tiff SHARED io-tiff.hpp io-tiff.cpp)
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
        add_library(libtadio-ffmpeg SHARED io-ffmpeg.hpp io-ffmpeg.cpp)
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
        add_library(libtadio-dcmtk SHARED io-dcmtk.hpp io-dcmtk.cpp)
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
        add_library(libtadio-gdal SHARED io-gdal.hpp io-gdal.cpp)
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
        add_library(libtadio-fits SHARED io-fits.hpp io-fits.cpp)
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
        add_library(libtadio-pdf SHARED io-pdf.hpp io-pdf.cpp)
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

# The tool
add_executable(tad tad.cpp cmdline.hpp cmdline.cpp)
target_link_libraries(tad libtad)
install(TARGETS tad RUNTIME DESTINATION bin)
