
set(mainlib hams)
set(mainlib_sources
  ams_utils.h
  ams_utils.cpp)

function (new_nodelib NAME)
  set(libname ${mainlib}_${NAME})
  message("New node: '${libname}' \nwith sources: '${ARGN}'")
  add_library(${libname} SHARED
    ${mainlib_sources}
    ${ARGN})
  target_link_libraries(${libname} Houdini)
#  houdini_configure_target(${libname})
endfunction()



new_nodelib(sop_objectmerge
  sop_objectmerge.h
  sop_objectmerge.cpp)
