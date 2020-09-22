# Change Log

## Unreleased
- [ROT13](https://en.wikipedia.org/wiki/ROT13) example

### Changed
- Upgraded SQLite3 to v3.33.0
- Minimum C++ version from `14` to `11`
- All headers and sources into one directory (`src`)
- Error reporting with `std::error_code` and `std::system_error` instead of custom exceptions

### Removed
- `backward.hpp` 3rd party
- `microsoft/gsl` 3rd party
