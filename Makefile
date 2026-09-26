CCFLAGS=-std=c17 -D_POSIX_C_SOURCE=200809L -DOPENSSL_API_COMPAT=0x30000000L -I./src -Wall -MMD -MP

# https://www.gnu.org/software/make/manual/html_node/How-Make-Works.html
all: lib/fiorino.so

build lib:
	mkdir -p $@

build/%.o: src/%.c src/%.h | build
	cc $(CCFLAGS) -fPIC -c -o $@ $<

lib/fiorino.so: build/fiorino.o build/encrypt.o | lib
	cc -shared -o $@ $^ -lcrypto

lib/fiorino.a: build/fiorino.o build/encrypt.o | lib
	ar rcs $@ $^

# # https://stackoverflow.com/a/45048948/15888601
# directories:
# 	mkdir -p bin build include lib

# We will take care of "install" way later
# We should implement standard targets too
# https://www.gnu.org/software/make/manual/html_node/Standard-Targets.html

clean:
	rm -r build lib

.PHONY: all clean directories
