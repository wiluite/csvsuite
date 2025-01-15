include_directories(../include  ${CMAKE_BINARY_DIR}/include ../external/multiprecision/include)

add_definitions( -DONLY_C_LOCALE=1 )
if (MSVC)
    add_definitions(/wd4702)
else()
    add_definitions(-Wno-narrowing)
    add_definitions(-Wno-old-style-cast)
    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
        message("CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
        add_definitions(-Wa,-mbig-obj)
    endif()
endif()

add_executable(csvsuite_core_test csvsuite_core_test.cpp)
target_link_libraries(csvsuite_core_test libcppp-reiconv.static  -lpthread )
add_test(csvsuite_core_test csvsuite_core_test)

add_executable(encoding_test encoding_test.cpp)
target_link_libraries(encoding_test libcppp-reiconv.static simdutf bz2_connector)

add_custom_command(
        TARGET encoding_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/bjarne.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/bjarne.csv)
add_custom_command(
        TARGET encoding_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_latin1.csv)
add_test(encoding_test encoding_test)

add_executable(csvClean_bad_skip_lines_test csvClean_bad_skip_lines_test.cpp csvClean_test_funcs.h)
target_link_libraries(csvClean_bad_skip_lines_test libcppp-reiconv.static bz2_connector)
add_custom_command(
        TARGET csvClean_bad_skip_lines_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/bad_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/bad_skip_lines.csv)
add_test(csvClean_bad_skip_lines_test csvClean_bad_skip_lines_test)

add_executable(csvClean_no_header_row_test csvClean_no_header_row_test.cpp csvClean_test_funcs.h)
target_link_libraries(csvClean_no_header_row_test libcppp-reiconv.static bz2_connector)
add_custom_command(
        TARGET csvClean_no_header_row_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/no_header_row.csv)
add_test(csvClean_no_header_row_test csvClean_no_header_row_test)

add_executable(csvClean_optional_quote_characters_test csvClean_optional_quote_characters_test.cpp csvClean_test_funcs.h)
target_link_libraries(csvClean_optional_quote_characters_test libcppp-reiconv.static bz2_connector)
add_custom_command(
        TARGET csvClean_optional_quote_characters_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/optional_quote_characters.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/optional_quote_characters.csv)
add_test(csvClean_optional_quote_characters_test csvClean_optional_quote_characters_test)

add_executable(csvClean_mac_newlines_test csvClean_mac_newlines_test.cpp csvClean_test_funcs.h)
target_link_libraries(csvClean_mac_newlines_test libcppp-reiconv.static bz2_connector)
add_custom_command(
        TARGET csvClean_mac_newlines_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/mac_newlines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/mac_newlines.csv)
add_test(csvClean_mac_newlines_test csvClean_mac_newlines_test)

add_executable(csvClean_changes_character_encoding_test csvClean_changes_character_encoding_test.cpp csvClean_test_funcs.h)
target_link_libraries(csvClean_changes_character_encoding_test libcppp-reiconv.static bz2_connector)
add_custom_command(
        TARGET csvClean_changes_character_encoding_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_latin1.csv)
add_test(csvClean_changes_character_encoding_test csvClean_changes_character_encoding_test)

add_executable(csvClean_removes_bom_test csvClean_removes_bom_test.cpp csvClean_test_funcs.h)
target_link_libraries(csvClean_removes_bom_test libcppp-reiconv.static bz2_connector)
add_custom_command(
        TARGET csvClean_removes_bom_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/external/CSV_co/test/examples/test_utf8_bom.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_utf8_bom.csv)
add_test(csvClean_removes_bom_test csvClean_removes_bom_test)

add_executable(csvClean_dry_run_test csvClean_dry_run_test.cpp)
target_link_libraries(csvClean_dry_run_test libcppp-reiconv.static bz2_connector)
add_custom_command(
        TARGET csvClean_dry_run_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/bad.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/bad.csv)
add_custom_command(
        TARGET csvClean_dry_run_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_field_size_limit.csv)

add_test(csvClean_dry_run_test csvClean_dry_run_test)

add_executable(csvCut_test csvCut_test.cpp)
add_custom_command(
        TARGET csvCut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_skip_lines.csv)
add_custom_command(
        TARGET csvCut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv)
add_custom_command(
        TARGET csvCut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_utf8.csv)
add_custom_command(
        TARGET csvCut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/external/CSV_co/test/examples/dummy.csv.gz
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv.gz)
add_custom_command(
        TARGET csvCut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/external/CSV_co/test/examples/dummy.csv.bz2
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv.bz2)
add_custom_command(
        TARGET csvCut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/no_header_row.csv)
add_custom_command(
        TARGET csvCut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_field_size_limit.csv)
add_custom_command(
        TARGET csvCut_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/null_byte.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/null_byte.csv)

target_link_libraries(csvCut_test libcppp-reiconv.static bz2_connector)
add_test(csvCut_test csvCut_test)

add_executable(csvLook_test csvLook_test.cpp)
add_custom_command(
        TARGET csvLook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_utf8.csv)
add_custom_command(
        TARGET csvLook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy3.csv)
add_custom_command(
        TARGET csvLook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_latin1.csv)
add_custom_command(
        TARGET csvLook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/blanks.csv)
add_custom_command(
        TARGET csvLook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/no_header_row3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/no_header_row3.csv)
add_custom_command(
        TARGET csvLook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy4.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy4.csv)
add_custom_command(
        TARGET csvLook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_precision.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_precision.csv)
add_custom_command(
        TARGET csvLook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/external/CSV_co/test/examples/test_utf8_bom.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_utf8_bom.csv)
add_custom_command(
        TARGET csvLook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv)
add_custom_command(
        TARGET csvLook_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_field_size_limit.csv)

target_link_libraries(csvLook_test libcppp-reiconv.static simdutf bz2_connector -lpthread)
add_test(csvLook_test csvLook_test)

add_executable(csvStat_test csvStat_test.cpp)
add_custom_command(
        TARGET csvStat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testxls_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testxls_converted.csv)
add_custom_command(
        TARGET csvStat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/ks_1033_data.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/ks_1033_data.csv)
add_custom_command(
        TARGET csvStat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/realdata/FY09_EDU_Recipients_by_State.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/realdata/FY09_EDU_Recipients_by_State.csv)
add_custom_command(
        TARGET csvStat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_field_size_limit.csv)
add_custom_command(
        TARGET csvStat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_utf8.csv)
add_custom_command(
        TARGET csvStat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_latin1.csv)
add_custom_command(
        TARGET csvStat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv)
add_custom_command(
        TARGET csvStat_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/no_header_row.csv)

target_link_libraries(csvStat_test libcppp-reiconv.static simdutf bz2_connector -lpthread)
add_test(csvStat_test csvStat_test)

add_executable(csvSort_test csvSort_test.cpp)
add_custom_command(
        TARGET csvSort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_latin1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_latin1.csv)
add_custom_command(
        TARGET csvSort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testxls_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testxls_converted.csv)
add_custom_command(
        TARGET csvSort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_utf8.csv)
add_custom_command(
        TARGET csvSort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/blanks.csv)
add_custom_command(
        TARGET csvSort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/no_header_row.csv)
add_custom_command(
        TARGET csvSort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_literal_order.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_literal_order.csv)
add_custom_command(
        TARGET csvSort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/sort_ints_nulls.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/sort_ints_nulls.csv)
add_custom_command(
        TARGET csvSort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/timedelta.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/timedelta.csv)
add_custom_command(
        TARGET csvSort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_field_size_limit.csv)
add_custom_command(
        TARGET csvSort_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_ignore_case.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_ignore_case.csv)

target_link_libraries(csvSort_test libcppp-reiconv.static simdutf bz2_connector -lpthread)
add_test(csvSort_test csvSort_test)

add_executable(csvJoin_test csvJoin_test.cpp)
add_custom_command(
        TARGET csvJoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/join_a.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/join_a.csv)
add_custom_command(
        TARGET csvJoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/join_b.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/join_b.csv)
add_custom_command(
        TARGET csvJoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/blanks.csv)
add_custom_command(
        TARGET csvJoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv)
add_custom_command(
        TARGET csvJoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/join_no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/join_no_header_row.csv)
add_custom_command(
        TARGET csvJoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/sniff_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/sniff_limit.csv)
add_custom_command(
        TARGET csvJoin_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_field_size_limit.csv)
target_link_libraries(csvJoin_test libcppp-reiconv.static simdutf bz2_connector ranav-glob -lpthread)
add_test(csvJoin_test csvJoin_test)

add_executable(csvStack_test csvStack_test.cpp)
add_custom_command(
        TARGET csvStack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_skip_lines.csv)
add_custom_command(
        TARGET csvStack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv)
add_custom_command(
        TARGET csvStack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy_col_shuffled.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy_col_shuffled.csv)
add_custom_command(
        TARGET csvStack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy_col_shuffled_ragged.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy_col_shuffled_ragged.csv)
add_custom_command(
        TARGET csvStack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy2.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy2.csv)
add_custom_command(
        TARGET csvStack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/no_header_row.csv)
add_custom_command(
        TARGET csvStack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/no_header_row2.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/no_header_row2.csv)
add_custom_command(
        TARGET csvStack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy3.csv)
add_custom_command(
        TARGET csvStack_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_field_size_limit.csv)

target_link_libraries(csvStack_test libcppp-reiconv.static simdutf bz2_connector ranav-glob -lpthread)
add_test(csvStack_test csvStack_test)

add_executable(csvJson_test csvJson_test.cpp)
add_custom_command(
        TARGET csvJson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv)
add_custom_command(
        TARGET csvJson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/blanks.csv)
add_custom_command(
        TARGET csvJson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/no_header_row.csv)
add_custom_command(
        TARGET csvJson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy3.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy3.csv)
add_custom_command(
        TARGET csvJson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_geo.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_geo.csv)
add_custom_command(
        TARGET csvJson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_geojson.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_geojson.csv)
add_custom_command(
        TARGET csvJson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testjson_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testjson_converted.csv)
add_custom_command(
        TARGET csvJson_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_field_size_limit.csv)

target_link_libraries(csvJson_test libcppp-reiconv.static simdutf bz2_connector -lpthread)
add_test(csvJson_test csvJson_test)

add_executable(csvGrep_test csvGrep_test.cpp)
add_custom_command(
        TARGET csvGrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_skip_lines.csv)
add_custom_command(
        TARGET csvGrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv)
target_link_libraries(csvGrep_test libcppp-reiconv.static simdutf -lpthread)
add_custom_command(
        TARGET csvGrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_utf8.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_utf8.csv)
add_custom_command(
        TARGET csvGrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/external/CSV_co/test/examples/test_utf8_bom.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_utf8_bom.csv)
add_custom_command(
        TARGET csvGrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/realdata/FY09_EDU_Recipients_by_State.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/realdata/FY09_EDU_Recipients_by_State.csv)
add_custom_command(
        TARGET csvGrep_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_field_size_limit.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_field_size_limit.csv)

target_link_libraries(csvGrep_test libcppp-reiconv.static simdutf bz2_connector -lpthread)
add_test(csvGrep_test csvGrep_test)

if (IO_CSVSUITE_UTILS)
  if (NOT _SANITY_CHECK)
    add_executable(csvSql_test csvSql_test.cpp)
    target_include_directories(csvSql_test PRIVATE
        ${CMAKE_BINARY_DIR}/../external/soci/include
        ${SQLITE3_INCLUDE_DIR}
        ${POSTGRESQL_INCLUDE_DIR}
        ${MARIADB_INCLUDE_DIR}
        ${MYSQL_INCLUDE_DIR}
        ${FIREBIRD_INCLUDE_DIR}
        ${ORACLE_INCLUDE_DIR}
        ../external/ocilib/include
    )
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testfixed_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testfixed_converted.csv)
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/blanks.csv)
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/no_header_row.csv)
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv)
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/iris.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/iris.csv)
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/irismeta.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/irismeta.csv)
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_query.sql
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_query.sql)
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_query_1252.sql
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_query_1252.sql)
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_query_empty.sql
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_query_empty.sql)
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/foo1.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/foo1.csv)
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/foo2.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/foo2.csv)
    add_custom_command(
        TARGET csvSql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/piped_stdin
        ${CMAKE_CURRENT_BINARY_DIR}/examples/piped_stdin)

    target_link_libraries(csvSql_test libcppp-reiconv.static simdutf soci_core ociliba bz2_connector ranav-glob -lpthread)
    add_test(csvSql_test csvSql_test)

    if (UNIX)
    else()
        if (MSVC)
            add_custom_command(
                    TARGET csvSql_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/soci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/soci_core_4_1.dll)
            add_custom_command(
                    TARGET csvSql_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_BINARY_DIR}/../external/ocilib/ociliba.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/ociliba.dll)

        else()
            add_custom_command(
                    TARGET csvSql_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/libsoci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/libsoci_core_4_1.dll)
            add_custom_command(
                    TARGET csvSql_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_BINARY_DIR}/../external/ocilib/libociliba.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/libociliba.dll)
        endif()
    endif()

    add_executable(Sql2csv_test Sql2csv_test.cpp)
    target_include_directories(Sql2csv_test PRIVATE
        ${CMAKE_BINARY_DIR}/../external/soci/include
        ${SQLITE3_INCLUDE_DIR}
        ${POSTGRESQL_INCLUDE_DIR}
        ${MARIADB_INCLUDE_DIR}
        ${MYSQL_INCLUDE_DIR}
        ${FIREBIRD_INCLUDE_DIR}
        ${ORACLE_INCLUDE_DIR}
        ../external/ocilib/include
    )
    add_custom_command(
            TARGET Sql2csv_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/suite/test/examples/test.sql
            ${CMAKE_CURRENT_BINARY_DIR}/examples/test.sql)
    add_custom_command(
            TARGET Sql2csv_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/suite/test/examples/test_utf8.csv
            ${CMAKE_CURRENT_BINARY_DIR}/examples/test_utf8.csv)
    add_custom_command(
            TARGET Sql2csv_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.csv
            ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv)
    add_custom_command(
            TARGET Sql2csv_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/suite/test/examples/iris.csv
            ${CMAKE_CURRENT_BINARY_DIR}/examples/iris.csv)
    add_custom_command(
            TARGET Sql2csv_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/suite/test/examples/stdin_select
            ${CMAKE_CURRENT_BINARY_DIR}/examples/stdin_select)

    target_link_libraries(Sql2csv_test libcppp-reiconv.static simdutf soci_core ociliba bz2_connector ranav-glob -lpthread)
    add_test(Sql2csv_test Sql2csv_test)

    if (UNIX)
    else()
        if (MSVC)
            add_custom_command(
                    TARGET Sql2csv_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/soci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/soci_core_4_1.dll)
        else()
            add_custom_command(
                    TARGET Sql2csv_test POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_BINARY_DIR}/bin/libsoci_core_4_1.dll
                    ${CMAKE_CURRENT_BINARY_DIR}/libsoci_core_4_1.dll)
        endif()
    endif()

  endif()

