#pragma once
#include <iostream>
#include <mutex>

static std::mutex mut;
struct stdin_subst {
    explicit stdin_subst(std::istream & to) {
        mut.lock();
        cin_buf = std::cin.rdbuf();
        std::cin.rdbuf(to.rdbuf());
    }
    ~stdin_subst() {
        std::cin.rdbuf(cin_buf);
        mut.unlock();
    }
private:
    std::streambuf* cin_buf;
};

struct stdin_redir {
    explicit stdin_redir(char const * const fn) {
        mut.lock();
#ifndef _MSC_VER
        strm = freopen(fn, "r", stdin);
        assert(strm != nullptr);
#else
        auto err = freopen_s(&strm, fn, "r", stdin);
        assert(err == 0);
#endif
    }
    ~stdin_redir() {
        fclose(strm);
        fclose(stdin);
        mut.unlock();
    }
private:
    FILE *strm = nullptr;
};
