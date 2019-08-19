# Copyright (C) 2019
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

FIND_PATH(PFS_INCLUDE_DIR NAMES pfs/pfs.h)

FIND_LIBRARY(PFS_LIBRARY NAMES pfs)

MARK_AS_ADVANCED(PFS_INCLUDE_DIR PFS_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PFS
    REQUIRED_VARS PFS_LIBRARY PFS_INCLUDE_DIR
)

IF(PFS_FOUND)
    SET(PFS_LIBRARIES ${PFS_LIBRARY})
    SET(PFS_INCLUDE_DIRS ${PFS_INCLUDE_DIR})
ENDIF()
