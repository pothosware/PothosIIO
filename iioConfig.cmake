INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_IIO iio)

FIND_PATH(
    IIO_INCLUDE_DIRS
    NAMES iio/api.h
    HINTS {IIO_DIR}/include
        
    PATHS /include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    IIO_LIBRARIES
    NAMES gnuradio-iio
    HINTS {IIO_DIR}/lib
        
    PATHS /lib
          /lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(IIO DEFAULT_MSG IIO_LIBRARIES IIO_INCLUDE_DIRS)
MARK_AS_ADVANCED(IIO_LIBRARIES IIO_INCLUDE_DIRS)
