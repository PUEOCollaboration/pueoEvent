### Makefile that delegates building to cmake 


.PHONY: all configure clean cleaner install cmake-build cmake-clean cmake-install

all: cmake-build 
clean: cmake-clean
install: cmake-install 


### TODO add doxygen into CMakelists 
doc: legacy-doc

cmake-build: build/Makefile 
	@+make -C  ./build

legacy-doc: 
	@make -f Makefile.legacy doc 

configure: build/Makefile 
	@ccmake . build 

cmake-install: 
	@make -C ./build install 

build/Makefile: 
	@echo "Setting up cmake build. Use "make legacy" to use legacy Makefile" 
	@mkdir -p build 
	@cd build && cmake ../ 

distclean: 
	@echo "Removing cmake directory" 
	@rm -rf build 

cmake-clean: build/Makefile 
	@make -C ./build clean 
