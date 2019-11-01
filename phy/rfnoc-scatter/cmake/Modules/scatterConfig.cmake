INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_SCATTER scatter)

FIND_PATH(
    SCATTER_INCLUDE_DIRS
    NAMES scatter/api.h
    HINTS $ENV{SCATTER_DIR}/include
        ${PC_SCATTER_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    SCATTER_LIBRARIES
    NAMES gnuradio-scatter
    HINTS $ENV{SCATTER_DIR}/lib
        ${PC_SCATTER_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SCATTER DEFAULT_MSG SCATTER_LIBRARIES SCATTER_INCLUDE_DIRS)
MARK_AS_ADVANCED(SCATTER_LIBRARIES SCATTER_INCLUDE_DIRS)

