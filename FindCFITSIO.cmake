# Copyright (C) 2019
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(CFITSIO_INCLUDE_DIR NAMES fitsio2.h)

FIND_LIBRARY(CFITSIO_LIBRARY NAMES cfitsio)

MARK_AS_ADVANCED(CFITSIO_INCLUDE_DIR CFITSIO_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CFITSIO
    REQUIRED_VARS CFITSIO_LIBRARY CFITSIO_INCLUDE_DIR
)

IF(CFITSIO_FOUND)
    SET(CFITSIO_LIBRARIES ${CFITSIO_LIBRARY})
    SET(CFITSIO_INCLUDE_DIRS ${CFITSIO_INCLUDE_DIR})
ENDIF()
