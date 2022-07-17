
CXX=g++
CXXLAGS=-std=c++20 -g -Wall -Wextra -Werror -Wpedantic -Wmissing-declarations -Wreturn-type \
        -Wparentheses -Wunused -Wundef -Wshadow -Wswitch-default -Wunreachable-code -w #TODO fix errors and remove -w

SRC=${wildcard *.cpp}
OBJ=${patsubst %.cpp,build/%.o,${SRC}}

ifeq (${opt},0)
	CFLAGS+=-g
else
	CFLAGS+=-O3
endif

all: ${OBJ}
	${CXX} ${CXXFLAGS} ${OBJ} ${LDFLAGS} -o auth

build/%.o: %.cpp
	mkdir -p ${dir $@}
	${CXX} -o $@ $< -c ${CXXLAGS}

clean :
	rm -rf auth build *.h.gch

valgrind: auth
	valgrind ./auth --leak-check=full --show-reachable=yes --track-origins=yes
