###############################################################################
# CMake module to search for SQLite 3 library
#
# On success, the macro sets the following variables:
# SQLITE3_FOUND = if the library found
# SQLITE3_LIBRARY = full path to the library
# SQLITE3_LIBRARIES = full path to the library
# SQLITE3_INCLUDE_DIR = where to find the library headers
#
# Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
###############################################################################
#message( ${BINARY_DIR_FOR_SOCI})
find_path(SQLITE3_INCLUDE_DIR
  NAMES sqlite3.h
  PATH_PREFIXES sqlite sqlite3
  PATHS
  ${CMAKE_BINARY_DIR}/external_deps/src)

set(SQLITE3_NAMES local_sqlite3)
find_library(SQLITE3_LIBRARY
  NAMES ${SQLITE3_NAMES}
  PATHS
  ${CMAKE_BINARY_DIR}/external_deps)

set(SQLITE3_LIBRARIES
  ${SQLITE3_LIBRARIES}
  ${SQLITE3_LIBRARY})

#message(STATUS ${SQLITE3_LIBRARY})
# Handle the QUIETLY and REQUIRED arguments and set SQLITE3_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLite3
  DEFAULT_MSG
  SQLITE3_LIBRARIES
  SQLITE3_INCLUDE_DIR)

if(NOT SQLITE3_FOUND)
	message(STATUS "SQLite3 not found (SQLITE3_INCLUDE_DIR=${SQLITE3_INCLUDE_DIR}, SQLITE3_LIBRARY=${SQLITE3_LIBRARY}.")
endif()

mark_as_advanced(SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR SQLITE3_LIBRARIES)
