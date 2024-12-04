///
/// \file   utils/csvkit/include/col_types.h
/// \author wiluite
/// \brief  Mostly , all possible column types supported.

#pragma once

namespace csvkit::cli {
    enum struct column_type {
        unknown_t,
        bool_t,
        number_t,
        datetime_t,
        date_t,
        timedelta_t,
        text_t,        
        sz
    };
}
