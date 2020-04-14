cmake_minimum_required(VERSION 3.0)
project(text-complex-access C)


set(text_complex_access_FILES
  api.c     api.h
  )

add_library(tcmplx_access ${text_complex_access_FILES})
target_link_libraries(tcmplx_access mmaptwo)
