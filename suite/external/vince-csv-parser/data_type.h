/** @file
 *  @brief Implements data type parsing functionality
 */

#pragma once

#include <limits>
#include <cmath>
#include <cctype>
#include <string>
#include <cassert>
#include "enum_data_type.h"

namespace vince_csv {
    static_assert(DataType::CSV_STRING < DataType::CSV_INT8, "String type should come before numeric types.");
    static_assert(DataType::CSV_INT8 < DataType::CSV_INT64, "Smaller integer types should come before larger integer types.");
    static_assert(DataType::CSV_INT64 < DataType::CSV_DOUBLE, "Integer types should come before floating point value types.");

    namespace internals {

        static const std::string ERROR_NAN = "Not a number.";
        static const std::string ERROR_OVERFLOW = "Overflow error.";
        static const std::string ERROR_FLOAT_TO_INT =
                "Attempted to convert a floating point value to an integral type.";
        static const std::string ERROR_NEG_TO_UNSIGNED = "Negative numbers cannot be converted to unsigned types.";

        template<typename T>
        inline bool is_equal(T a, T b, T epsilon = 0.001) {
            /** Returns true if two floating point values are about the same */
            static_assert(std::is_floating_point<T>::value, "T must be a floating point type.");
            return std::abs(a - b) < epsilon;
        }


        /** Compute 10 to the power of n */
        template<typename T>
        constexpr
        long double pow10(const T& n) noexcept /*requires std::is_integral_v<T>*/{
            long double multiplicand = n > 0 ? 10 : 0.1,
                ret = 1;

            // Make all numbers positive
            T iterations = n > 0 ? n : -n;
            static_assert(std::is_floating_point_v<T>);
            for (/*T*/int i = 0; i < iterations; i++) {
                ret *= multiplicand;
            }

            return ret;
        }

        /** Compute 10 to the power of n */
        template<>
        constexpr
        long double pow10(const unsigned& n) noexcept {
            long double multiplicand = n > 0 ? 10 : 0.1,
                ret = 1;

            for (unsigned i = 0; i < n; i++) {
                ret *= multiplicand;
            }

            return ret;
        }

#ifndef DOXYGEN_SHOULD_SKIP_THIS
        /** Private site-indexed array mapping byte sizes to an integer size enum */
        constexpr DataType int_type_arr[8] = {
            DataType::CSV_INT8,  // 1
            DataType::CSV_INT16, // 2
            DataType::UNKNOWN,
            DataType::CSV_INT32, // 4
            DataType::UNKNOWN,
            DataType::UNKNOWN,
            DataType::UNKNOWN,
            DataType::CSV_INT64  // 8
        };

        template<typename T>
        inline DataType type_num() {
            static_assert(std::is_integral<T>::value, "T should be an integral type.");
            static_assert(sizeof(T) <= 8, "Byte size must be no greater than 8.");
            return int_type_arr[sizeof(T) - 1];
        }

        template<> inline DataType type_num<float>() { return DataType::CSV_DOUBLE; }
        template<> inline DataType type_num<double>() { return DataType::CSV_DOUBLE; }
        template<> inline DataType type_num<long double>() { return DataType::CSV_DOUBLE; }
        template<> inline DataType type_num<std::nullptr_t>() { return DataType::CSV_NULL; }
        template<> inline DataType type_num<std::string>() { return DataType::CSV_STRING; }

        constexpr DataType data_type(std::string_view in, long double* /*const*/ out = nullptr);
#endif

        /** Given a byte size, return the largest number than can be stored in
         *  an integer of that size
         *
         *  Note: Provides a platform-agnostic way of mapping names like "long int" to
         *  byte sizes
         */
        template<size_t Bytes>
        constexpr long double get_int_max() {
            static_assert(Bytes == 1 || Bytes == 2 || Bytes == 4 || Bytes == 8,
                "Bytes must be a power of 2 below 8.");

            if constexpr (sizeof(signed char) == Bytes) {
                return static_cast<long double>(std::numeric_limits<signed char>::max());
            }

            if constexpr (sizeof(short) == Bytes) {
                return static_cast<long double>(std::numeric_limits<short>::max());
            }

            if constexpr (sizeof(int) == Bytes) {
                return static_cast<long double>(std::numeric_limits<int>::max());
            }

            if constexpr (sizeof(long int) == Bytes) {
                return static_cast<long double>(std::numeric_limits<long int>::max());
            }

            if constexpr (sizeof(long long int) == Bytes) {
                return static_cast<long double>(std::numeric_limits<long long int>::max());
            }

            //HEDLEY_UNREACHABLE();
        }

