# FiorinoPrego - ICS0022 File Manager

An encrypted file manager that [won't let you pass unnoticed](https://www.youtube.com/watch?v=te7XM8RCfxQ).

> [!IMPORTANT]
> This project is not implemented yet.

## Concepts

### Vault

A vault is essentially a subdirectory where encrypted files are stored all under the same primary key.

```
~/.fiorino/default/file1.txt.enc
~/.fiorino/default/keys/file1.txt.key
~/.fiorino/default/file2.txt.enc
~/.fiorino/default/keys/file2.txt.key
~/.fiorino/vault2/file3.txt.enc
~/.fiorino/vault2/keys/file3.txt.key
~/.fiorino/vault2/file4.txt.enc
~/.fiorino/vault2/keys/file4.txt.key
```

In this example, the first two files are encrypted under one primary key (well, their keys are encrypted with the primary key), and the other ones are under a second key.

```bash
fiorino decrypt --vault vault2 --files file3.txt file4.txt
```

### 

## Platforms

Only Linux systems are supported.

> [!NOTE]
> Support for other POSIX-adjacent platforms is a naturally consequent feature.

## Build

> [!IMPORTANT]
> This project is not implemented yet, you just cannot compile it now.

> [!NOTE]
> I may want to use autoconf/automake in order to port this to other platforms.

```bash
make
```
