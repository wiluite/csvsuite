# cppp_init.cmake

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

# C++ Plus CMake build script init part.

# C++ Plus cmake build-aux directory.
set(cmakeaux_dir "${CMAKE_CURRENT_LIST_DIR}")

# C++ Plus build-aux directory.
set(buildaux_dir "${CMAKE_CURRENT_LIST_DIR}/..")
# Absolute path of C++ Plus build-aux directory
get_filename_component(buildaux_dir "${buildaux_dir}" ABSOLUTE)

# C++ Plus modules directory.
set(moduledir "${cmakeaux_dir}/../modules")
# Absolute path of C++ Plus modules directory
get_filename_component(moduledir "${moduledir}" ABSOLUTE)

# -------------------------------------------------- Output Directories 

# Binary runtime output directory.
set(output_bindir "${CMAKE_BINARY_DIR}/bin")

# Shared library output directory.
set(output_shareddir "${CMAKE_BINARY_DIR}/lib")

# Static library output directory.
set(output_staticdir "${CMAKE_BINARY_DIR}/lib")

# Include output directory.
set(output_includedir "${CMAKE_BINARY_DIR}/include")

# Test suite output directory.
set(output_testsdir "${CMAKE_BINARY_DIR}/tests")

# Manual output directory on Unix or *nix OS.
set(output_mandir "${CMAKE_BINARY_DIR}/man")

# Install pdb output directory.
set(output_pdbdir "${CMAKE_BINARY_DIR}/pdb")

# -------------------------------------------------- Install Directories

# Install prefix.
set(install_prefix "${CMAKE_INSTALL_PREFIX}")

# Install runtime directory.
set(install_bindir "${install_prefix}/bin")

# Install shared output directory.
if(WIN32)
    # On native Windows, dynmaic link libraries should put in binary directory. 
    set(install_shareddir "${install_prefix}/bin")
else()
    # On other OS, dynmaic link libaries can put in library directory.
    set(install_shareddir "${install_prefix}/lib")
endif()

# Install static library output directory.
set(install_staticdir "${install_prefix}/lib")

# Install include directory.
set(install_includedir "${install_prefix}/include")

# Install manual directory on Unix or *nix OS.
set(install_mandir "${install_prefix}/share/man")
