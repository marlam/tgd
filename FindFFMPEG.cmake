# Copyright (C) 2019
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(FFMPEG_INCLUDE_DIR
    libavcodec/avcodec.h
HINTS
    "${FFMPEG_DIR}"
    "$ENV{FFMPEG_DIR}"
PATH_SUFFIXES
    include/
DOC
    "FFmpeg headers path"
)

foreach(FFMPEG_LIB avformat avcodec avutil swscale)
    find_library(FFMPEG_${FFMPEG_LIB}_LIBRARY
	NAMES
	    ${FFMPEG_LIB}
	HINTS
	    "${FFMPEG_DIR}"
	    "$ENV{FFMPEG_DIR}"
	PATH_SUFFIXES
	    lib/
	DOC
	    "FFmpeg ${FFMPEG_LIB} library path"
    )
    if(FFMPEG_${FFMPEG_LIB}_LIBRARY)
	list(APPEND FFMPEG_LIBRARY_LIST ${FFMPEG_${FFMPEG_LIB}_LIBRARY})
    endif()
endforeach()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFMPEG
    REQUIRED_VARS FFMPEG_LIBRARY_LIST FFMPEG_INCLUDE_DIR
)

IF(FFMPEG_FOUND)
    SET(FFMPEG_LIBRARIES ${FFMPEG_LIBRARY_LIST})
    SET(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
ENDIF()
