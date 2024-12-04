# locale.cmake

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

# Get locale infomation.

if(NOT DEFINED LOCALE_LANGUAGE_NAME)
    find_package(Python REQUIRED)

    set(LOCALEINFO_OUTPUT_FILE "${CMAKE_BINARY_DIR}/locale_info.txt")
    if(NOT EXISTS "${LOCALEINFO_OUTPUT_FILE}")
        execute_process(COMMAND "${Python_EXECUTABLE}" "${buildaux_dir}/tools/getlocale.py"
                        OUTPUT_FILE "${LOCALEINFO_OUTPUT_FILE}" RESULT_VARIABLE RUN_RESULT)
        if(NOT RUN_RESULT EQUAL 0)
            file(WRITE "${LOCALEINFO_OUTPUT_FILE}" "en_US")
        endif()
        unset(RUN_RESULT)
        set(OUTPUT_MESSAGE TRUE)
    endif()
    file(READ "${LOCALEINFO_OUTPUT_FILE}" LOCALE_LANGUAGE_NAME)
    string(STRIP "${LOCALE_LANGUAGE_NAME}" LOCALE_LANGUAGE_NAME)
    if(DEFINED OUTPUT_MESSAGE)
        message(STATUS "Detected locale language: '${LOCALE_LANGUAGE_NAME}'")
    endif()
    unset(OUTPUT_MESSAGE)
    unset(LOCALEINFO_OUTPUT_FILE)
endif()
