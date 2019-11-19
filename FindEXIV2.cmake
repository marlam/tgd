# Copyright (C) 2019
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(EXIV2_INCLUDE_DIR NAMES exiv2/exiv2.hpp)

FIND_LIBRARY(EXIV2_LIBRARY NAMES exiv2)

MARK_AS_ADVANCED(EXIV2_INCLUDE_DIR EXIV2_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EXIV2
    REQUIRED_VARS EXIV2_LIBRARY EXIV2_INCLUDE_DIR
)

IF(EXIV2_FOUND)
    SET(EXIV2_LIBRARIES ${EXIV2_LIBRARY})
    SET(EXIV2_INCLUDE_DIRS ${EXIV2_INCLUDE_DIR})
ENDIF()
