#cmake -DBMICXX_INCLUDE_DIRS=/home/zhengtao.cui/UEB/bmi-cxx -DLIBCURL_DIR=/opt/curl-7.88.1 -DLIBZ_DIR=/opt/zlib-1.2.13 -DHDF_DIR=/opt/hdf5-1.12.3 -DNETCDF_C_DIR=/opt/netcdf-c-4.8.1 -DCMAKE_CXX_FLAGS="-fPIC" -DPNETCDF_DIR=/opt/pnetcdf-1.12.3 -DCMAKE_BUILD_TYPE=Debug

#cmake -DBMICXX_INCLUDE_DIRS=/home/zhengtao.cui/UEB/bmi-cxx -DLIBCURL_DIR=/opt/curl-7.88.1 -DLIBZ_DIR=/opt/zlib-1.2.13 -DHDF_DIR=/opt/hdf5-1.12.3 -DNETCDF_C_DIR=/opt/netcdf-c-4.8.1 -DPNETCDF_DIR=/opt/pnetcdf-1.12.3 -DCMAKE_BUILD_TYPE=Debug

#cmake -DBMICXX_INCLUDE_DIRS=/home/zhengtao.cui/UEB/bmi-cxx -DLIBCURL_DIR=/usr/lib/x86_64-linux-gnu/ -DHDF_DIR=/usr/lib/x86_64-linux-gnu/hdf5/serial -DNETCDF_C_DIR=/usr/lib/x86_64-linux-gnu -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_BUILD_TYPE=Debug 

#cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/home/zhengtao.cui/UEB/cxx_bmi_install -DBMICXX_INCLUDE_DIRS=/home/zhengtao.cui/UEB/cxx_bmi_install/include

cmake -B cmake_build -DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_INSTALL_PREFIX=/home/zhengtao.cui/UEB/cxx_bmi_install \
	-DBMICXX_INCLUDE_DIRS=/home/zhengtao.cui/ngwpc/ngen_no_docker/extern/sloth/extern/bmi-cxx \
	-DLIBCURL_LIB_DIR=/usr/lib/x86_64-linux-gnu \
	-DLIBCURL_INCLUDE_DIR=/usr/include         \
	-DLIBZ_LIB_DIR=/usr/lib/x86_64-linux-gnu \
	-DLIBZ_INCLUDE_DIR=/usr/include         \
	-DHDF_LIB_DIR=/usr/lib/x86_64-linux-gnu/hdf5/serial/lib \
	-DHDF_INCLUDE_DIR=/usr/lib/x86_64-linux-gnu/hdf5/serial/include \
        -DNETCDF_C_LIB_DIR=/usr/lib/x86_64-linux-gnu \
        -DNETCDF_C_INCLUDE_DIR=/usr/include  \
	-S ./

make -C ./cmake_build

#make install
#make

