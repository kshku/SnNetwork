# Changelog

## [0.1.3] - 2026-10-02

### Changed
- Take sncore v0.3.2 rather than v0.3.1

## [0.1.2] - 2026-10-02

### Changed
- -Wconversion and -Wsign-conversion are on for gcc and clang. The code was
  already clean of both

## [0.1.1] - 2026-09-28

### Changed
- Take sncore v0.3.1 rather than v0.2.0

All notable changes to this project will be documented in this file.

## [0.1.0] - 2026-07-05

### Added
- Cross-platform TCP client/server API (`SnTcpSocket`)
- Cross-platform UDP client/server API (`SnUdpSocket`)
- Network endpoint abstraction (`SnEndPoint`) with IPv4/IPv6 support
- IP address parsing from/to string (`sn_end_point_from_string`, `sn_end_point_to_string`)
- POSIX (Linux/macOS) and Windows (Win32) platform backends for all socket types
- Socket options: TCP nodelay, send/receive timeout, keepalive
- Win32 WSAStartup/WSACleanup lifecycle via `sn_network_init`/`sn_network_deinit`
- CI/CD workflows: build & test on Ubuntu (GCC), macOS (Clang), Windows (MSVC)

### Fixed
- macOS/BSD `sin_len` field causing incorrect address family detection
