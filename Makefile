default: mumbi
all: mumbi ext ext-inplace

mumbi.o: src/mumbi.c src/mumbi.h
	gcc -c src/mumbi.c -o mumbi.o

mumbi: mumbi.o
	gcc mumbi.o -lmraa -o mumbi

ext:
	python3 setup.py build

ext-inplace:
	python3 setup.py build_ext --inplace

install: ext
	python3 setup.py install

clean:
	rm -f mumbi mumbi.o mumbi.cpython-*.so
	rm -rf build