        /** Given a byte size, return the largest number than can be stored in
         *  an unsigned integer of that size
         */
        template<size_t Bytes>
        constexpr long double get_uint_max() {
            static_assert(Bytes == 1 || Bytes == 2 || Bytes == 4 || Bytes == 8,
                "Bytes must be a power of 2 below 8.");

            if constexpr (sizeof(unsigned char) == Bytes) {
                return static_cast<long double>(std::numeric_limits<unsigned char>::max());
            }

            if constexpr (sizeof(unsigned short) == Bytes) {
                return static_cast<long double>(std::numeric_limits<unsigned short>::max());
            }

            if constexpr (sizeof(unsigned int) == Bytes) {
                return static_cast<long double>(std::numeric_limits<unsigned int>::max());
            }

            if constexpr (sizeof(unsigned long int) == Bytes) {
                return static_cast<long double>(std::numeric_limits<unsigned long int>::max());
            }

            if constexpr (sizeof(unsigned long long int) == Bytes) {
                return static_cast<long double>(std::numeric_limits<unsigned long long int>::max());
            }

            //HEDLEY_UNREACHABLE();
        }

        /** Largest number that can be stored in a 8-bit integer */
        constexpr long double CSV_INT8_MAX = get_int_max<1>();

        /** Largest number that can be stored in a 16-bit integer */
        constexpr long double CSV_INT16_MAX = get_int_max<2>();

        /** Largest number that can be stored in a 32-bit integer */
        constexpr long double CSV_INT32_MAX = get_int_max<4>();

        /** Largest number that can be stored in a 64-bit integer */
        constexpr long double CSV_INT64_MAX = get_int_max<8>();

        /** Largest number that can be stored in a 8-bit ungisned integer */
        constexpr long double CSV_UINT8_MAX = get_uint_max<1>();

        /** Largest number that can be stored in a 16-bit unsigned integer */
        constexpr long double CSV_UINT16_MAX = get_uint_max<2>();

        /** Largest number that can be stored in a 32-bit unsigned integer */
        constexpr long double CSV_UINT32_MAX = get_uint_max<4>();

        /** Largest number that can be stored in a 64-bit unsigned integer */
        constexpr long double CSV_UINT64_MAX = get_uint_max<8>();

        /** Given a pointer to the start of what is start of
         *  the exponential part of a number written (possibly) in scientific notation
         *  parse the exponent
         */
        constexpr
        DataType _process_potential_exponential(
            std::string_view exponential_part,
            const long double& coeff,
            long double * const out) {
            long double exponent = 0;
            auto result = data_type(exponential_part, &exponent);

            // Exponents in scientific notation should not be decimal numbers
            if (result >= DataType::CSV_INT8 && result < DataType::CSV_DOUBLE) {
                if (out) *out = coeff * pow10(exponent);
                return DataType::CSV_DOUBLE;
            }

            return DataType::CSV_STRING;
        }

