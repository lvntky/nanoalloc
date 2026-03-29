# nanoalloc

[![CI](https://github.com/lvntky/nanoalloc/actions/workflows/ci.yml/badge.svg)](https://github.com/lvntky/nanoalloc/actions/workflows/ci.yml)

**nanoalloc** (or `na_alloc`) is a general-purpose dynamic memory allocator for Linux. It's a from-scratch reimplementation of `malloc`, designed not to be the fastest allocator out there, but to actually work — replacing glibc's allocator via `LD_PRELOAD` and running real programs like Python, curl, and Vim.

## How It Works

nanoalloc manages memory through a circular doubly-linked free list of chunks. Each chunk carries metadata (size, magic sentinel, forward/backward pointers) followed by the user's data. The allocator follows a simple strategy:

1. **Allocation** — Walk the free list for a chunk that fits (first-fit). If nothing is available, request fresh pages from the kernel via `mmap`.
2. **Deallocation** — Mark the chunk as free and coalesce it with adjacent free neighbors to reduce fragmentation.
3. **Reallocation** — Allocate new space, copy the data, free the old chunk.
4. **Calloc** — Allocate and zero-fill. Pages from `mmap` are already zeroed by the kernel, but `memset` is applied for correctness on recycled chunks.

All operations are guarded by a global mutex for thread safety. Initialization is lazy and happens exactly once via `pthread_once`.

### Chunk Layout

```
┌──────────┬──────────┬──────────┬──────────┐
│  magic   │   size   │    fc    │    bc    │  ← chunk header (na_chunk)
├──────────┴──────────┴──────────┴──────────┤
│                 user data                 │  ← pointer returned to caller
└───────────────────────────────────────────┘
```

- **magic** — `0xDEADC0DEDEADC0DE` for live chunks, `0xF733DC0DEF733DC0` after free. Used to detect double-frees and foreign pointers.
- **size** — Total chunk size with the lowest bit used as an in-use flag.
- **fc / bc** — Forward and backward pointers forming the circular free list.

## The Hook

nanoalloc exports `malloc`, `free`, `realloc`, and `calloc` symbols directly, so you can inject it into any dynamically-linked program with `LD_PRELOAD`:

```bash
LD_PRELOAD=./libnanoalloc.so python3 -c "print('hello from nanoalloc')"
LD_PRELOAD=./libnanoalloc.so curl https://example.com
LD_PRELOAD=./libnanoalloc.so vim
```

## Building

```bash
# clone and build
git clone https://github.com/lvntky/nanoalloc.git
cd nanoalloc
make
```

Compile with `NA_DEBUG=1` to enable verbose allocation tracing and free-list integrity checks to stderr.

## Limitations

- **Single global lock** — all threads contend on one mutex. No per-thread caches (tcache) yet.
- **No chunk splitting** — if a large free chunk satisfies a small request, the entire chunk is used. Internal fragmentation can be significant.
- **No return-to-kernel** — freed memory is coalesced but not yet unmapped back to the OS (the `munmap` path exists but is disabled for stability).
- **First-fit traversal** — free list search is O(n). No binning, no best-fit, no size-class segregation.
- **No prev_size field** — physical neighbor coalescing relies on list adjacency, not on boundary tags.

Using nanoalloc in everyday applications will likely cause slowdowns compared to glibc. It may also crash on programs that depend on allocator behaviors nanoalloc doesn't replicate. That said — it works, and that's the point.

## References

- [dlmalloc](https://github.com/ennorehling/dlmalloc/tree/master) — Doug Lea's classic allocator
- [glibc malloc (ptmalloc2)](https://sourceware.org/glibc/) — the allocator nanoalloc is spiritually influenced by
- [Paul R. Wilson — Dynamic Storage Allocation: A Survey and Critical Review (1995)](https://www.cs.hmc.edu/~oneill/gc-library/Wilson-Alloc-Survey-1995.pdf)

## Code of Conduct

See [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)

## License

MIT. Have fun.
