cmake_minimum_required(VERSION 3.0)
project(text-complex-access C)


set(text_complex_access_FILES
  api.c
  util.c
  offtable.c
  woff2.c
  seq.c
  fixlist.c
  ringdist.c
  inscopy.c
  ctxtmap.c
  )
set(text_complex_access_HEADERS
  ../include/text-complex/access/api.h
  ../include/text-complex/access/util.h
  ../include/text-complex/access/offtable.h
  ../include/text-complex/access/woff2.h
  ../include/text-complex/access/seq.h
  ../include/text-complex/access/fixlist.h
  ../include/text-complex/access/ringdist.h
  ../include/text-complex/access/inscopy.h
  ../include/text-complex/access/ctxtmap.h
  )

add_library(tcmplx_access ${text_complex_access_FILES}
  ${text_complex_access_HEADERS})
target_link_libraries(tcmplx_access mmaptwo)
target_include_directories(tcmplx_access
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../deps>"
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
      "$<INSTALL_INTERFACE:include>"
  )

if (WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(tcmplx_access
    PUBLIC "TCMPLX_A_WIN32_DLL")
endif(WIN32 AND BUILD_SHARED_LIBS)
