cmake_minimum_required(VERSION 3.0)
option(TCMPLX_ACCESS_BUILD_TESTS "build some tests" ON)

if (TCMPLX_ACCESS_BUILD_TESTS)
add_library(munit STATIC munit/munit.c munit/munit.h)
add_library(tcmplx_access_testfont STATIC testfont.c testfont.h)
target_link_libraries(tcmplx_access_testfont mmaptwo munit)
target_include_directories(tcmplx_access_testfont
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../deps>")

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
  target_include_directories(tcmplx_access_test_woff2
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../deps>")
#END   Woff2 test

#BEGIN Seq test
  add_executable(tcmplx_access_test_seq tca_seq.c)
  target_link_libraries(tcmplx_access_test_seq
    tcmplx_access_testfont munit tcmplx_access mmaptwo)
  target_include_directories(tcmplx_access_test_seq
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../deps>")
#END   Seq test

#BEGIN FixList test
  add_executable(tcmplx_access_test_fixlist tca_fixlist.c)
  target_link_libraries(tcmplx_access_test_fixlist
    tcmplx_access_testfont munit tcmplx_access mmaptwo)
#END   FixList test

#BEGIN RingDist test
  add_executable(tcmplx_access_test_ringdist tca_ringdist.c)
  target_link_libraries(tcmplx_access_test_ringdist
    tcmplx_access_testfont munit tcmplx_access)
#END   RingDist test

#BEGIN InsCopy test
  add_executable(tcmplx_access_test_inscopy tca_inscopy.c)
  target_link_libraries(tcmplx_access_test_inscopy
    tcmplx_access_testfont munit tcmplx_access)
#END   InsCopy test

#BEGIN CtxtMap test
  add_executable(tcmplx_access_test_ctxtmap tca_ctxtmap.c)
  target_link_libraries(tcmplx_access_test_ctxtmap
    tcmplx_access_testfont munit tcmplx_access)
#END   CtxtMap test

#BEGIN BDict test
  add_executable(tcmplx_access_test_bdict tca_bdict.c)
  target_link_libraries(tcmplx_access_test_bdict
    tcmplx_access_testfont munit tcmplx_access)
#END   BDict test

#BEGIN ZUtil test
  add_executable(tcmplx_access_test_zutil tca_zutil.c)
  target_link_libraries(tcmplx_access_test_zutil munit tcmplx_access)
#END   ZUtil test
endif (TCMPLX_ACCESS_BUILD_TESTS)
