#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "./heap.h"
#define N 11

typedef struct Node Node;

struct Node {
    char x;
    Node *left;
    Node *right;
};

Node *generate_tree(size_t CurLevel, size_t maxLevel)
{
    if (CurLevel < maxLevel) {
        Node *root = heapAlloc(sizeof(*root));
        //printf("root %zu\n", root);
        assert((char) CurLevel - 'a' <= 'z');
        root->x = CurLevel + 'a';
        root->left = generate_tree(CurLevel + 1, maxLevel);
        root->right = generate_tree(CurLevel + 1, maxLevel);
        return root;
    } else {
        return NULL;
    }
}




int main()
{

    stackBase = (const uintptr_t*)__builtin_frame_address(0);

    Node *root1 = generate_tree(0, 4);   //15
    root1 = NULL;

    printf("Root 2 before gc \n");
    Node *root2 = generate_tree(0, 4);   //14 + 1    

    //Node *root3 = generate_tree(0, 4);   //15
    // List Capacity 44 

    chunkListDump(&chuncksAllocated, "Alloced");
    chunkListDump(&chunksFreed, "Freed");

    return 0;
}