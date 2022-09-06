
CXX=g++
CXXLAGS=-std=c++20 -g -Wall -Wextra -Werror -Wpedantic -Wreturn-type \
        -Wparentheses -Wunused -Wundef -Wshadow -Wswitch-default -Wunreachable-code

SRC=${wildcard src/authenticator_api.cpp}
OBJ=${patsubst %.cpp,build/%.o,${SRC}}

ifeq (${opt},0)
	CXXLAGS+=-g
else
	CXXLAGS+=-O3
endif

all: ${OBJ}
	$echo "lalalalala"
	${CXX} ${CXXFLAGS} ${OBJ} ${SRC} ${LDFLAGS} -o auth -iquote export

build/%.o: %.cpp
	mkdir -p ${dir $@}
	${CXX} -o $@ $< -c ${CXXLAGS} -iquote export

clean :
	rm -rf auth build *.h.gch

valgrind: auth
	valgrind ./auth --leak-check=full --show-reachable=yes --track-origins=yes
