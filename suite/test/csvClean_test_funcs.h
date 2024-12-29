///
/// \file   suite/test/csvClean_test_funcs.h
/// \author wiluite
/// \brief  Several test functions used in the csvClean utility tests.

#ifndef CSV_CO_CSVCLEAN_TEST_FUNCS_H
#define CSV_CO_CSVCLEAN_TEST_FUNCS_H

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"

using namespace boost::ut;

namespace csvsuite::test_facilities {

static auto assertCleaned = [](std::string const & basename, std::vector<std::string> const & output_lines, std::vector<std::string> const & error_lines) {
    auto const output_file = basename + "_out.csv";
    auto const error_file = basename + "_err.csv";
    expect(std::filesystem::exists(std::filesystem::path{output_file}) == !output_lines.empty());
    expect(std::filesystem::exists(std::filesystem::path{error_file}) == !error_lines.empty());

    auto compare_rows = [] (auto const & lines, auto const & file) {
        if (!lines.empty()) {
            std::ifstream f (file);
            f.exceptions(std::ifstream::eofbit);
            try {
                for (auto const & e: lines) {
                    std::string temp;
                    std::getline(f, temp);
                    expect(temp==e);
                }
            } catch (std::exception &) {
                throw;
            }
        }
    };
    class remove_files {
        std::string output_file_;
        std::string error_file_;
    public:
        remove_files(std::string o, std::string e) : output_file_(std::move(o)), error_file_(std::move(e)) {}
        ~remove_files() {
            std::filesystem::remove(std::filesystem::path{output_file_});
            std::filesystem::remove(std::filesystem::path{error_file_});
        }
    };
    remove_files rf (output_file, error_file);
    compare_rows(output_lines, output_file);
    compare_rows(error_lines, error_file);
};

} /// namespace

#endif //CSV_CO_CSVCLEAN_TEST_FUNCS_H
