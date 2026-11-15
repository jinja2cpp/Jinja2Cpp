---
status: open
priority: low
area: robustness
depends: [0231]
touches: [src/hash_map.h, src/hash_map.cpp]
---
# Host keys of raw bytes can collide in the name hash whatever its per-process key

**Problem.** Left by the security review of #891 (0231). `HashedName::Hash` keys unordered_dense's
unseeded `hash_bytes` per process with a bijective finalizer (`detail::HashName`, src/hash_map.h),
which spreads names that only share part of the unkeyed hash, the attack the review measured (about
90x per step with 12,000 crafted template names). It cannot separate names whose unkeyed hashes are
equal. `hash_bytes` has wyhash's multiply-by-zero weakness: for a key of 8 to 16 bytes it returns
`mix(secret[1] ^ len, mix(a ^ secret[1], b ^ seed))` with `a` the first eight bytes, so every key of one
length whose first eight bytes equal `secret[1]` (`db 28 b4 a0 d1 7e 03 e7`) hashes alike, and the
longer paths have the same lanes. Template names cannot hold those bytes (not identifier characters,
not valid UTF-8), but a host map passed as render params can, so a host that forwards untrusted keys
unchecked gives every such key one group: each lookup scans them all. robin_hood's MurmurHash-based
hash on master had seed-independent collisions of its own, so this is not a regression of 0231.

**Proposal.** Put the process key into the hash itself rather than after it: either xor it into the
lane inputs (a local hash function in src/hash_map.h over the vendored reads, so `src/ankerl/` stays
upstream), or hash names with a keyed function such as SipHash-1-3 or rapidhash with a secret derived
from the key, measured with bench/count.py against the per-node render path (the parse-time hash is
off it, rehashes and string-keyed lookups are not).

**Done when.** A test builds 4,000 params keys of 16 bytes sharing the first eight `secret[1]` bytes
and a render reading them runs within a small factor of the same render with ordinary keys, and the
bench rows stay within the perf bar.
