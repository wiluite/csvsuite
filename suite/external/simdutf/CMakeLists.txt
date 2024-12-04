project(simdutf)

if (MSVC)
else()
    add_definitions(-Wno-old-style-cast)
    add_definitions(-Wno-unused-function)
endif()

set(HEADER_FILES simdutf.h)
set(SOURCE_FILES simdutf.cpp)

add_library(simdutf STATIC ${SOURCE_FILES} ${HEADER_FILES})
