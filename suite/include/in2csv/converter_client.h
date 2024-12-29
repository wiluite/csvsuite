///
/// \file   suite/include/in2csv/converter_client.h
/// \author wiluite
/// \brief  Interface for ANY-to-csv converter.

#pragma once

namespace in2csv::detail {
    struct converter_client
    {
        virtual ~converter_client() = default;
        virtual void convert() = 0;
    };
}