add_definitions("-DCSVSUITE_UNIT_TEST=1")

add_executable(In2csv_test In2csv_test.cpp ../src/in2csv/in2csv_fixed.cpp ../src/in2csv/in2csv_dbf.cpp ../src/in2csv/in2csv_xls.cpp
        ../src/in2csv/in2csv_geojson.cpp ../src/in2csv/in2csv_csv.cpp ../src/in2csv/in2csv_xlsx.cpp ../src/in2csv/in2csv_json.cpp
        ../src/in2csv/in2csv_ndjson.cpp)

add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testfixed_schema.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testfixed_schema.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/stdin_select
        ${CMAKE_CURRENT_BINARY_DIR}/examples/stdin_select)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/foo2.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/foo2.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_locale.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_locale.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_locale_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_locale_converted.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/blanks.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/blanks.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/blanks_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/blanks_converted.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_no_leading_zeroes.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_no_leading_zeroes.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_date_format.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_date_format.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_numeric_date_format.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_numeric_date_format.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/date_like_number.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/date_like_number.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testfixed_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testfixed_converted.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_skip_lines.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_skip_lines.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testdbf.dbf
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testdbf.dbf)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testdbf_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testdbf_converted.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testjson.json
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testjson.json)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testjson_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testjson_converted.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_geojson.json
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_geojson.json)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_geojson.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_geojson.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testjson_multiline.json
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testjson_multiline.json)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testjson_multiline_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testjson_multiline_converted.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test.xls
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test.xls)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testxls_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testxls_converted.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/sheets.xls
        ${CMAKE_CURRENT_BINARY_DIR}/examples/sheets.xls)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_skip_lines.xls
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_skip_lines.xls)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test.xlsx
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test.xlsx)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/sheets.xlsx
        ${CMAKE_CURRENT_BINARY_DIR}/examples/sheets.xlsx)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testxlsx_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testxlsx_converted.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_skip_lines.xlsx
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_skip_lines.xlsx)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/no_header_row.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/no_header_row.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.xls
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.xls)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/dummy.xlsx
        ${CMAKE_CURRENT_BINARY_DIR}/examples/dummy.xlsx)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testxls_unicode_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testxls_unicode_converted.csv)
