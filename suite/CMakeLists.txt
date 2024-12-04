#include(external/external)

add_subdirectory(external/cppp-reiconv-v2.1.0)
add_subdirectory(external/simdutf)

include_directories(external/CSV_co/include external/CSV_co/include/csv_co/external ${CMAKE_BINARY_DIR}/include include external/multiprecision/include)

add_definitions( -DONLY_C_LOCALE=1 )
if (MSVC)
    add_definitions(/wd4626)
    add_definitions(/wd4820)
    add_definitions(/wd4625)
    add_definitions(/wd5026)
    add_definitions(/wd4244)
    add_definitions(/wd4365)

else()
    add_definitions(-Wno-old-style-cast)
    add_definitions(-Wno-narrowing)
    add_definitions(-Wno-unused-but-set-variable)
#    add_definitions(-Wno-error=reorder)
    add_definitions(-Wno-reorder)
    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
        add_definitions(-Wa,-mbig-obj)
    endif()
    add_definitions(-Wno-format)
    add_definitions(-Wno-unused-function)
endif()

find_package(Threads REQUIRED)

if (IO_CSVSUITE_UTILS)
if (NOT _SANITY_CHECK)
    add_subdirectory(external/soci)
    add_subdirectory(external/ocilib)
    add_definitions(-DOCI_IMPORT_RUNTIME -DOCI_CHARSET_ANSI)

    if (SOCI_HAVE_SQLITE3)
        message("YOU WILL HAVE SQLITE3 BACKEND FOR YOUR I/O UTILS.")
    else()
        set(SQLITE3_INCLUDE_DIR "")
    endif()
    if (SOCI_HAVE_POSTGRESQL)
        message("YOU WILL HAVE POSTGRESQL BACKEND FOR YOUR I/O UTILS.")
    else()
        set(POSTGRESQL_INCLUDE_DIR "")
    endif()
    if (SOCI_HAVE_MYSQL)
        message("YOU WILL HAVE MYSQL BACKEND FOR YOUR I/O UTILS.")
    else()
        set(MYSQL_INCLUDE_DIR "")
    endif()
    if (SOCI_HAVE_MARIADB)
        message("YOU WILL HAVE MARIADB BACKEND FOR YOUR I/O UTILS.")
    else()
        set(MARIADB_INCLUDE_DIR "")
    endif()

    if (SOCI_HAVE_FIREBIRD)
        message("YOU WILL HAVE FIREBIRD BACKEND FOR YOUR I/O UTILS.")
    else()
        set(FIREBIRD_INCLUDE_DIR "")
    endif()
    if (SOCI_HAVE_ORACLE AND SOCI_ORACLE)
        message("YOU WILL BE SUPPORTED BY THE OCILIB LIBRARY TO WORK WITH ORACLE.")
    else()
        set(ORACLE_INCLUDE_DIR "")
    endif()

    add_executable(csvsql csvsql.cpp)
    target_include_directories(csvsql
        PRIVATE
        ${CMAKE_BINARY_DIR}/external/soci/include ${SQLITE3_INCLUDE_DIR} ${POSTGRESQL_INCLUDE_DIR} ${MARIADB_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${FIREBIRD_INCLUDE_DIR} ${ORACLE_INCLUDE_DIR} external/ocilib/include
    )
    target_link_libraries(csvsql PRIVATE libcppp-reiconv.static simdutf soci_core ociliba bz2_connector -lpthread)
    if (UNIX)
    else()
        if (MSVC)
            add_custom_command(
                    TARGET csvsql POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/soci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/soci_core_4_1.dll)
            add_custom_command(
                    TARGET csvsql POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_BINARY_DIR}/external/ocilib/ociliba.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/ociliba.dll)
        else()
            add_custom_command(
                    TARGET csvsql POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/libsoci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/libsoci_core_4_1.dll)
            add_custom_command(
                    TARGET csvsql POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_BINARY_DIR}/external/ocilib/libociliba.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/libociliba.dll)
        endif()
    endif()

    add_executable(sql2csv sql2csv.cpp)
    target_include_directories(sql2csv
        PRIVATE
        ${CMAKE_BINARY_DIR}/external/soci/include ${SQLITE3_INCLUDE_DIR} ${POSTGRESQL_INCLUDE_DIR} ${MARIADB_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${FIREBIRD_INCLUDE_DIR} ${ORACLE_INCLUDE_DIR} external/ocilib/include
    )
    target_link_libraries(sql2csv PRIVATE libcppp-reiconv.static simdutf soci_core ociliba bz2_connector -lpthread)
    if (UNIX)
    else()
        if (MSVC)
            add_custom_command(
                    TARGET sql2csv POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/soci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/soci_core_4_1.dll)
        else()
            add_custom_command(
                    TARGET sql2csv POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/libsoci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/libsoci_core_4_1.dll)
        endif()
    endif()

endif()

add_subdirectory(external/libxls)
add_subdirectory(external/dbf_lib)
add_subdirectory(external/lib_geojson)
add_subdirectory(external/OpenXLSX)
add_executable(in2csv in2csv.cpp src/in2csv/in2csv_fixed.cpp src/in2csv/in2csv_dbf.cpp src/in2csv/in2csv_xls.cpp
        src/in2csv/in2csv_geojson.cpp src/in2csv/in2csv_csv.cpp src/in2csv/in2csv_xlsx.cpp src/in2csv/in2csv_json.cpp
        src/in2csv/in2csv_ndjson.cpp)
target_link_libraries(in2csv libcppp-reiconv.static simdutf dbf xls geojson OpenXLSX::OpenXLSX bz2_connector -lpthread)
target_include_directories(in2csv
        PRIVATE
        external/jsoncons/include
)

endif()

add_subdirectory(test)
add_executable(csvclean csvclean.cpp)
target_link_libraries(csvclean libcppp-reiconv.static simdutf bz2_connector)

add_executable(csvcut csvcut.cpp)
target_link_libraries(csvcut libcppp-reiconv.static simdutf bz2_connector)

add_executable(csvlook csvlook.cpp)
target_link_libraries(csvlook libcppp-reiconv.static simdutf bz2_connector -lpthread)

add_executable(csvstat csvstat.cpp)
target_link_libraries(csvstat libcppp-reiconv.static simdutf bz2_connector -lpthread)

add_executable(csvsort csvsort.cpp)
target_link_libraries(csvsort libcppp-reiconv.static simdutf bz2_connector -lpthread)

add_executable(csvjson csvjson.cpp)
target_link_libraries(csvjson libcppp-reiconv.static simdutf bz2_connector -lpthread)

add_executable(csvjoin csvjoin.cpp)
target_link_libraries(csvjoin libcppp-reiconv.static simdutf bz2_connector -lpthread)

add_executable(csvstack csvstack.cpp)
target_link_libraries(csvstack libcppp-reiconv.static simdutf bz2_connector)

add_executable(csvgrep csvgrep.cpp)
target_link_libraries(csvgrep libcppp-reiconv.static simdutf bz2_connector)

