CCFLAGS=-std=c17 -I./src

build/%.o: src/%.c src/%.h
	cc $(CCFLAGS) -fPIC -c -o $@ $<

lib/%.so: build/%.o
	cc $< -shared -o $@

lib/%.a: %.a
	ar rcs $@ $<

all: lib/fiorino.so

# We will take care of "install" way later
# We should implement standard targets too
# https://www.gnu.org/software/make/manual/html_node/Standard-Targets.html

.PHONY: all
