if(NOT LIBDSD_FOUND)
  pkg_check_modules (LIBDSD_PKG libdsd)
  find_path(LIBDSD_INCLUDE_DIR NAMES dsd_api.h
    PATHS
    ${LIBDSD_PKG_INCLUDE_DIRS}
    /usr/include
    /usr/local/include
    /usr/local/include/dsd
  )

  find_library(LIBDSD_LIBRARIES NAMES gr-dsd
    PATHS
    ${LIBDSD_PKG_LIBRARY_DIRS}
    /usr/lib
    /usr/local/lib
  )

if(LIBDSD_INCLUDE_DIR AND LIBDSD_LIBRARIES)
  set(LIBDSD_FOUND TRUE CACHE INTERNAL "libdsd found")
  message(STATUS "Found libdsd: ${LIBDSD_INCLUDE_DIR}, ${LIBDSD_LIBRARIES}")
else(LIB_INCLUDE_DIR AND LIBDSD_LIBRARIES)
  set(LIBDSD_FOUND FALSE CACHE INTERNAL "libdsd found")
  message(STATUS "libdsd not found.")
endif(LIBDSD_INCLUDE_DIR AND LIBDSD_LIBRARIES)

mark_as_advanced(LIBDSD_INCLUDE_DIR LIBDSD_LIBRARIES)

endif(NOT LIBDSD_FOUND)
