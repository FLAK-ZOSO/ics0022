CCFLAGS=-std=c17 -I./src

# https://www.gnu.org/software/make/manual/html_node/How-Make-Works.html
all: lib/fiorino.so

build lib:
	mkdir -p $@

build/%.o: src/%.c src/%.h | build
	cc $(CCFLAGS) -fPIC -c -o $@ $<

lib/%.so: build/%.o | lib
	cc $< -shared -o $@

lib/%.a: %.a | lib
	ar rcs $@ $<

# # https://stackoverflow.com/a/45048948/15888601
# directories:
# 	mkdir -p bin build include lib

# We will take care of "install" way later
# We should implement standard targets too
# https://www.gnu.org/software/make/manual/html_node/Standard-Targets.html

clean:
	rm -r build lib

.PHONY: all clean directories
