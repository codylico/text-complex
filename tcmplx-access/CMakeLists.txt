cmake_minimum_required(VERSION 3.0)
project(text-complex-access C)


set(text_complex_access_FILES
  api.c         api.h
  util.c        util.h
  offtable.c    offtable.h
  woff2.c       woff2.h
  seq.c         seq.h
  fixlist.c     fixlist.h
  ringdist.c    ringdist.h
  inscopy.c     inscopy.h
  )

add_library(tcmplx_access ${text_complex_access_FILES})
target_link_libraries(tcmplx_access mmaptwo)
if (WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(tcmplx_access
    PUBLIC "TCMPLX_A_WIN32_DLL")
endif(WIN32 AND BUILD_SHARED_LIBS)
