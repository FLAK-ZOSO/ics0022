CCFLAGS=-std=c17 -D_POSIX_C_SOURCE=200809L -DOPENSSL_API_COMPAT=0x30000000L -I./src -Wall -g -MMD -MP
VALGRIND ?= valgrind
VALGRIND_FLAGS ?= --leak-check=full --show-leak-kinds=definite,indirect --error-exitcode=1 --enable-debuginfod=no
TEST_BINS = build/test_encrypt_gcm build/test_bundle build/test_encrypt_file_cleanup build/test_fiorino_roundtrip

# https://www.gnu.org/software/make/manual/html_node/How-Make-Works.html
all: lib/fiorino.so

build lib:
	mkdir -p $@

build/%.o: src/%.c src/%.h | build
	cc $(CCFLAGS) -fPIC -c -o $@ $<

lib/fiorino.so: build/fiorino.o build/encrypt.o build/bundle.o | lib
	cc -shared -o $@ $^ -lcrypto

lib/fiorino.a: build/fiorino.o build/encrypt.o build/bundle.o | lib
	ar rcs $@ $^

build/test_encrypt_gcm: tests/encrypt_gcm.c src/encrypt.c src/encrypt.h src/bundle.h | build
	cc $(CCFLAGS) -o $@ tests/encrypt_gcm.c src/encrypt.c -lcrypto

build/test_bundle: tests/bundle.c src/bundle.c src/bundle.h src/fiorino.h | build
	cc $(CCFLAGS) -o $@ tests/bundle.c src/bundle.c

build/test_encrypt_file_cleanup: tests/encrypt_file_cleanup.c lib/fiorino.so | build
	cc $(CCFLAGS) -o $@ $< ./lib/fiorino.so -Wl,-rpath,$(CURDIR)/lib

build/test_fiorino_roundtrip: tests/fiorino_roundtrip.c lib/fiorino.so | build
	cc $(CCFLAGS) -o $@ $< ./lib/fiorino.so -Wl,-rpath,$(CURDIR)/lib

test: $(TEST_BINS)
	./build/test_encrypt_gcm
	./build/test_bundle
	./build/test_encrypt_file_cleanup
	./build/test_fiorino_roundtrip

memtest: $(TEST_BINS)
	@status=0; for test in $(TEST_BINS); do \
		$(VALGRIND) $(VALGRIND_FLAGS) ./$$test || status=1; \
	done; exit $$status

# # https://stackoverflow.com/a/45048948/15888601
# directories:
# 	mkdir -p bin build include lib

# We will take care of "install" way later
# We should implement standard targets too
# https://www.gnu.org/software/make/manual/html_node/Standard-Targets.html

clean:
	rm -r build lib

.PHONY: all clean test memtest
