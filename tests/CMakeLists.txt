cmake_minimum_required(VERSION 3.0)
option(TCMPLX_ACCESS_BUILD_TESTS "build some tests" ON)

if (TCMPLX_ACCESS_BUILD_TESTS)
add_library(munit STATIC munit/munit.c munit/munit.h)

#BEGIN API test
  add_executable(tcmplx_access_test_api tca_api.c)
  target_link_libraries(tcmplx_access_test_api tcmplx_access)
#END   API test
endif (TCMPLX_ACCESS_BUILD_TESTS)
