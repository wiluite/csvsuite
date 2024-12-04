# library.cmake

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

# C++ Plus library utils for CMake

# Build library
#macro(cppp_build_library name sources enable_shared enable_static resource_file)
macro(cppp_build_library name sources enable_static resource_file)
    if(${enable_shared})
        if(MSVC)
            set(RCFILE "${resource_file}")
        else()
            set(RCFILE "")
        endif()
        add_library(lib${name}.shared SHARED ${sources} "${RCFILE}")
        set_target_properties(lib${name}.shared PROPERTIES
            OUTPUT_NAME ${name}
            ARCHIVE_OUTPUT_DIRECTORY "${output_staticddir}"
            RUNTIME_OUTPUT_DIRECTORY "${output_bindir}"
            LIBRARY_OUTPUT_DIRECTORY "${output_shareddir}"
            PDB_OUTPUT_DIRECTORY "${output_pdbdir}"
            VERSION ${PROJECT_VERSION} )
    endif()
    if(${enable_static})
        add_library(lib${name}.static STATIC ${sources})
        set_target_properties(lib${name}.static PROPERTIES
            OUTPUT_NAME ${name}.static
            ARCHIVE_OUTPUT_DIRECTORY "${output_staticdir}"
            RUNTIME_OUTPUT_DIRECTORY "${output_bindir}"
            LIBRARY_OUTPUT_DIRECTORY "${output_shareddir}"
            PDB_OUTPUT_DIRECTORY "${output_pdbdir}"
            VERSION ${PROJECT_VERSION} )
    endif()
endmacro()
