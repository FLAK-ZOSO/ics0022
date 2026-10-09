# Progress note

- Checkpoint 1: complete.
- Encryption library: implemented and tested with AES-256-GCM.
- Decryption library: implemented and tested.
- Bundle serialization: implemented and tested.
- Password prompting: wrapped by Fiorino’s API.
- Stream-based encryption API: named fiorino_encrypt_stream.
- Path and symlink validation: implemented and tested.
- Logging: implemented.
- CLI: in progress; encrypt and decrypt commands are partially implemented.

## Before checkpoint 2
- finish CLI
- add CLI round-trip test (both valid and invalid input)
- document clean build and usage in the README

## Todo
- secure deletion
- signal handling
- file/stream size/read limits
- complete access-control review
- atomic replacement
- final validation tests
- report
