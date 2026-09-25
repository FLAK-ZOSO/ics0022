CCFLAGS=-std=c17 -I./src

# https://www.gnu.org/software/make/manual/html_node/How-Make-Works.html
all: directories lib/fiorino.so

build/%.o: src/%.c src/%.h
	cc $(CCFLAGS) -fPIC -c -o $@ $<

lib/%.so: build/%.o
	cc $< -shared -o $@

lib/%.a: %.a
	ar rcs $@ $<

# https://stackoverflow.com/a/45048948/15888601
directories:
	mkdir -p bin build include lib

# We will take care of "install" way later
# We should implement standard targets too
# https://www.gnu.org/software/make/manual/html_node/Standard-Targets.html

.PHONY: all directories
