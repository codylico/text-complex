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

  bdict.c    bdict_p.h  bdict_cb.c
  bdict04.c  bdict05.c  bdict06.c  bdict07.c  bdict08.c
  bdict09.c  bdict10.c  bdict11.c  bdict12.c  bdict13.c
  bdict14.c  bdict15.c  bdict16.c  bdict17.c  bdict18.c
  bdict19.c  bdict20.c  bdict21.c  bdict22.c  bdict23.c
  bdict24.c

  zutil.c
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
  ../include/text-complex/access/bdict.h
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
