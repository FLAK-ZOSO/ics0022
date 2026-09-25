CCFLAGS=-std=c17 -I./src

build/%.o: src/%.c src/%.h
	cc $(CCFLAGS) -fPIC -c -o $@ $<

lib/%.so: build/%.o
	cc $< -shared -o $@

lib/%.a: %.a
	ar rcs $@ $<

directories:
	$(shell mkdir -p bin build include lib)

all: directories lib/fiorino.so

# We will take care of "install" way later
# We should implement standard targets too
# https://www.gnu.org/software/make/manual/html_node/Standard-Targets.html

# https://stackoverflow.com/a/80005641/15888601
# https://stackoverflow.com/a/1951024/15888601

.PHONY: all directories
