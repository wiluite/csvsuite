set(MariaDB_FIND_QUIETLY TRUE)

find_package(MariaDB)

boost_external_report(MariaDB INCLUDE_DIR LIBRARIES)
