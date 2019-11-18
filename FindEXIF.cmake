# Copyright (C) 2019
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(EXIF_INCLUDE_DIR NAMES libexif/exif-data.h)

FIND_LIBRARY(EXIF_LIBRARY NAMES exif)

MARK_AS_ADVANCED(EXIF_INCLUDE_DIR EXIF_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EXIF
    REQUIRED_VARS EXIF_LIBRARY EXIF_INCLUDE_DIR
)

IF(EXIF_FOUND)
    SET(EXIF_LIBRARIES ${EXIF_LIBRARY})
    SET(EXIF_INCLUDE_DIRS ${EXIF_INCLUDE_DIR})
ENDIF()
