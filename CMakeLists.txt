cmake_minimum_required(VERSION 3.0)

project (bmi-ueb-cxx CXX)

#find_package(PkgConfig REQUIRED)
#pkg_check_modules(BMICXX REQUIRED IMPORTED_TARGET bmicxx)
message("--   bmicxx include - ${BMICXX_INCLUDE_DIRS}")
include_directories(${BMICXX_INCLUDE_DIRS})

add_subdirectory (src)
#add_subdirectory (testing)

include (CTest)
