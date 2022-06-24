/*---INCLUDES ---*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h> // provides a macro called assert which can be used to verify assumptions made by the program 


/*---DEFINES ---*/
#define HEAP_CAP_BYTES 4096

static_assert(HEAP_CAP_BYTES % sizeof(uintptr_t) == 0, "Heap capacity is not divisible by the size of the pointer of the OS. Try a 64 bit OS.");
#define HEAP_CAP_WORDS (HEAP_CAP_BYTES / sizeof(uintptr_t))

#define CHUNK_LIST_CAP 1024

/*---STRUCTS---*/

typedef struct {
    uintptr_t *start;
    uintptr_t *forwarding;
    int marked;
    size_t size;
} Chunk;

typedef struct {
    size_t countParts[2];
    Chunk chunksParts[2][CHUNK_LIST_CAP]; 
} ChunkList;



/*---EXTERNS---*/
extern uintptr_t heapPart1[HEAP_CAP_WORDS/2]; // semispace 1
extern uintptr_t heapPart2[HEAP_CAP_WORDS/2]; // semispace 2
extern const uintptr_t *stackBase;
extern ChunkList chuncksAllocated;
extern ChunkList chunksFreed;

/*---FUNCTIONS---*/
void *heapAlloc(size_t size_bytes); // The most important part of our implementation
void heapCollect(); //Need to be corrected?!
void chunkListInsert(ChunkList *list, void *start, size_t size);
void chunkListDump(const ChunkList *list, const char *name); 
void chunkListRemove(ChunkList *list, size_t index);
