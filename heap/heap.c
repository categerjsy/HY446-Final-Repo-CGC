/*Standard libraries*/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/*Our libraries*/
#include "./heap.h"


/*Heap Parts*/
uintptr_t heapPart1[HEAP_CAP_WORDS/2] = {0};
uintptr_t heapPart2[HEAP_CAP_WORDS/2] = {0};

const uintptr_t *stackBase = 0;


ChunkList chuncksAllocated = {0};
ChunkList chunksFreed = {
    .countParts[0] = 1,
    .countParts[1] = 1,
    .chunksParts[0] = {
        [0] = {.start = heapPart1, .size = sizeof(heapPart1)} //-
    },
    .chunksParts[1] = {
        [0] = {.start = heapPart2, .size = sizeof(heapPart2)} //-
    },
};


int semispace=0;

void chunkListInsert(ChunkList *list, void *start, size_t size)
{
    assert(list->countParts[semispace] < CHUNK_LIST_CAP);
    
    list->chunksParts[semispace][list->countParts[semispace]].start = start;
    list->chunksParts[semispace][list->countParts[semispace]].size  = size;

    for (size_t i = list->countParts[semispace];
            i > 0 && list->chunksParts[semispace][i].start < list->chunksParts[semispace][i - 1].start;
            --i) {
        const Chunk t = list->chunksParts[semispace][i];
        list->chunksParts[semispace][i] = list->chunksParts[semispace][i - 1];
        list->chunksParts[semispace][i - 1] = t;
    }

    list->countParts[semispace] += 1;
}

// our dump debugger
void chunkListDump(const ChunkList *list, const char *name)
{
    printf("%s Chunks (%zu):\n", name, list->countParts[semispace]);
    for (size_t i = 0; i < list->countParts[semispace]; ++i) {
        printf("semispace:%d  start: %p, size: %zu\n",
               semispace,(void*) list->chunksParts[semispace][i].start,
               list->chunksParts[semispace][i].size);
    }
}


void chunkListRemove(ChunkList *list, size_t index)
{
    assert(index < list->countParts[semispace]);
    for (size_t i = index; i < list->countParts[semispace] - 1; ++i) {
        list->chunksParts[semispace][i] = list->chunksParts[semispace][i + 1];
    }
    list->countParts[semispace] -= 1;
}

