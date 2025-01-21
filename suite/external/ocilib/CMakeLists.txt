cmake_minimum_required(VERSION 3.16.2)
project(ocilib C CXX)

include_directories(include)
add_definitions(-DOCI_IMPORT_RUNTIME -DOCI_CHARSET_ANSI)
if (MSVC)
    add_definitions(/wd4273)
else()
    remove_definitions(-Wno-old-style-cast -fcoroutines -Wno-reorder)
    if (IS_GNU)
        remove_definitions(-Wno-redundant-move)
    endif()
    add_definitions(-Wno-error -Wno-cast-function-type -Wno-attributes)
endif()

add_library(ociliba SHARED src/agent.c
src/array.c
src/bind.c
src/callback.c
src/collection.c
src/column.c
src/connection.c
src/database.c
src/date.c
src/define.c
src/dequeue.c
src/dirpath.c
src/element.c
src/enqueue.c
src/environment.c
src/error.c
src/event.c
src/exception.c
src/file.c
src/format.c
src/handle.c
src/hash.c
src/helpers.c
src/interval.c
src/iterator.c
src/list.c
src/lob.c
src/long.c
src/memory.c
src/message.c
src/mutex.c
src/number.c
src/object.c
src/ocilib.c
src/pool.c
src/queue.c
src/reference.c
src/resultset.c
src/statement.c
src/stringutils.c
src/subscription.c
src/thread.c
src/threadkey.c
src/timestamp.c
src/transaction.c
src/typeinfo.c)
