# how to run a exist test
>> cd scripts
>> ./build.sh               # test will be built at ocl_env/_BUILD_
>> ./pdumpCap.sh <testName> # test pdump captured at ocl_env/_BUILD_/pdump/
>> ./pdumpSim.sh <testName> # verify the pdump generated at ocl_env/_BUILD_/pdump/

# how to add a new raw ocl test
1. add new folder under ocl_env/apps/<ocl_template>
2. add new case folder in the ocl_env/CMakeLists.txt like "add_subdirectory(apps/ocl_tempalte)"
3. follow the "how to run an exist test" to "run" and "dump" and "verify"

note: if the test use external lib, please put the lib get method in ocl_env/CMakeLists.txt, and add lib generation manner in the ocl_env/CMakeLists.txt