void *heapAlloc(size_t size_bytes)
{
    const size_t size_words = (size_bytes + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

    if (size_words > 0) {
        

        if (chuncksAllocated.countParts[semispace] == CHUNK_LIST_CAP || chunksFreed.countParts[semispace] == 0)  {
            
            if (semispace==0) {
                semispace=1; 
                heapCollect();
                chuncksAllocated.countParts[0] = 0;
                chuncksAllocated.chunksParts[0][0].start = NULL;
                chuncksAllocated.chunksParts[0][0].size = 0;
                chunksFreed.countParts[0] = 1;
                chunksFreed.chunksParts[0][0].start = heapPart1;
                chunksFreed.chunksParts[0][0].size = sizeof(heapPart1); 
            } else {
                semispace=0;
                heapCollect();
                chuncksAllocated.countParts[1] = 0;
                chuncksAllocated.chunksParts[1][0].start = NULL;
                chuncksAllocated.chunksParts[1][0].size = 0;
                chunksFreed.countParts[1] = 1;
                chunksFreed.chunksParts[1][0].start = heapPart2;
                chunksFreed.chunksParts[1][0].size = sizeof(heapPart2);
            }

        }

        for (size_t i = 0; i < chunksFreed.countParts[semispace]; ++i) {
            const Chunk chunk = chunksFreed.chunksParts[semispace][i];
            if (chunk.size >= size_words) {

                chunkListRemove(&chunksFreed, i);
             
                const size_t tail_size_words = chunk.size - size_words;
                chunkListInsert(&chuncksAllocated, chunk.start, size_words);

                if (tail_size_words > 0) {
                    chunkListInsert(&chunksFreed, chunk.start + size_words, tail_size_words);
                }

                return chunk.start;
            }
        }
    }

    return NULL;
}

int from_semispace;
int objects = 0;
size_t help_with_end;
int offset;
uintptr_t* stack_scan;
uintptr_t* stack_next;

static void markStack(const uintptr_t *start, const uintptr_t *end)
{  

    // Parse stack memory word by word (8 bytes)
    for (; start < end; start += 1) { 

        const uintptr_t *p = (const uintptr_t *) *start;
        uintptr_t* t = (uintptr_t *) start;
        
        // Parse heap memory chunk by chunk
        for (size_t i = 0; i < chuncksAllocated.countParts[from_semispace]; ++i) {
            Chunk* chunk = &chuncksAllocated.chunksParts[from_semispace][i]; 
            
            // Check if the pointer found on stack points to our heap memory.
            if (chunk->start == p) {

                if(chunk->marked == 0){

                    chunk->marked = 1;
                    objects++;

                    // printf("Objects Found in stack: %d\n",objects);

                    // --- new --- 

                    // pass the new node to the memory
                    chunk->forwarding = heapAlloc(chunk->size*8);
                    memcpy(chunk->forwarding, (void*)chunk->start, (chunk->size*8));

                    
                    //printf("memcmp %d\n", memcmp(chunk->forwarding, chunk->start, (chunk->size*8)));                    

                    // change the root to point to the correct chunk
                    *t = (uintptr_t) chunk->forwarding;


                    
                    // point to the start of this chunk created in the new semispace
                    stack_scan = chuncksAllocated.chunksParts[semispace][chuncksAllocated.countParts[semispace]-1].start;
                    // point to the end of this chunk created in the new semispace
                    help_with_end = chuncksAllocated.chunksParts[semispace][chuncksAllocated.countParts[semispace]-1].size;
                    stack_next = stack_scan + help_with_end;
                
                    // printf("-------------------\n");
                    // printf("new start by heapAlloc %zu and stack scan is %zu\n", chunk->forwarding, stack_scan);
                    // printf("old chunk size is %d help with end is %d and stack_next is %zu\n", chunk->size, help_with_end, stack_next);
                    // printf("-------------------\n\n");

                    // parse the newly created chunk for pointers that point to the "from" semispace heap
                    for ( ; stack_scan < stack_next; stack_scan +=1) {

                        uintptr_t *p_inner = (uintptr_t *) *stack_scan;
                        uintptr_t* t_inner = (uintptr_t *) stack_scan;

                        for (size_t i = 0; i < chuncksAllocated.countParts[from_semispace]; ++i) {
                            
                            Chunk* chunk_inner = &chuncksAllocated.chunksParts[from_semispace][i]; 

                            if (chunk_inner->start == p_inner) {                                

                                if(chunk_inner->marked == 0) {

                                    chunk_inner->marked = 1;
                                    objects++;
                                    //printf("Objects Found in stack: %d\n",objects);

                                    chunk_inner->forwarding = heapAlloc(chunk_inner->size*8);
                                    memcpy(chunk_inner->forwarding, (void*)chunk_inner->start, (chunk_inner->size*8));

                                    // point to the end of this chunk created in the new semispace
                                    help_with_end = chuncksAllocated.chunksParts[semispace][chuncksAllocated.countParts[semispace]].size;
                                    stack_next = chuncksAllocated.chunksParts[semispace][chuncksAllocated.countParts[semispace]].start + help_with_end;

                                }

                                *t_inner = (uintptr_t) chunk_inner->forwarding;
                                
                            }

                        }                     
                    }
                  
                }
             
            }

        }

    }
}

void heapCollect()
{

    const uintptr_t *stack_start = (const uintptr_t*)__builtin_frame_address(0);

   // determine the "to" semispace

    if (semispace==0) {
        from_semispace=1;
    } else {
        from_semispace=0;
    }
 
    objects = 0;

    //  --- scan stack ---
    markStack(stack_start, stackBase + 1);

}
