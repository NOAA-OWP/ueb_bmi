cmake_minimum_required(VERSION 3.0)

project (bmi-ueb-cxx CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fPIC")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------
# Find the Boost library and configure usage
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.79.0 REQUIRED COMPONENTS serialization)

if(NOT BMICXX_INCLUDE_DIRS)
    message("--  BMICXX_INCLUDE_DIRS is not defined")
    set(BMICXX_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../bmi-cxx")
    message("--   bmicxx include - ${BMICXX_INCLUDE_DIRS}")
    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../bmi-cxx")
	message(FATAL_ERROR "-- ${CMAKE_CURRENT_LIST_DIR}/../bmi-cxx doesn't exist!")
    endif()
endif()

include_directories(${BMICXX_INCLUDE_DIRS})

if(NOT NETCDF_C_LIB_DIR OR NOT NETCDF_C_INCLUDE_DIR)
   find_package(netCDF REQUIRED)
   if(NOT netCDF_FOUND)
	   message(INFO " netCDF package not found! Please install netCDF or define NETCDF_C_LIB_DIR and NETCDF_C_INCLUDE_DIR")
   endif()
else()
   include_directories(${BMICXX_INCLUDE_DIRS} ${NETCDF_C_INCLUDE_DIR})
   link_directories(  ${NETCDF_C_LIB_DIR} )

endif()

link_libraries(Boost::serialization)

add_subdirectory (src)
#add_subdirectory (testing)

include (CTest)
