project(ranav-glob)

set(HEADER_FILES glob/glob.h)
set(SOURCE_FILES glob.cpp)

add_library(ranav-glob STATIC ${SOURCE_FILES} ${HEADER_FILES})
