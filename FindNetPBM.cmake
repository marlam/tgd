# Copyright (C) 2019
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(NETPBM_INCLUDE_DIR NAMES pam.h)

FIND_LIBRARY(NETPBM_LIBRARY NAMES netpbm)

MARK_AS_ADVANCED(NETPBM_INCLUDE_DIR NETPBM_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NETPBM
    REQUIRED_VARS NETPBM_LIBRARY NETPBM_INCLUDE_DIR
)

IF(NETPBM_FOUND)
    SET(NETPBM_LIBRARIES ${NETPBM_LIBRARY})
    SET(NETPBM_INCLUDE_DIRS ${NETPBM_INCLUDE_DIR})
ENDIF()
