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
> The code uses POSIX.1-2008 interfaces, but also uses `flock()` for advisory
> file locking. `flock()` is a BSD/Linux interface, not part of POSIX.1-2008;
> supporting other systems would require replacing or adapting that locking
> code and validating the result.

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

`-f` accepts a path, so `-f report/main.pdf` reads that file and stores it in
the vault as `main.pdf.enc`. Vaults are flat: only the input basename is kept,
so another input with the same basename replaces that encrypted entry.

Decrypt a vault file into the current directory. The password is entered once:

```bash
./build/fiorino-decrypt -v default -f file.txt
```

> [!WARNING]
> Don't rely on either return codes or behaviour to reflect received signals, as calls to OpenSSL and other libraries can overwrite the fiorino CLI handlers.

Run the test suite with:

```bash
make test
```

### Container

> [!WARNING]
> This part of the project is not part of the project, it was slopped on the last moment just for ease of testing by the lecturer.

Build the image and start a shell with your current directory and Fiorino vault mounted:

```bash
docker build -t fiorino .
mkdir -p "$HOME/.fiorino"
docker run --rm -it --user "$(id -u):$(id -g)" \
  -e HOME=/home/fiorino -v "$PWD:/work" \
  -v "$HOME/.fiorino:/home/fiorino/.fiorino" \
  fiorino
```

Then, inside the container, try the commands in sequence:

```bash
echo 'container test' > sample.txt
fiorino-encrypt -v container-test -f sample.txt
fiorino-list -v container-test
mkdir output && cd output
fiorino-decrypt -v container-test -f sample.txt
fiorino-delete -v container-test -f sample.txt
exit
```

### Full installation

```bash
make install-strip
```

Consult the Makefile for more targets among which `install`.
