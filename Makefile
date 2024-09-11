DEPS_CLONE_PATH := /home/zmy/work/rdma/rdma-bw-exercise-pdsl/deps/3rd
DEPS_INSTALL_PATH := /home/zmy/work/rdma/rdma-bw-exercise-pdsl/deps/install

download_deps:
	@mkdir -p $(DEPS_CLONE_PATH)

	@echo "downloading fmtlib/fmt"
	@cd $(DEPS_CLONE_PATH) && git clone https://github.com/fmtlib/fmt.git

	@echo "downloading open-source-parsers/jsoncpp"
	@cd $(DEPS_CLONE_PATH) && git clone https://github.com/open-source-parsers/jsoncpp.git

	@echo "downloading cinemast/libjson-rpc-cpp"
	@cd $(DEPS_CLONE_PATH) && git clone https://github.com/cinemast/libjson-rpc-cpp.git

remove_deps:
	rm -rf ./deps/*

install_deps:
	@echo "installing fmt"
	cd $(DEPS_CLONE_PATH)/fmt && \
	mkdir -p build && cd build && \
	cmake -DCMAKE_INSTALL_PREFIX=$(DEPS_INSTALL_PATH) .. && \
	make -j8 && make install 

	@echo "installing json-cpp..."
	cd $(DEPS_CLONE_PATH)/jsoncpp && \
	mkdir -p build && cd build && \
	cmake -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$(DEPS_INSTALL_PATH) -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. && \
	make -j8 && make install

	@echo "installing libjson-rpc-cpp"
	cd $(DEPS_CLONE_PATH)/libjson-rpc-cpp && \
	mkdir -p build && cd build && \
	cmake -DCMAKE_INSTALL_PREFIX=$(DEPS_INSTALL_PATH) -DCMAKE_PREFIX_PATH=$(DEPS_INSTALL_PATH) -DCOMPILE_TESTS=NO -DCOMPILE_STUBGEN=NO -DCOMPILE_EXAMPLES=NO -DHTTP_SERVER=NO -DHTTP_CLIENT=NO -DREDIS_SERVER=NO -DREDIS_CLIENT=NO -DTCP_SOCKET_SERVER=YES -DTCP_SOCKET_CLIENT=YES .. && \
	make -j8 && make install && sudo ldconfig

deps: download_deps install_deps

clean:
	rm -rf build

build:
	mkdir build
	cd build && cmake .. && make -j4

client:
	./build/client 

server:
	./build/server
