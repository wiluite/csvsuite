///
/// \file   suite/include/sql-utils/local-sqlite3-dep.h
/// \author wiluite
/// \brief  Absolute dependency paths of the bound sqlite libraries.

#pragma once

#include <string>

#if !defined(__unix__)

struct local_sqlite3_dependency {
    local_sqlite3_dependency() {
        std::string path;
        if (char const * const env_p = getenv("PATH"))
            path = env_p;
#if !defined(BOOST_UT_DISABLE_MODULE)
        path = "PATH=" + path + R"(;..\external_deps)";
#else
        path = "PATH=" + path + R"(;..\..\external_deps)";
#endif
#if !defined(_MSC_VER)
        putenv(path.c_str());
#else
        _putenv(path.c_str());
#endif
    }
};
#endif
