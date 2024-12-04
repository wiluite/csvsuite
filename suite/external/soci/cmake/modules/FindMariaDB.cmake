include(CheckCXXSourceCompiles)

if(WIN32)
   find_path(MARIADB_INCLUDE_DIR mysql.h
      PATHS
      "C:/Program Files/MariaDB 11.5/include/mysql"
      NO_DEFAULT_PATH
   )

   set(MARIADB_LIB_PATHS
      "C:/Program Files/MariaDB 11.5/lib"
   )
   find_library(MARIADB_LIBRARIES NAMES libmariadb
      PATHS
      ${MARIADB_LIB_PATHS}
      NO_DEFAULT_PATH
   )
else(WIN32)
   find_path(MARIADB_INCLUDE_DIR mysql.h
      PATHS
      /usr/include
      /usr/local/include
      PATH_SUFFIXES
      mariadb
   )
   find_library(MARIADB_LIBRARIES NAMES mariadbclient
      PATHS
      $ENV{MARIADB_DIR}/lib
   )
endif(WIN32)

if(MARIADB_LIBRARIES)
   get_filename_component(MARIADB_LIB_DIR ${MARIADB_LIBRARIES} PATH)
endif(MARIADB_LIBRARIES)

set( CMAKE_REQUIRED_INCLUDES ${MARIADB_INCLUDE_DIR} )

if(MARIADB_INCLUDE_DIR AND MARIADB_LIBRARIES)
   set(MARIADB_FOUND TRUE)
   message(STATUS "Found MariaDB: ${MARIADB_INCLUDE_DIR}, ${MARIADB_LIBRARIES}")
else()
   set(MARIADB_FOUND FALSE)
   message(STATUS "MariaDB not found.")
endif()

mark_as_advanced(MARIADB_INCLUDE_DIR MARIADB_LIBRARIES)
