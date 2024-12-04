///
/// \file   test/test_runner_macros.h
/// \author wiluite
/// \brief  Tests for the csvstat utility.

#pragma once

#define test_reader_configurator_and_runner(arguments, function)         \
        notrimming_reader_type r(std::filesystem::path{arguments.file}); \
        recode_source(r, arguments);                                     \
        function(r, arguments);