        /** Given the absolute value of an integer, determine what numeric type
         *  it fits in
         */
#ifdef _MSC_VER

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
# define HEDLEY_MSVC_VERSION_CHECK(major,minor,patch) (_MSC_FULL_VER >= ((major * 1000000) + (minor * 10000) + (patch)))
#endif
#if HEDLEY_MSVC_VERSION_CHECK(14, 0, 0)
#  define HEDLEY_PURE __declspec(noalias)
#else
#  define HEDLEY_PURE
#endif

#else
#  define HEDLEY_PURE __attribute__((__pure__))
#endif
        HEDLEY_PURE constexpr
        DataType _determine_integral_type(const long double& number) noexcept {
            // We can assume number is always non-negative
            assert(number >= 0);

            if (number <= internals::CSV_INT8_MAX)
                return DataType::CSV_INT8;
            else if (number <= internals::CSV_INT16_MAX)
                return DataType::CSV_INT16;
            else if (number <= internals::CSV_INT32_MAX)
                return DataType::CSV_INT32;
            else if (number <= internals::CSV_INT64_MAX)
                return DataType::CSV_INT64;
            else // Conversion to long long will cause an overflow
                return DataType::CSV_DOUBLE;
        }

        /** Distinguishes numeric from other text values. Used by various
         *  type casting functions, like csv_parser::CSVReader::read_row()
         *
         *  #### Rules
         *   - Leading and trailing whitespace ("padding") ignored
         *   - A string of just whitespace is NULL
         *
         *  @param[in]  in  String value to be examined
         *  @param[out] out Pointer to long double where results of numeric parsing
         *                  get stored
         */
        constexpr
        DataType data_type(std::string_view in, long double* const out) {
            // Empty string --> NULL
            std::size_t start_pos;
            if ((start_pos = in.find_first_not_of(' ')) == std::string::npos)
                return DataType::CSV_NULL;

            bool ws_allowed = true,
                neg_allowed = true,
                dot_allowed = true,
                digit_allowed = true,
                has_digit = false,
                prob_float = false;

            unsigned places_after_decimal = 0;
            long double integral_part = 0, decimal_part = 0;
            start_pos = in.find_first_not_of('+', start_pos);
            if (start_pos == std::string::npos) {
                // there we only +++
                return DataType::CSV_STRING;
            }

            for (size_t i = start_pos, ilen = in.size(); i < ilen; i++) {
                const char& current = in[i];

                switch (current) {
                case ' ':
                    if (!ws_allowed) {
                        if (isdigit(in[i - 1])) {
                            digit_allowed = false;
                            ws_allowed = true;
                        }
                        else {
                            // Ex: '510 123 4567'
                            return DataType::CSV_STRING;
                        }
                    }
                    break;
                case '-':
                    if (!neg_allowed) {
                        // Ex: '510-123-4567'
                        return DataType::CSV_STRING;
                    }

                    neg_allowed = false;
                    break;
                case '.':
                    if (!dot_allowed) {
                        return DataType::CSV_STRING;
                    }

                    dot_allowed = false;
                    prob_float = true;
                    break;
                case 'e':
                case 'E':
                    // Process scientific notation
                    if (prob_float || (i && i + 1 < ilen && isdigit(in[i - 1]))) {
                        size_t exponent_start_idx = i + 1;
                        prob_float = true;

                        // Strip out plus sign
                        if (in[i + 1] == '+') {
                            exponent_start_idx++;
                        }

                        return _process_potential_exponential(
                            in.substr(exponent_start_idx),
                            neg_allowed ? integral_part + decimal_part : -(integral_part + decimal_part),
                            out
                        );
                    }

                    return DataType::CSV_STRING;
                    break;
                default:
                    auto const digit = static_cast<short>(current - '0');
                    if (digit >= 0 && digit <= 9) {
                        // Process digit
                        has_digit = true;

                        if (!digit_allowed)
                            return DataType::CSV_STRING;
                        else if (ws_allowed) // Ex: '510 456'
                            ws_allowed = false;

                        // Build current number
                        if (prob_float)
                            decimal_part += digit / pow10(++places_after_decimal);
                        else
                            integral_part = (integral_part * 10) + digit;
                    }
                    else {
                        return DataType::CSV_STRING;
                    }
                }
            }

            // No non-numeric/non-whitespace characters found
            if (has_digit) {
                long double number = integral_part + decimal_part;
                if (out) {
                    *out = neg_allowed ? number : -number;
                }

                return prob_float ? DataType::CSV_DOUBLE : _determine_integral_type(number);
            }

            // Just whitespace
            return DataType::CSV_NULL;
        }
    }
}
