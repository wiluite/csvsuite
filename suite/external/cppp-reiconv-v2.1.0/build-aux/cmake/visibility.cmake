# visibility.cmake

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

# Tests whether the compiler supports the command-line option
# -fvisibility=hidden and the function and variable attributes
# __attribute__((__visibility__("hidden"))) and
# __attribute__((__visibility__("default"))).
# Does *not* test for __visibility__("protected") - which has tricky
# semantics (see the 'vismain' test in glibc) and does not exist e.g. on
# Mac OS X.
# Does *not* test for __visibility__("internal") - which has processor
# dependent semantics.
# Does *not* test for #pragma GCC visibility push(hidden) - which is
# "really only recommended for legacy code".
#

include(CheckCCompilerFlag)

macro(check_have_visibility)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        check_c_compiler_flag("-fvisibility=default" HAVE_VISIBILITY)
    else()
        set(HAVE_VISIBILITY 0)
    endif()
endmacro()
