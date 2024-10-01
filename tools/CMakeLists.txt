cmake_minimum_required(VERSION 3.0)
option(TCMPLX_ACCESS_BUILD_TOOLS "build some tools" OFF)

if (TCMPLX_ACCESS_BUILD_TOOLS)
#BEGIN ZCvt tool
  add_executable(tcmplx_access_zcvt tca_zcvt.c)
  target_link_libraries(tcmplx_access_zcvt tcmplx_access)
#END   ZCvt tool

#BEGIN BrCvt tool
add_executable(tcmplx_access_brcvt tca_brcvt.c)
target_link_libraries(tcmplx_access_brcvt tcmplx_access)
#END   BrCvt tool
endif (TCMPLX_ACCESS_BUILD_TOOLS)
