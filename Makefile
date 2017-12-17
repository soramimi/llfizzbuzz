CXXFLAGS = -std=c++11 -I/usr/lib/llvm-3.9/include

all: llfizzbuzz

llfizzbuzz: main.o
	g++ $^ -o $@ -L/usr/lib/llvm-3.9/lib `llvm-config-3.9 --libs`

clean:
	-rm llfizzbuzz
	-rm *.o

