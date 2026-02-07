#rm ../_BUILD_ -fr

if [ ! -f "../libs/libOpenCL/include/CL/cl.h" ]; then
	# Download OCL -ICD -HEADER
	cmake -S ../libs  -B ../_BUILD_/libs
	# BUILD AND INSTALL OPENCL-HEADER
    	cmake -S ../libs/OpenCL-Headers-2022.09.30  -B ../libs/OpenCL-Headers-2022.09.30/BUILD \
		-DCMAKE_INSTALL_PREFIX=../libs/libOpenCL
	cd ../libs/OpenCL-Headers-2022.09.30/BUILD
	make -j 8
	make install
	cd ../../../scripts/
	# BUILD AND INSTALL OPENCL-ICD
    	cmake -S ../libs/OpenCL-ICD-Loader-2022.09.30  -B ../libs/OpenCL-ICD-Loader-2022.09.30/BUILD \
		-DOPENCL_ICD_LOADER_HEADERS_DIR=../libs/OpenCL-Headers-2022.09.30 \
		-DCMAKE_INSTALL_PREFIX=../libs/libOpenCL/ \
		-DOPENCL_ICD_LOADER_BUILD_TESTING=ON \
		-DBUILD_TESTING=ON
	cd ../libs/OpenCL-ICD-Loader-2022.09.30/BUILD
	make -j 8
	make install
	cd ../../../scripts/
fi

#export LD_LIBRARY_PATH=/opt/OpenCL-ICD/lib:${LD_LIBRARY_PATH}

#cmake -S ./ -B _BUILD_ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=./_BUILD_/gflags-2.2.2 -DBUILD_STATIC_LIBS
#	cd _BUILD_/gflags-2.2.2
#	make -j8
#	make install
#	cd ..

cmake -S ../ -B ../_BUILD_ -DCMAKE_BUILD_TYPE=Debug 
cd ../_BUILD_

make -j 8

