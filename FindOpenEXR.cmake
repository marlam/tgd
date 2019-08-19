# Copyright (C) 2019
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(OPENEXR_INCLUDE_DIR
    OpenEXR/half.h
HINTS
    "${OPENEXR_DIR}"
    "$ENV{OPENEXR_DIR}"
PATH_SUFFIXES
    include/
DOC
    "OpenEXR headers path"
)

foreach(OPENEXR_LIB Half Iex Imath IlmImf IlmThread)
    find_library(OPENEXR_${OPENEXR_LIB}_LIBRARY
	NAMES
	    ${OPENEXR_LIB}
	HINTS
	    "${OPENEXR_DIR}"
	    "$ENV{OPENEXR_DIR}"
	PATH_SUFFIXES
	    lib/
	DOC
	    "OpenEXR ${OPENEXR_LIB} library path"
    )
    if(OPENEXR_${OPENEXR_LIB}_LIBRARY)
	list(APPEND OPENEXR_LIBRARY_LIST ${OPENEXR_${OPENEXR_LIB}_LIBRARY})
    endif()
endforeach()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenEXR
    REQUIRED_VARS OPENEXR_LIBRARY_LIST OPENEXR_INCLUDE_DIR
)

IF(OPENEXR_FOUND)
    SET(OPENEXR_LIBRARIES ${OPENEXR_LIBRARY_LIST})
    SET(OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR}/OpenEXR)
ENDIF()
