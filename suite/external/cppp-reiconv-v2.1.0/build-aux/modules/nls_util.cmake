# nls_util.cmake

# Copyright (C) 2023 The C++ Plus Project.
# This file is part of the build-aux Library.
#
# The build-aux Library is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or (at your option) any later version.
#
# The build-aux Library is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with the build-aux Library; see the file COPYING.
# If not, see <https://www.gnu.org/licenses/>.


function(cppp_init_nls_util)
    if(NOT TARGET build_nls_${PROJECT_NAME})
        add_custom_target(build_nls_${PROJECT_NAME} ALL)
    endif()
    find_package(Python REQUIRED)
    
endfunction()

function(cppp_nls_translate file langmap)
    find_package(Python REQUIRED)
    add_custom_command(TARGET build_nls_${PROJECT_NAME} POST_BUILD
                       COMMAND "${Python_EXECUTABLE}" "${buildaux_dir}/nls_util/nls_util.py" "${file}" "${file}" "${langmap}"
                       COMMENT "Translating \"${file}\" with langmap file \"${langmap}\" ..." )
endfunction()

macro(cppp_nls_autotranslate file langmaps_directory)
    set(langmap "${langmaps_directory}/${LOCALE_LANGUAGE_NAME}.langmap")
    if(NOT EXISTS "${langmap}")
        set(langmap "${langmaps_directory}/en_US.langmap")
    endif()
    if(EXISTS "${langmap}")
        cppp_nls_translate("${file}" "${langmap}")
    else()
        # langmap not exist, report a warning
        message(WARNING "Language map file '${langmap}' is not exist, C++ Plus NLS Auto Translate will not take effect.")
    endif()
endmacro()
