# cppp.cmake

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

# C++ Plus CMake build script.

if(NOT DEFINED CPPP_BUILD_AUX_INCLUDED)
    set(CPPP_BUILD_AUX_INCLUDED TRUE)

    # Include init.
    include("${CMAKE_CURRENT_LIST_DIR}/cppp_init.cmake")

    # Other utils.
    include("${cmakeaux_dir}/visibility.cmake")
    include("${cmakeaux_dir}/file.cmake")
    include("${cmakeaux_dir}/library.cmake")
    include("${cmakeaux_dir}/cppp_msvcsupport.cmake")
    include("${cmakeaux_dir}/locale.cmake")

    # Add module
    include("${cmakeaux_dir}/modules.cmake")

    # Uninstall target define.
#    if(NOT TARGET uninstall)
#        configure_file(
#            "${cmakeaux_dir}/cmake_uninstall.cmake.in"
#            "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
#            IMMEDIATE @ONLY )
#        add_custom_target(uninstall
#            COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake" )
#    endif()
endif()
