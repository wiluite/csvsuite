#pragma once

namespace in2csv::detail {
    struct converter_client
    {
        virtual ~converter_client() = default;
        virtual void convert() = 0;
    };
}
