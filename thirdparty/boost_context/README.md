# boostorg/context fcontext assembly

Unmodified copies of `libs/context/src/asm/{make,jump}_{x86_64_sysv,arm64_aapcs}_{elf,macho}_gas.S`
from the Boost 1.92.0 release (`boost-1.92.0-cmake.tar.xz`, the archive and SHA-256 that
`thirdparty/thirdparty-internal.cmake` pins), under the Boost Software License 1.0 (`LICENSE_1_0.txt`).

`python/src/fiber/fiber_{make,jump}.S` include the one for the target, and `python/src/fiber/fiber.cpp`
calls `make_fcontext` and `jump_fcontext` (docs/tasks/0457). No Boost C++ interface is used. To update,
copy the same files from the newer pinned release and update `sbom/components.json`
(`boost-context-fcontext`) with `scripts/sbom.py`.
