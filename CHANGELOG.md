# Change Log

## [Unreleased]
- Add file name to VFS file
- Add file support for version 3
- Add URI parameter support to crypt file for `key` and `cipher`

## [0.3.0] - 2020-10-01
### Added
- Re-encryption with the new `PRAGMA`s `crypt_transformation` and `chipher`
- Multiple key format support with `PRAGMA key="r'my raw key'"` or `PRAGMA key="x'070617373776f7264313233'"`

### Changed
- Error code from `generic` to `not_a_database` when decryption of database fails

### Removed
- `PRAGMA plain_key`

## [0.2.0] - 2020-09-29
### Added
- Page transforming file
- Cryptographic functions: `sha1`, `sha2`, `sha3`, `md5` and `digest`
- Crypt VFS for main DB + extension
- Encryption example

### Removed
- Layered VFS

## [0.1.0] - 2020-09-22
### Added
- [ROT13](https://en.wikipedia.org/wiki/ROT13) example

### Changed
- Upgraded SQLite3 to v3.33.0
- Minimum C++ version from `14` to `11`
- All headers and sources into one directory (`ysqlite3`)
- Error reporting with `std::error_code` and `std::system_error` instead of custom exceptions

### Removed
- `backward.hpp` 3rd party
- `microsoft/gsl` 3rd party

[Unreleased]: https://github.com/terrakuh/ysqlite3/compare/v0.3.0...dev
[0.3.0]: https://github.com/terrakuh/ysqlite3/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/terrakuh/ysqlite3/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/terrakuh/ysqlite3/compare/v0.0.0...v0.1.0
