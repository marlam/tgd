# Copyright (C) 2019
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(POPPLER_INCLUDE_DIR NAMES poppler/cpp/poppler-document.h)

FIND_LIBRARY(POPPLER_LIBRARY NAMES poppler-cpp)

MARK_AS_ADVANCED(POPPLER_INCLUDE_DIR POPPLER_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(POPPLER
    REQUIRED_VARS POPPLER_LIBRARY POPPLER_INCLUDE_DIR
)

IF(POPPLER_FOUND)
    SET(POPPLER_LIBRARIES ${POPPLER_LIBRARY})
    SET(POPPLER_INCLUDE_DIRS ${POPPLER_INCLUDE_DIR})
ENDIF()
