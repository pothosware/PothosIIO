########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.9)
project(PothosIIO CXX)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    find_package(Pothos "0.6.0" CONFIG REQUIRED)
else()
    find_package(Pothos CONFIG REQUIRED) #in-tree build
endif()

########################################################################
# Find libiio
########################################################################
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
find_package(libiio 0.9)

if (LIBIIO_FOUND)
    message(STATUS "LIBIIO_INCLUDE_DIRS: ${LIBIIO_INCLUDE_DIRS}")
    message(STATUS "LIBIIO_LIBRARIES: ${LIBIIO_LIBRARIES}")
    message(STATUS "LIBIIO_DEFINITIONS: ${LIBIIO_DEFINITIONS}")
else()
    message(WARNING "Pothos IIO blocks require libiio, skipping...")
endif()

########################################################################
# json.hpp header
########################################################################
find_path(JSON_HPP_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)

if (NOT JSON_HPP_INCLUDE_DIR)
    message(WARNING "Pothos LibIIO toolkit requires json.hpp, skipping...")
endif (NOT JSON_HPP_INCLUDE_DIR)

########################################################################
## Feature registration
########################################################################
include(FeatureSummary)
include(CMakeDependentOption)
cmake_dependent_option(ENABLE_IIO "Enable Pothos IIO component" ON "Pothos_FOUND;LIBIIO_FOUND;JSON_HPP_INCLUDE_DIR" OFF)
add_feature_info(IIO ENABLE_IIO "Linux Industrial IO device source and sink blocks")
if (NOT ENABLE_IIO)
    return()
endif()

########################################################################
## Build and install module
########################################################################
include_directories(${LIBIIO_INCLUDE_DIRS})
add_definitions(${LIBIIO_DEFINITIONS})
include_directories(${JSON_HPP_INCLUDE_DIR})

POTHOS_MODULE_UTIL(
    TARGET IIOSupport
    SOURCES
        IIOInfo.cpp
	IIOSink.cpp
	IIOSource.cpp
	IIOSupport.cpp
    LIBRARIES ${LIBIIO_LIBRARIES}
    DESTINATION iio
    ENABLE_DOCS
)
