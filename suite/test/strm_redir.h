///
/// \file   test/strm_redir.h
/// \author wiluite
/// \brief  A stream redirection class macro.

#pragma once

#define redirect(name) struct redirect_##name {             \
    explicit redirect_##name( std::streambuf * new_buffer ) \
        : old( std::name.rdbuf( new_buffer ) ) {}           \
    ~redirect_##name() {std::name.rdbuf( old );}            \
    private:                                                \
        std::streambuf * old;                               \
};

