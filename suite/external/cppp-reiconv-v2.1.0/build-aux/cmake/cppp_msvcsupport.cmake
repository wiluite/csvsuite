# cppp_msvcsupport.cmake

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

# C++ Plus MSVC support.
if(MSVC)
    # Enable this extention only on MSVC toolchain.
    include(CheckCXXCompilerFlag)

    # Checking for the MSVC is support '/utf-8' option.
    check_cxx_compiler_flag("/utf-8" COMPILER_SUPPORTS_UTF8)

    if(COMPILER_SUPPORTS_UTF8)
        add_compile_options(/utf-8)
    endif()

    # If CMAKE_BUILD_TYPE is null, set it to Debug
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug")
    endif()
endif()
