cmake_minimum_required(VERSION 3.0)
option(TCMPLX_ACCESS_BUILD_TESTS "build some tests" ON)

if (TCMPLX_ACCESS_BUILD_TESTS)
add_library(munit STATIC munit/munit.c munit/munit.h)
endif (TCMPLX_ACCESS_BUILD_TESTS)
