CXX=g++
CXXFLAGS=-std=c++20 -g -Wall -Wextra -Werror -Wpedantic -Wreturn-type \
        -Wparentheses -Wunused -Wundef -Wshadow -Wswitch-default -Wunreachable-code

SRC=${wildcard src/*.cpp}
OBJ=${patsubst %.cpp,build/%.o,${SRC}}
OBJ2=${subst src/,,${OBJ}}

ifeq (${TEST},1)
	USER_FILE=\"${CURDIR}/build/users.xml\"
	HASH_TOKEN_FILE=\"${CURDIR}/build/hashed_tokens.xml\"
	CXXFLAGS+=-g
else
	USER_FILE=\"${CURDIR}/xml/users.xml\"
	HASH_TOKEN_FILE=\"${CURDIR}/xml/hashed_tokens.xml\"
	CXXFLAGS+=-O3
endif

all:
	$(MAKE) TEST=0 main

tests:
	$(MAKE) TEST=1 unit_tests

unit_tests: library_static test/unit_tests.cpp
	${CXX} ${CXXFLAGS} test/unit_tests.cpp -iquote include -B build -L. -lauthenticator -o build/authenticator_unit_test

main:	library_static
	${CXX} ${CXXFLAGS} test/test.cpp -iquote include -B build -L. -lauthenticator -o build/authenticator

library_dynamic: src/authenticator_api.cpp include/authenticator_api.h
	mkdir -p build
	$(CXX) src/authenticator_api.cpp src/tinyxml2.cpp ${CXXFLAGS} -iquote include -fPIC -shared -lc -o build/libauthenticator.so

library_static: ${OBJ}
	ar rcs build/libauthenticator.a ${OBJ2}

build/%.o: %.cpp
	mkdir -p ${dir ${subst src/,,${subst test/,,$@}} }
	${CXX} -o ${subst src/,,${subst test/,,$@}} $< -c ${CXXFLAGS} -iquote include \
	-DHASH_TOKEN_FILE=${HASH_TOKEN_FILE} -DUSER_FILE=${USER_FILE}

clean :
	rm -rf build *.h.gch

valgrind: auth
	valgrind ./auth --leak-check=full --show-reachable=yes --track-origins=yes