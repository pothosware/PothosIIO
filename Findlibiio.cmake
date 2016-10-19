# - Try to find libiio
# Once done this will define
#
#  LIBIIO_FOUND - system has Portaudio
#  LIBIIO_INCLUDE_DIRS - the Portaudio include directory
#  LIBIIO_LIBRARIES - Link these to use Portaudio
#  LIBIIO_DEFINITIONS - Compiler switches required for using Portaudio
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (LIBIIO_LIBRARIES AND LIBIIO_INCLUDE_DIRS)
  # in cache already
  set(LIBIIO_FOUND TRUE)
else (LIBIIO_LIBRARIES AND LIBIIO_INCLUDE_DIRS)
   include(FindPkgConfig)
   pkg_check_modules(LIBIIO libiio)
endif (LIBIIO_LIBRARIES AND LIBIIO_INCLUDE_DIRS)

