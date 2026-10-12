# FiorinoPrego - ICS0022 File Manager

An encrypted file manager that [won't let you pass unnoticed](https://www.youtube.com/watch?v=te7XM8RCfxQ).

## Concepts

### Vault

A vault is essentially a subdirectory where encrypted files are stored all under the same primary key.

```
~/.fiorino/default/file1.txt.enc
~/.fiorino/default/file2.txt.enc
~/.fiorino/vault2/file3.txt.enc
~/.fiorino/vault2/file4.txt.enc
```

In this example, the first two files are encrypted under one primary key, and the other ones are under a second key.

## Platforms

Only Linux systems are supported.

> [!NOTE]
> Support for other POSIX-adjacent platforms is a naturally consequent feature.

## Build and usage

Linux and OpenSSL development headers are required.

### Dependencies

Build dependencies:

- Fedora: `make`
- Debian: `make`

Compile dependencies:

- Fedora: `gcc glibc-devel openssl-devel`
- Debian: `gcc libc6-dev libssl-dev`

Runtime dependencies:

- Fedora: `glibc openssl-libs`
- Debian: `libc6 libssl3` (or the current OpenSSL 3 runtime package)

Test dependencies:

- Fedora: `bash valgrind python3`
- Debian: `bash valgrind python3`

### CLI only

```bash
make
make cli
```

Encrypt a file into a vault. The password is entered twice:

```bash
./build/fiorino-encrypt -v default -f file.txt
```

Decrypt a vault file into the current directory. The password is entered once:

```bash
./build/fiorino-decrypt -v default -f file.txt
```

> [!WARNING]
> Don't rely on neither return codes nor behaviour to reflect received signals, as calls to OpenSSL and other libraries can overwrite the fiorino CLI handlers.

Run the test suite with:

```bash
make test
```

### Full installation

> [!WARNING]
> GNU Make targets for installation are not available yet, and some twists to rpaths and include directories will be needed by then.
