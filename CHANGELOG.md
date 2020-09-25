# Change Log

## Unreleased
### Added
- Custom data support to layers

### Changed
- Only transform pages

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

[0.1.0]: https://github.com/terrakuh/ysqlite3/compare/v0.0.0...v0.1.0
