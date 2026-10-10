SANFLAGS=-fsanitize=address,undefined
ASMFLAGS=-S -fverbose-asm
DBGFLAGS=-g
CCFLAGS=-std=c17 -D_POSIX_C_SOURCE=200809L -DOPENSSL_API_COMPAT=0x30000000L \
	-I./src -Wall -Wextra $(DBGFLAGS) -MMD -MP $(SANFLAGS)
VALGRIND ?= valgrind
VALGRIND_FLAGS ?= --leak-check=full --show-leak-kinds=definite,indirect --error-exitcode=1 --enable-debuginfod=no
TEST_BINS = build/test_encrypt_gcm build/test_bundle build/test_encrypt_file_cleanup \
			build/test_fiorino_limits build/test_fiorino_roundtrip \
			build/test_fiorino_symlink build/test_fiorino_log
CLI_TESTS = build/fiorino-encrypt build/fiorino-decrypt
CC=cc
# CC=/opt/fil/bin/filcc
DIST_NAME ?= fiorino
DIST_PLATFORM ?= linux-x86_64
PREFIX ?= /usr/local
DESTDIR ?=

# https://www.gnu.org/software/make/manual/html_node/How-Make-Works.html
all: lib/libfiorino.so

cli: build/fiorino-encrypt build/fiorino-decrypt

sdist:
	mkdir -p dist
	tar -czf dist/$(DIST_NAME).tar.gz \
		--transform='s,^,$(DIST_NAME)/,' \
		Makefile README.md DESIGN.md CHECKPOINTS.md src tests images

dist: all cli
	mkdir -p dist
	tar -czf dist/$(DIST_NAME)-$(DIST_PLATFORM).tar.gz \
		--transform='s,^,$(DIST_NAME)/,' \
		README.md DESIGN.md CHECKPOINTS.md \
		build/fiorino-encrypt build/fiorino-decrypt lib/libfiorino.so

install: all cli
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/lib \
		$(DESTDIR)$(PREFIX)/share/doc/fiorino
	install -m 0755 build/fiorino-encrypt build/fiorino-decrypt \
		$(DESTDIR)$(PREFIX)/bin/
	install -m 0644 lib/libfiorino.so $(DESTDIR)$(PREFIX)/lib/
	install -m 0644 README.md DESIGN.md CHECKPOINTS.md \
		$(DESTDIR)$(PREFIX)/share/doc/fiorino/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/fiorino-encrypt \
		$(DESTDIR)$(PREFIX)/bin/fiorino-decrypt \
		$(DESTDIR)$(PREFIX)/lib/libfiorino.so
	rm -f $(DESTDIR)$(PREFIX)/share/doc/fiorino/README.md \
		$(DESTDIR)$(PREFIX)/share/doc/fiorino/DESIGN.md \
		$(DESTDIR)$(PREFIX)/share/doc/fiorino/CHECKPOINTS.md
	rmdir $(DESTDIR)$(PREFIX)/share/doc/fiorino 2>/dev/null || true

build lib:
	mkdir -p $@

build/%.o: src/%.c src/%.h src/fiorino_log.h | build
	$(CC) $(CCFLAGS) -fPIC -c -o $@ $<

lib/libfiorino.so: build/fiorino.o build/encrypt.o build/bundle.o build/fiorino_log.o | lib
	$(CC) -shared -o $@ $^ -lcrypto

lib/fiorino.a: build/fiorino.o build/encrypt.o build/bundle.o build/fiorino_log.o | lib
	ar rcs $@ $^

build/fiorino-decrypt.o: src/fiorino-decrypt.c
	$(CC) $(CCFLAGS) -c -o $@ $<

build/fiorino-decrypt: lib/libfiorino.so build/fiorino-decrypt.o
	$(CC) $(CCFLAGS) -o $@ build/fiorino-decrypt.o -L$(CURDIR)/lib -lfiorino -Wl,-rpath,'$$ORIGIN/../lib'

build/fiorino-encrypt.o: src/fiorino-encrypt.c
	$(CC) $(CCFLAGS) -c -o $@ $<

build/fiorino-encrypt: lib/libfiorino.so build/fiorino-encrypt.o
	$(CC) $(CCFLAGS) -o $@ build/fiorino-encrypt.o -L$(CURDIR)/lib -lfiorino -Wl,-rpath,'$$ORIGIN/../lib'

build/test_cli_usage: $(CLI_TESTS) tests/cli_usage.sh | build
	@bash tests/cli_usage.sh

build/test_cli_fuzz: $(CLI_TESTS) tests/cli_fuzz.sh | build
	@bash tests/cli_fuzz.sh

build/test_cli_roundtrip: $(CLI_TESTS) tests/cli_roundtrip.c | build
	$(CC) $(CCFLAGS) -o $@ tests/cli_roundtrip.c -lutil

build/test_encrypt_gcm: tests/encrypt_gcm.c src/encrypt.c src/encrypt.h src/bundle.h src/fiorino_log.c src/fiorino_log.h | build
	$(CC) $(CCFLAGS) -o $@ tests/encrypt_gcm.c src/encrypt.c src/fiorino_log.c -lcrypto

build/test_bundle: tests/bundle.c src/bundle.c src/bundle.h src/fiorino.h src/fiorino_log.c src/fiorino_log.h | build
	$(CC) $(CCFLAGS) -o $@ tests/bundle.c src/bundle.c src/fiorino_log.c

build/test_encrypt_file_cleanup: tests/encrypt_file_cleanup.c lib/libfiorino.so | build
	$(CC) $(CCFLAGS) -o $@ $< -L$(CURDIR)/lib -lfiorino -Wl,-rpath,$(CURDIR)/lib

build/test_fiorino_log: tests/fiorino_log.c src/fiorino_log.c src/fiorino_log.h | build
	$(CC) $(CCFLAGS) -o $@ tests/fiorino_log.c src/fiorino_log.c

build/test_fiorino_roundtrip: tests/fiorino_roundtrip.c lib/libfiorino.so | build
	$(CC) $(CCFLAGS) -o $@ $< -L$(CURDIR)/lib -lfiorino -Wl,-rpath,$(CURDIR)/lib

build/test_fiorino_limits: tests/fiorino_limits.c lib/libfiorino.so | build
	$(CC) $(CCFLAGS) -o $@ $< -L$(CURDIR)/lib -lfiorino -Wl,-rpath,$(CURDIR)/lib

build/test_fiorino_symlink: tests/fiorino_symlink.c lib/libfiorino.so | build
	$(CC) $(CCFLAGS) -o $@ $< -L$(CURDIR)/lib -lfiorino -Wl,-rpath,$(CURDIR)/lib

test: $(TEST_BINS) build/test_cli_usage build/test_cli_fuzz build/test_cli_roundtrip
	./build/test_encrypt_gcm
	./build/test_bundle
	./build/test_encrypt_file_cleanup
	./build/test_fiorino_log
	./build/test_fiorino_limits
	./build/test_fiorino_roundtrip
	./build/test_fiorino_symlink
	./build/test_cli_roundtrip

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

.PHONY: all cli sdist dist install uninstall clean test memtest
