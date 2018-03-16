start:
	cd build; cmake ..; make; mv compile_commands.json ..
run: start
	./build/server
clean:
	cd build; make clean;
proto:
	protoc -I protos --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` protos/nodecom.proto
	protoc -I protos --cpp_out=. protos/nodecom.proto

