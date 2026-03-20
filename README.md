# nanoalloc

nanoalloc __(or na_alloc)__ is a general purpose dynamic memory allocator. Basically a poor re-inventation of wheel that ultimately goals to work in production (even if it's slow).

Nanoalloc's primary goal isn't speed or offering a groundbreaking perspective. Its sole purpose is to function. While no performance promises are made, its main requirement is acceptable performance in commonly used modern programs like Python, curl, and Vim.

Although significantly influenced by malloc/ptmalloc used in glibc, nanoalloc is not a copy of them and therefore does not offer the same functionality and security as those libraries. While nanoalloc is not a 'toy memory allocator' because it is designed for modern programs to perform memory allocation, its use in everyday applications will likely cause slowdowns or crashes.

## The Hook

As mentioned, the primary purpose of nanoalloc is to function and execute. To verify this, smoke tests show that a simple LD_PRELOAD hack generates a hook dynamic library. This library is loaded before the program runs and replaces the program's existing malloc with nanoalloc.

## Referances

- [dlmalloc](https://github.com/ennorehling/dlmalloc/tree/master)
- [glibc's source code]()
- [Paul R. Wilson - Alloc Survey '95](https://www.cs.hmc.edu/~oneill/gc-library/Wilson-Alloc-Survey-1995.pdf)

## Code of Conduct

See [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)

## License

MIT, Have fun.