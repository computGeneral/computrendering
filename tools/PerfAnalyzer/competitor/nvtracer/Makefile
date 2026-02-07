
all: 
	bash ./get_nvbit.sh
	make -C tracing
	make -C tracing/trace_processing
	#make -C silicon_checkpoint_tool

clean:
	rm -fr nvbit_release
	make clean -C tracing
	make clean -C tracing/trace_processing
