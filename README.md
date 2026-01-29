# NNIST

> **“A modern forensic-grade NIST EBTS toolkit.”**

**NNIST** is a modern C++ library for parsing, validating, editing, and serializing
**NIST EBTS / AN2K biometric records**, designed with a strong focus on:

- Standards correctness
- Deterministic behavior
- Forensic integrity
- Minimal dependencies
- Freestanding compatibility
- Test-driven development

This project is intended for developers building tooling around
**fingerprints, palm prints, mugshots, and biometric identity records**
in compliance with NIST specifications.

---

## Features

- [x] Full **NIST EBTS / AN2K** record parsing
- [ ] Binary image extraction and validation
- [ ] Field-level structured access
- [x] Deterministic round-trip serialization
- [x] Strong input validation and bounds safety
- [x] No heap requirement in compliant builds
- [x] Cross-platform and freestanding-friendly
- [x] Extensive unit tests and fuzz testing

---

## Design Philosophy

NNIST prioritizes:

- **Correctness over convenience**
- **Explicit failure over silent corruption**
- **Structured data over ad-hoc parsing**
- **Test coverage over speculation**

The goal is to provide a **clean, auditable reference implementation**
for NIST biometric record handling.
---
## Building
```bash
cmake -S . -B build
cmake --build build
```
Run Tests
```bash
ctest --test-dir build
```

---
## Status
**Active Development**
API may evolve until v1.0 stability.
---

## License
MIT (see LICENSE)


