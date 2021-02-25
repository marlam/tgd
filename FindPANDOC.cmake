# Copyright (C) 2021
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

find_program(
    PANDOC_EXECUTABLE
    NAMES pandoc
    DOC "pandoc executable"
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    PANDOC
    FOUND_VAR PANDOC_FOUND
    REQUIRED_VARS PANDOC_EXECUTABLE
)

mark_as_advanced(PANDOC_EXECUTABLE)
