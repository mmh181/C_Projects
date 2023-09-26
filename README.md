# Simple Memory Allocator in C

This repository contains a simple memory allocator implemented in C. The memory allocator provides basic functionality similar to the standard `malloc`, `calloc`, `realloc`, and `free` functions.

## Overview

The memory allocator uses a linked list to keep track of free blocks of memory. Each block has a header that contains the size of the block, a flag indicating whether the block is free, and a pointer to the next block. The allocator uses the `sbrk` system call to increase the program break and allocate more memory when necessary.

The allocator is thread-safe and uses a mutex to protect the global variables from being accessed by multiple threads at the same time.

## Functions

- `malloc(size_t size)`: Allocates a block of memory of the specified size. If there is a free block in the list that is large enough, it will be used. Otherwise, more memory will be allocated using `sbrk`.

- `calloc(size_t num, size_t nsize)`: Allocates memory for an array of `num` elements, each of size `nsize`. The memory is initialized to zero.

- `realloc(void *block, size_t size)`: Changes the size of the memory block pointed to by `block` to `size` bytes. The contents will be unchanged to the minimum of the old and new sizes.

- `free(void *block)`: Deallocates the memory previously allocated by `malloc`, `calloc`, or `realloc`.

## Compilation

To compile the program, use the following command:

```bash
gcc -o mem_alloc mem_alloc.c -lpthread
```

The `-lpthread` option is necessary to link against the POSIX threads library.

## Execution

After successful compilation, you can run the program using the following command:

```bash
./mem_alloc
```

## Testing

The main function in the code tests all four functions (`malloc`, `calloc`, `realloc`, and `free`). It first allocates an array of 10 integers using `malloc`, fills it with values, and prints it. It then reallocates the array to hold 20 integers using `realloc`, fills the new elements with values, and prints it. It also allocates an array of 10 integers using `calloc` and prints it (the values should all be zero because `calloc` initializes the memory to zero). Finally, it frees both arrays using `free`.

Please note that this is a simple test and does not cover all edge cases. Always thoroughly test your code before using it in a production environment.
