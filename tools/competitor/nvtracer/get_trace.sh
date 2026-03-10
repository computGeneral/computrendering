
export CUDA_VISIBLE_DEVICES=0
LD_PRELOAD=./tracing/tracing.so $1
./tracing/trace_processing/post_processing ./_traces_/kernellist


