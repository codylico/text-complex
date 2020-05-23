cmake_minimum_required(VERSION 3.0)
option(TCMPLX_ACCESS_BUILD_TESTS "build some tests" ON)

if (TCMPLX_ACCESS_BUILD_TESTS)
add_library(munit STATIC munit/munit.c munit/munit.h)
add_library(tcmplx_access_testfont STATIC testfont.c testfont.h)
target_link_libraries(tcmplx_access_testfont mmaptwo munit)

#BEGIN API test
  add_executable(tcmplx_access_test_api tca_api.c)
  target_link_libraries(tcmplx_access_test_api tcmplx_access)
#END   API test

#BEGIN Util test
  add_executable(tcmplx_access_test_util tca_util.c)
  target_link_libraries(tcmplx_access_test_util munit tcmplx_access)
#END   Util test

#BEGIN OffTable test
  add_executable(tcmplx_access_test_offtable tca_offtable.c)
  target_link_libraries(tcmplx_access_test_offtable munit tcmplx_access)
#END   OffTable test

#BEGIN Woff2 test
  add_executable(tcmplx_access_test_woff2 tca_woff2.c)
  target_link_libraries(tcmplx_access_test_woff2
    tcmplx_access_testfont munit tcmplx_access)
#END   Woff2 test

#BEGIN Seq test
  add_executable(tcmplx_access_test_seq tca_seq.c)
  target_link_libraries(tcmplx_access_test_seq
    tcmplx_access_testfont munit tcmplx_access mmaptwo)
#END   Seq test
endif (TCMPLX_ACCESS_BUILD_TESTS)
