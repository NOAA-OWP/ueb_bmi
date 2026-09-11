#
# ueb-bmi/src/CMakeLists.txt

cmake_minimum_required(VERSION 3.0)

set (UEB_VERSION 0.1)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/uebcxx.pc.cmake
                ${CMAKE_BINARY_DIR}/uebcxx.pc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------
# Find the Boost library and configure usage
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.79.0 REQUIRED COMPONENTS serialization)

set(bmi_ueb_lib_SRCS "bmi/ControlFile.cxx" "bmi/Watershed.cxx"
	             "bmi/Parameters.cxx"  "bmi/SiteVariables.cxx"
		     "bmi/ForcingVariables.cxx" "bmi/OutControl.cxx"
		     "bmi/bmi_ueb.cxx" "bmi/bmi_ueb.hxx"
                    "ueb/matrixnvector.cpp" "ueb/uebdecls.cpp"
	            "ueb/ncfunctions.cpp" "ueb/uebinputs.cpp"
		    "ueb/uebpgdecls.h" "ueb/snowxv.cpp"
		    "ueb/canopy.cpp" "ueb/snowdgtv.cpp")
if(WIN32)
  add_library(bmiuebcxx ${bmi_ueb_lib_SRCS})
else()
#add_library(bmiuebcxx STATIC ${bmi_ueb_lib_SRCS})
  add_library(bmiuebcxx SHARED ${bmi_ueb_lib_SRCS})
endif()

target_include_directories(bmiuebcxx PRIVATE ueb utilities )
#target_link_libraries(bmiuebcxx -lmpi -lmpicxx -lnetcdf -lhdf5_hl -lhdf5 -lz libcurl.a libpnetcdf.a)
#target_link_libraries(bmiuebcxx -lnetcdf -lhdf5_hl -lhdf5 -lz -lcurl)
#target_link_libraries(bmiuebcxx -lnetcdf -lhdf5_hl -lhdf5 -lz -lcurl)
target_link_libraries(bmiuebcxx -lnetcdf )

add_executable(run_bmiuebcxx bmi/bmi_main.cxx bmi/ControlFile.cxx
	            bmi/Watershed.cxx bmi/Parameters.cxx
		    bmi/SiteVariables.cxx bmi/ForcingVariables.cxx
		    bmi/OutControl.cxx "bmi/bmi_ueb.cxx"
                    "ueb/matrixnvector.cpp" "ueb/uebdecls.cpp"
	            "ueb/ncfunctions.cpp" "ueb/uebpgdecls.h"
		    "ueb/uebinputs.cpp" "ueb/snowxv.cpp" "ueb/canopy.cpp"
		    "ueb/snowdgtv.cpp" )

target_include_directories(run_bmiuebcxx PRIVATE ueb utilities )
#target_link_libraries(run_bmiuebcxx libnetcdf.a -lhdf5_hl -lhdf5 -lz libcurl.a libpnetcdf.a -lmpi -lmpicxx)
#target_link_libraries(run_bmiuebcxx -lnetcdf -lhdf5_hl -lhdf5 -lz -lcurl)
target_link_libraries(run_bmiuebcxx -lnetcdf)

target_link_libraries(run_bmiuebcxx Boost::serialization)

set_target_properties(bmiuebcxx PROPERTIES 
       PUBLIC_HEADER bmi/ControlFile.hxx 
       PUBLIC_HEADER bmi/Watershed.hxx 
       PUBLIC_HEADER bmi/Parameters.hxx 
       PUBLIC_HEADER bmi/SiteVariables.hxx 
       PUBLIC_HEADER bmi/ForcingVariables.hxx 
       PUBLIC_HEADER bmi/OutControl.hxx 
       PUBLIC_HEADER ueb/uebpgdecls.h 
       PUBLIC_HEADER bmi/bmi_ueb.hxx
       PUBLIC_HEADER utilities/Logger.hpp
)

option(USE_EWTS "Build UEB with EWTS logging" ON)
message("-- UEB CMakeLists USE_EWTS = ${USE_EWTS}")
if(USE_EWTS)
    message("-- UEB compiled with EWTS logging")

    # --- EWTS (installed from nwm-ewts) ---
    find_package(ewts CONFIG REQUIRED)

    target_link_libraries(bmiuebcxx
        ewts::ewts_cpp
        "-Wl,--no-as-needed" ewts::ewts_ngen_bridge "-Wl,--as-needed"
    )
    target_compile_definitions(bmiuebcxx PRIVATE UEB_USE_EWTS)

    target_link_libraries(run_bmiuebcxx
        ewts::ewts_cpp
        "-Wl,--no-as-needed" ewts::ewts_ngen_bridge "-Wl,--as-needed"
    )
    target_compile_definitions(run_bmiuebcxx PRIVATE UEB_USE_EWTS)
else()
    message("-- UEB compiled with stdout fallback logging")
endif()

include(GNUInstallDirs)

install(
  TARGETS run_bmiuebcxx
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(
  TARGETS bmiuebcxx
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

#install(
#  FILES bmi/ControlFile.hxx bmi/Watershed.hxx bmi/Parameters.hxx 
#        bmi/SiteVariables.hxx bmi/ForcingVariables.hxx 
#	bmi/OutControl.hxx ueb/uebpgdecls.h bmi/bmi_ueb.hxx
#	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#)

install(
  FILES ${CMAKE_BINARY_DIR}/uebcxx.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
