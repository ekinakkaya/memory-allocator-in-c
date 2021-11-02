/* virtual address space of a process:
 * - text section: contains binary instructions to be executed by the processor.
 * - data section: contains non-zero initialized static data.
 * - BSS (block started by symbol): contains zero initialized static data.
 * - heap: contains the dynamically allocated data.
 * - stack: contains automatic variables, function arguments, copy of base pointer etc.
 *
 * ----------------------
 * |--------------------|
 * |       stack        |
 * |---------||---------|
 * |         ||         |
 * |         ||         |
 * |         \/         |
 * |                    |
 * |                    |
 * |         /\         |
 * |         ||         |
 * |         ||         |
 * |---------||---------|  <-- brk
 * |       heap         |                          }
 * |--------------------|                           }
 * |       bss          |  uninitialized variables  }  data segment
 * |--------------------|                           }
 * |       data         |  initialized variables   }
 * |--------------------|
 * |       text         |  instructions
 * |--------------------|
 *
 * 
 * brk points to the end of the heap.
 * if we want to allocate more memory on heap, we need to increment brk pointer.
 * if we want to deallocate memory, we need to decrement brk pointer
 *
 * for unix-like systems, we'll use sbrk() system call to manipulate the program break.
 * sbrk() is not thread-safe. only grows and shrinks in LIFO (last in, first out) order.
 * nmap() is a better alternative.
 *
 * (maybe i will implement a nmap version)
 */

#include <unistd.h>
#include <pthread.h>

#include <stdio.h>
#include <string.h>



/* the size of a union is equal to the size of the largest data type used in it.
 * this does exactly what it suggests. it alignes the header to 16 bytes.
 * this guarantees that the end of the header is memory aligned
 */
typedef char ALIGN[16];



union header {
    /* we can't free memory from the middle of the heap, so we will mark it as free or not free.
     * this header will be added to every newly allocated mem block.
     */
    struct {
        size_t size;
        unsigned is_free;
    
        /* we can't know if the next memory block is actually next in the memory.
         * to keep track, we will use linked lists. very convenient...
         */
        union header *next;
    } s;

    ALIGN stub;
};

typedef union header header_t;


header_t *head, *tail;


/* global lock
 * before every action on memory, acquire the lock;
 * once done with it, release the lock.
 */
pthread_mutex_t global_malloc_lock;



header_t *get_free_block(size_t size) {
    header_t *curr = head;

    while (curr) {
        if (curr->s.is_free && curr->s.size >= size)
            return curr;

        curr = curr->s.next;
    }
    
    return NULL;
}

void *malloc(size_t size) {
    size_t total_size;
    void *block;
    header_t *header;

    /* if requested size is zero, return NULL. */
    if (!size) return NULL;

    /* acquire the lock */
    pthread_mutex_lock(&global_malloc_lock);

    /* checks if a free block with sufficient size exists.
     * if such a block exists, mark it as not free, release the lock, return a ptr to that block
     */
    header = get_free_block(size);
    if (header) {
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void *) (header + 1);
    }

    /* if no available block found, extend the heap by calling sbrk() */
    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);
    if (block == (void *) -1) {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    /* mark header as not free, set the size, set the next header as NULL */
    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;


    /* if the global head is null, set the head to the header allocated */
    if (!head)
        head = header;
    if (tail)
        tail->s.next = header;

    tail = header;

    pthread_mutex_unlock(&global_malloc_lock);
    return (void *) (header + 1);
}



void free(void *block) {
    header_t *header, *tmp;
    void *programbreak;

    if (!block)
        return;

    pthread_mutex_lock(&global_malloc_lock);
    /* the header is behind by 1 unit. */
    header = (header_t *) block - 1;

    /* get the current value of the program break */
    programbreak = sbrk(0);

    /* check if the end of the current block is the end of the heap
     * if it is, shrink the size of the heap 
     * if not, just mark it as free bruh
     */
    if ( (char *) block + header->s.size == programbreak ) {
        if (head == tail)
            head = tail = NULL;
        else {
            tmp = head;
            while (tmp) {
                if (tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }

        sbrk(0 - sizeof(header_t) - header->s.size);
        pthread_mutex_unlock(&global_malloc_lock);

        return;
    }

    header->s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_lock);
}


/* allocate for an array of num elemens of nsize bytes each */
void *calloc(size_t num, size_t nsize) {
    size_t size;
    void *block;

    if (!num || ! nsize)
        return NULL;

    /* calculate size and sanity check */
    size = num * nsize;
    if (nsize != size / num)
        return NULL;

    /* call malloc and check NULLity */
    block = malloc(size);
    if (!block)
        return NULL;

    /* set all the memory to zero */
    memset(block, 0, size);
    return block;
}


/* reallocate memory for the block */
void *realloc(void *block, size_t size) {
    header_t *header;
    void *ret;

    /* if the block is NULL or the size is zero, call malloc */
    if (!block || !size)
        return malloc(size);

    /* if the size from header is bigger than the requested size, do nothing */
    header = (header_t *) block - 1;
    if (header->s.size >= size)
        return block;

    /* if the requested size is bigger than the current size, memcpy the block to ret,
     * free the block, return the new block
     */
    ret = malloc(size);
    if (ret) {
        memcpy(ret, block, header->s.size);
        free(block);
    }
    return ret;
}



// TODO
// [+] implement calloc
// [+] implement realloc
// [ ] write a few tests



