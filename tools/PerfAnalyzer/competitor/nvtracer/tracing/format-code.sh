# This bash script formats using clang-format
THIS_DIR="$( cd "$( dirname "$BASH_SOURCE" )" && pwd )"
clang-format -i ${THIS_DIR}/*.cu
clang-format -i ${THIS_DIR}/*.h
clang-format -i ${THIS_DIR}/trace_processing/*.cpp
