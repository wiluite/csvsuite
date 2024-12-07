///
/// \file   utils/csvsuite/include/printer_concepts.h
/// \author wiluite
/// \brief  Concepts to distinquish how to print a generated csv.

#ifndef CSV_CO_PRINTER_CONCEPTS_H
#define CSV_CO_PRINTER_CONCEPTS_H

namespace csvsuite::cli {
    template <class T>
    concept CsvKitCellSpanTableConcept = requires (T t) {
        { ((*t.begin()).at(0)).operator csv_co::cell_string() };
        { (t.operator[](0)).operator [](0)  };
    };

    template <class T>
    concept CsvKitCellSpanRowConcept = requires (T t) {
        { (t.at(0)).operator csv_co::cell_string() };
    };

    template <class T>
    concept CellSpanRowConcept = requires (T t) {
        { t.operator[](std::string()) };
    };

    template <class T>
    concept CsvReaderConcept = requires (T t) {
        { t.validate() };
    };

    void print_LF(auto & stdout_) {
        stdout_ << "\n";
    }
}

#endif
