#include <unistd.h> // defines miscellaneous symbolic constants and types, and declares miscellaneous functions.
#include <string.h> // defines several functions that convert between strings and numbers, and functions that create, scan, copy and compare strings.
#include <pthread.h> // defines functions and structures related to multi-threaded programming.
#include <stdio.h> // defines three variable types, several macros, and various functions for performing input and output.

typedef char ALIGN[16]; // define a new type ALIGN as an array of 16 characters.

union header 
{ // define a union header
    struct { // define a structure
        size_t size; // size_t is an unsigned integer type of at least 16 bit used to represent the size of an object.
        unsigned is_free; // unsigned is an integer data type that is at least 2 bytes long.
        union header *next; // define a pointer to a union header
    } s; // define a structure s inside the union header called s that has a size_t, an unsigned, and a pointer to a union header as its members.
    // align the header to 16 bytes
    ALIGN stub; // define a stub
};

typedef union header header_t; // define a new type header_t as a union header

header_t *head = NULL, *tail = NULL; // define two pointers to header_t
pthread_mutex_t global_malloc_lock; // define a mutex variable global_malloc_lock to protect the global variables head and tail from being accessed by multiple threads at the same time.

header_t *get_free_block(size_t size)
{ // define a function get_free_block that takes a size_t as an argument and returns a pointer to a header_t
    
    header_t *curr = head; // define a pointer to header_t and initialize it to head
    while(curr) { // while curr is not NULL
        if (curr->s.is_free && curr->s.size >= size) // if curr is free and curr's size is greater than or equal to size
            return curr; // return curr
        curr = curr->s.next; // set curr to curr's next
    }
    return NULL; // return NULL
}

void free(void *block)
{ // define a function free that takes a pointer to void as an argument and returns nothing
    header_t *header, *tmp; // define two pointers to header_t
    void *programbreak; // define a pointer to void

    if (!block) // if block is NULL
        return; // return
    pthread_mutex_lock(&global_malloc_lock); // lock the mutex global_malloc_lock
    header = (header_t*)block - 1; // set header to block - 1
    programbreak = sbrk(0); // set programbreak to the current program break. sbrk(0) gives the current program break address.
    
    /*
	   Check if the block to be freed is the last one in the
	   linked list. If it is, then we could shrink the size of the
	   heap and release memory to OS. Else, we will keep the block
	   but mark it as free.
	 */

    if ((char*)block + header->s.size == programbreak) 
    { // if block + header's size is equal to programbreak
        if (head == tail) { // if head is equal to tail
            head = tail = NULL; // set head and tail to NULL
        } else { // otherwise
            tmp = head; // set tmp to head
            while (tmp) { // while tmp is not NULL
                if(tmp->s.next == tail) { // if tmp's next is equal to tail
                    tmp->s.next = NULL; // set tmp's next to NULL
                    tail = tmp; // set tail to tmp
                }
                tmp = tmp->s.next; // set tmp to tmp's next
            }
        }

        /*
		   sbrk() with a negative argument decrements the program break.
		   So memory is released by the program to OS.
		*/

        sbrk(0 - header->s.size - sizeof(header_t)); // set the program break to the current program break - header's size - the size of a header_t

        /* Note: This lock does not really assure thread
		   safety, because sbrk() itself is not really
		   thread safe. Suppose there occurs a foregin sbrk(N)
		   after we find the program break and before we decrement
		   it, then we end up realeasing the memory obtained by
		   the foreign sbrk().
		*/

        pthread_mutex_unlock(&global_malloc_lock); // unlock the mutex global_malloc_lock
        return; // return
    }
    header->s.is_free = 1; // set header's is_free to 1
    pthread_mutex_unlock(&global_malloc_lock); // unlock the mutex global_malloc_lock
}

