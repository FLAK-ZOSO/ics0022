# Remaining work

- Finish validation and write the project report.

## Report notes

- Mirrors and backups were dropped from the design.
- AES-GCM provides integrity through its authentication tag.
- System logging uses syslog; journal sealing can be configured by system administrators on top.
- Free-space preflight checks and a global concurrency limit were not implemented.
- File deletion unlinks the file; secure erasure is not promised.
- Atomic replacement renames a completed temporary file.
- OpenSSL use does not establish FIPS-validated operation.
- SELinux policy, Fil-C, static analysis, and man pages are out of scope.
