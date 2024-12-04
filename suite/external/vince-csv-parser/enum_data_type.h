///
/// \file   include/csv_co/external/enum_data_type.h
/// \author From Vince's CSV Parser
/// \brief  Possible types detected for all typed spans. Borrowed from VINCE CSV parser implementation.

#pragma once
namespace vince_csv {
    /** Enumerates the different CSV field types that are
     *  recognized by this library
     *
     *  @note Overflowing integers will be stored and classified as doubles.
     *  @note Unlike previous releases, integer enums here are platform agnostic.
     */
    enum class DataType : signed char {
        UNKNOWN = -1,
        CSV_NULL,   /**< Empty string */
        CSV_STRING, /**< Non-numeric string */
        CSV_INT8,   /**< 8-bit integer */
        CSV_INT16,  /**< 16-bit integer (short on MSVC/GCC) */
        CSV_INT32,  /**< 32-bit integer (int on MSVC/GCC) */
        CSV_INT64,  /**< 64-bit integer (long long on MSVC/GCC) */
        CSV_DOUBLE  /**< Floating point value */
    };
}