void *malloc(size_t size)
{
    size_t total_size; // define a size_t total_size
    void *block; // define a pointer to void
    header_t *header; // define a pointer to header_t

    if (!size) // if size is 0
        return NULL; // return NULL
    pthread_mutex_lock(&global_malloc_lock); // lock the mutex global_malloc_lock
    header = get_free_block(size); // set header to the return value of get_free_block(size)
    if (header) { // if header is not NULL
        header->s.is_free = 0; // set header's is_free to 0
        pthread_mutex_unlock(&global_malloc_lock); // unlock the mutex global_malloc_lock
        return (void*)(header + 1); // return header + 1
    }
    total_size = sizeof(header_t) + size; // set total_size to the size of a header_t + size
    block = sbrk(total_size); // set block to the current program break
    if (block == (void*) -1) { // if block is equal to -1
        pthread_mutex_unlock(&global_malloc_lock); // unlock the mutex global_malloc_lock
        return NULL; // return NULL
    }
    header = block; // set header to block
    header->s.size = size; // set header's size to size
    header->s.is_free = 0; // set header's is_free to 0
    header->s.next = NULL; // set header's next to NULL
    if (!head) // if head is NULL
        head = header; // set head to header
    if (tail) // if tail is not NULL
        tail->s.next = header; // set tail's next to header
    tail = header; // set tail to header
    pthread_mutex_unlock(&global_malloc_lock); // unlock the mutex global_malloc_lock
    return (void*)(header + 1); // return header + 1
}

void *calloc(size_t num, size_t nsize)
{
    size_t size; // define a size_t size
    void *block; // define a pointer to void

    if (!num || !nsize) // if num or nsize is 0
        return NULL; // return NULL
    size = num * nsize; // set size to num * nsize
    /* check mul overflow */
    if (nsize != size / num) // if nsize is not equal to size / num
        return NULL; // return NULL
    block = malloc(size); // set block to the return value of malloc(size)
    if (!block) // if block is NULL
        return NULL; // return NULL
    memset(block, 0, size); // set block to 0
    return block; // return block
}

void *realloc(void *block, size_t size)
{
    header_t *header; // define a pointer to header_t
    void *ret; // define a pointer to void
    if (!block || !size) // if block or size is 0
        return malloc(size); // return the return value of malloc(size)
    header = (header_t*)block - 1; // set header to block - 1
    if (header->s.size >= size) // if header's size is greater than or equal to size
        return block; // return block
    ret = malloc(size); // set ret to the return value of malloc(size)
    if (ret) { // if ret is not NULL
        memcpy(ret, block, header->s.size); // copy block to ret
        free(block); // free block
    }
    return ret; // return ret
}

void print_mem_list()
{
    header_t *curr = head; // define a pointer to header_t and initialize it to head
    printf("head = %p, tail = %p \n", (void*)head, (void*)tail); // print the addresses of head and tail
    while(curr) { // while curr is not NULL
        printf("addr = %p, size = %zu, is_free=%u, next=%p\n", (void*)curr, curr->s.size, curr->s.is_free, (void*)curr->s.next); // print the address of curr, curr's size, curr's is_free, and curr's next
        curr = curr->s.next; // set curr to curr's next
    }
}

int main() {
    // Test malloc
    int *arr = (int*) malloc(10 * sizeof(int));
    if (arr == NULL) {
        printf("my_malloc failed\n");
        return -1;
    }

    // Use the array
    for (int i = 0; i < 10; i++) {
        arr[i] = i;
    }

    // Print the array
    for (int i = 0; i < 10; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    // Test realloc
    arr = (int*) realloc(arr, 20 * sizeof(int));
    if (arr == NULL) {
        printf("my_realloc failed\n");
        return -1;
    }

    // Use the reallocated array
    for (int i = 10; i < 20; i++) {
        arr[i] = i;
    }

    // Print the reallocated array
    for (int i = 0; i < 20; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    // Test calloc
    int *arr2 = (int*) calloc(10, sizeof(int));
    if (arr2 == NULL) {
        printf("my_calloc failed\n");
        return -1;
    }

    // Print the calloc'ed array
    for (int i = 0; i < 10; i++) {
        printf("%d ", arr2[i]);
    }
    printf("\n");

    // Test free
    free(arr);
    
	// Test double free
	free(arr); 

	// Test freeing NULL pointer
	int *null_ptr = NULL;
	free(null_ptr);

	// Test zero allocation
	int *zero_alloc = (int*) malloc(0);
	if(zero_alloc != NULL) {
		printf("Zero allocation failed\n");
		return -1;
	}

	// Test large allocation
	int *large_alloc = (int*) malloc(1e9);
	if(large_alloc != NULL) {
		printf("Large allocation failed\n");
		return -1;
	}

	free(arr2);

	return 0;
}