add_custom_command(
        TARGET In2csv_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testxlsx_unicode_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testxlsx_unicode_converted.csv)

target_link_libraries(In2csv_test libcppp-reiconv.static simdutf dbf xls geojson OpenXLSX::OpenXLSX bz2_connector -lpthread)
target_include_directories(In2csv_test
        PRIVATE
        ../external/jsoncons/include
)
add_test(In2csv_test In2csv_test)

add_executable(In2csv_date_test In2csv_date_test.cpp ../src/in2csv/in2csv_csv.cpp)
add_custom_command(
        TARGET In2csv_date_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_date_format.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_date_format.csv)
add_custom_command(
        TARGET In2csv_date_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_date_format_converted.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_date_format_converted.csv)
add_custom_command(
        TARGET In2csv_date_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/test_numeric_date_format.csv
        ${CMAKE_CURRENT_BINARY_DIR}/examples/test_numeric_date_format.csv)

target_link_libraries(In2csv_date_test libcppp-reiconv.static simdutf dbf xls geojson OpenXLSX::OpenXLSX bz2_connector -lpthread)
target_include_directories(In2csv_date_test
        PRIVATE
        ../external/jsoncons/include
)
add_test(In2csv_date_test In2csv_date_test)

add_executable(In2csv_fixed_format_test In2csv_fixed_format_test.cpp ../src/in2csv/in2csv_fixed.cpp)
add_custom_command(
        TARGET In2csv_fixed_format_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/suite/test/examples/testfixed
        ${CMAKE_CURRENT_BINARY_DIR}/examples/testfixed)
add_custom_command(
          TARGET In2csv_fixed_format_test POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/suite/test/examples/testfixed_schema.csv
          ${CMAKE_CURRENT_BINARY_DIR}/examples/testfixed_schema.csv)
add_custom_command(
          TARGET In2csv_fixed_format_test POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/suite/test/examples/testfixed_converted.csv
          ${CMAKE_CURRENT_BINARY_DIR}/examples/testfixed_converted.csv)
add_custom_command(
          TARGET In2csv_fixed_format_test POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/suite/test/examples/testfixed_skip_lines
          ${CMAKE_CURRENT_BINARY_DIR}/examples/testfixed_skip_lines)
add_custom_command(
          TARGET In2csv_fixed_format_test POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/suite/test/examples/testfixed_schema_no_inference.csv
          ${CMAKE_CURRENT_BINARY_DIR}/examples/testfixed_schema_no_inference.csv)

target_link_libraries(In2csv_fixed_format_test libcppp-reiconv.static simdutf dbf xls geojson OpenXLSX::OpenXLSX bz2_connector -lpthread)
add_test(In2csv_fixed_format_test In2csv_fixed_format_test)

endif()

enable_testing ()
