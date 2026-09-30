# FiorinoPrego - ICS0022 File Manager

An encrypted file manager that [won't let you pass unnoticed](https://www.youtube.com/watch?v=te7XM8RCfxQ).

> [!IMPORTANT]
> This project is not implemented yet.

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

## Logging

Fiorino sends messages through `syslog(3)`. On Fedora, journald receives them.
Messages are prefixed with `fiorino:` and use the calling program's journal
identity. The default threshold is `FIORINO_LOG_WARNING`.
Warnings report rejected inputs and paths the caller can correct. Errors
report failures while setting up or writing vault data. An `errno` value
provides detail about the failure; it does not determine its severity.

```c
#include <fiorino_log.h>

fiorino_log_set_threshold(FIORINO_LOG_DEBUG); /* include debug messages */
fiorino_log_set_threshold(FIORINO_LOG_OFF);   /* silence Fiorino */
```

The threshold is process-wide for Fiorino. The host program's syslog settings
can further restrict messages. To inspect messages on a journald system:

```bash
journalctl --grep='fiorino:'
```
