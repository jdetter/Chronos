#ifndef _MMAN_H_
#define _MMAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAP_TYPE        0x0f    /* Mask that determines the type of map */
#define MAP_SHARED      0x01    /* Share this mapping (fork) */
#define MAP_PRIVATE     0x02    /* Set mapping to Copy on Write */
#define MAP_FIXED       0x10    /* Do not take addr as a hint. */
#define MAP_ANONYMOUS   0x20    /* Mapping is not backed by a file */
#define MAP_32BIT       0x40    /* Only map 32bit addresses */
#define MAP_ANON        MAP_ANONYMOUS /* DEPRICATED */
#define MAP_FILE        0x00    /* DEPRICATED */
#define MAP_GROWSDOWN   0x00100 /* Used for stacks. */
#define MAP_DENYWRITE   0x00800 /* DEPRICATED */
#define MAP_EXECUTABLE  0x01000 /* DEPRICATED */
#define MAP_LOCKED      0x02000 /* Lock the page with mlock. */
#define MAP_NORESERVE   0x04000 /* Do not reserve swap space for this mapping. */
#define MAP_POPULATE    0x08000 /* Prepopulate the tlb. */
#define MAP_NONBLOCK    0x10000 /* Used with populate. */
#define MAP_STACK       0x20000 /* Marks allocation as a stack. */
#define MAP_HUGETLB     0x40000 /* Allocate huge pages. */
#define MAP_UNINITIALIZED 0x0   /* Disabled in Chronos. */

/**
 * Protectections for mmap and mprotect
 */
#define PROT_NONE       0x00
#define PROT_READ       0x01
#define PROT_WRITE      0x02
#define PROT_EXE        0x04
#define PROT_GROWSDOWN  0x01000000
#define PROT_GROWSUP    0x02000000


/**
 * Map memory into user memory.
 */
void* mmap(void* addr, size_t sz, int prot,
                int flags, int fd, off_t offset);

/**
 * Unmap a region in memory. This does not close the mapped file if there
 * is a mapped file. Returns 0 on success, -1 otherwise.
 */
int munmap(void* addr, size_t length);



#ifdef __cplusplus
}
#endif

#endif
