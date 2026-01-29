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

- Full **NIST EBTS / AN2K** record parsing
- Binary image extraction and validation
- Field-level structured access
- Deterministic round-trip serialization
- Strong input validation and bounds safety
- No heap requirement in compliant builds
- Cross-platform and freestanding-friendly
- Extensive unit tests and fuzz testing

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

## Example Usage

```cpp
#include <nnist/nnist.h>

nnnist::Record record = nnnist::ParseFile("sample.an2");

if (record.IsValid()) {
    auto image = record.GetType4Image();
}
```

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


---

# ✅ Suggested License

### 🥇 MIT (simple, permissive)  
or  
### 🥈 Apache-2.0 (better for gov & corporate)

If this might go into **gov / law enforcement ecosystems**, Apache-2.0 looks more “official.”

---

# ✅ Release Announcement Text

**Short version:**

> I’ve open-sourced **NNIST**, a modern C++ library for parsing and validating NIST EBTS / AN2K biometric records.  
> Focused on correctness, deterministic behavior, and forensic-grade integrity.  
> Contributions and audits welcome.

**Long version:**

> Today I’m releasing **NNIST** — a modern C++ library for handling NIST EBTS / AN2K biometric records.  
>  
> It’s designed to be:
> - standards-correct  
> - deterministic  
> - test-driven  
> - freestanding-friendly  
> - safe for forensic and legal workflows  
>  
> The goal is to provide a clean, auditable foundation for biometric tooling and digital forensics.

---
