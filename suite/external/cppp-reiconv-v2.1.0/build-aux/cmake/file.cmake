# file.cmake

# Copyright (C) 2023 The C++ Plus Project.
# This file is part of the build-aux library.
#
# The build-aux is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# The build-aux is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with the build-aux; see the file COPYING.  If not,
# see <https://www.gnu.org/licenses/>.

# C++ Plus file utils for CMake

macro(cppp_file_compare file1 file2)
    # Reading file1
    file(READ "${file1}" content1)
    string(REPLACE "\r\n" "\n" content1 "${content1}")
    string(STRIP "${content1}" content1)

    # Reading file2
    file(READ "${file2}" content2)
    string(REPLACE "\r\n" "\n" content2 "${content2}")
    string(STRIP "${content2}" content2)

    # Compare file contents
    if(NOT "${content1}" STREQUAL "${content2}")
        message(FATAL_ERROR "File content difference detected between \"${file1}\" and \"${file2}\".")
    endif()
endmacro()
