#define SOCI_MARIADB_SOURCE
#include "soci/mariadb/soci-mariadb.h"
#include "soci/backend-loader.h"
#include <ciso646>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;


// concrete factory for MariaDB concrete strategies
mariadb_session_backend * mariadb_backend_factory::make_session(
     connection_parameters const & parameters) const
{
     return new mariadb_session_backend(parameters);
}

mariadb_backend_factory const soci::mariadb;

extern "C"
{

// for dynamic backend loading
SOCI_MARIADB_DECL backend_factory const * factory_mariadb()
{
    return &soci::mariadb;
}

SOCI_MARIADB_DECL void register_factory_mariadb()
{
    soci::dynamic_backends::register_backend("mariadb", soci::mariadb);
}

} // extern "C"
