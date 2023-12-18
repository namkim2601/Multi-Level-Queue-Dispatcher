/* MAB management functions for MLQD dispatcher */

/* Include Files */
#include "mab.h"

/*******************************************************
 * USER FUNCTION 
 * void print_mem_info - prints current state of virtual memory
 *
 * use for testing
 ******************************************************/
void print_mem_info(MabPtr m) {
    if (m == NULL) 
        return;  // Stop the traversal if the current node is NULL.

    if (m->size)
        printf("Offset: %d, Size: %d, Allocated: %d\n", m->offset, m->size, m->allocated);

    // Recursively traverse the left and right children.
    print_mem_info(m->left_child);
    print_mem_info(m->right_child);
}

/*******************************************************
 * MabPtr memMerge(MabPtr m) - Merge buddy memory blocks.
 *
 * Returns: A pointer to the merged memory block
 ******************************************************/
MabPtr memMerge(MabPtr m) {
    if (m == NULL) 
        return NULL;  // Base case: If the node is NULL, return NULL.

    // Recursive call to merge left and right children.
    MabPtr mergedLeft = memMerge(m->left_child);
    MabPtr mergedRight = memMerge(m->right_child);

    // Check if both children are NULL (leaf nodes) and unallocated.
    if (m->left_child != NULL && m->right_child != NULL &&
        !m->left_child->allocated && !m->right_child->allocated) {
        // Merge the current block with its buddy.
        m->allocated = 0;
        m->size = 2 * m->left_child->size;
        m->left_child = NULL;
        m->right_child = NULL;
    }

    // Return the merged block 
    return m;
}

/*******************************************************
 * MabPtr memSplit(MabPtr m, int size) - Split a memory block
 *
 * Parameters:
 *   m - The memory block to be split.
 *   size - The size of memory to be allocated.
 *
 * Returns:
 *   A pointer to the newly split memory block or NULL if it cannot be split.
 ******************************************************/
MabPtr memSplit(MabPtr m, int size) {
    if (m == NULL || m->allocated || m->size < size) 
        return NULL; // This block is not suitable for splitting.

    // Recursevily split blocks until allocation can be facilitiated
    if (m->size >= 2 * size && 
    (m->left_child == NULL && m->right_child == NULL)) 
    {
        int halfSize = m->size / 2;

        // Create left and right child blocks.
        m->left_child = (MabPtr)malloc(sizeof(Mab));
        m->right_child = (MabPtr)malloc(sizeof(Mab));

        if (m->left_child == NULL || m->right_child == NULL) 
        {
            // Memory allocation for left or right child failed.
            if (m->left_child) 
            {
                free(m->left_child);
                m->left_child = NULL;
            }
            if (m->right_child) 
            {
                free(m->right_child);
                m->right_child = NULL;
            }
            fprintf(stderr, "FATAL: malloc() not working");
            exit(EXIT_FAILURE);
        }

        m->left_child->offset = m->offset;
        m->left_child->size = halfSize;
        m->left_child->allocated = 0;
        m->left_child->parent = m;
        m->left_child->left_child = NULL;
        m->left_child->right_child = NULL;

        m->right_child->offset = m->offset + halfSize;
        m->right_child->size = halfSize;
        m->right_child->allocated = 0;
        m->right_child->parent = m;
        m->right_child->left_child = NULL;
        m->right_child->right_child = NULL;

        m->size = 0;
        m->allocated = 2; // 2 means this block has children who are allocated

        // Continue the allocation attempt in the left child
        MabPtr block_to_allocate = memSplit(m->left_child, size);
        if (block_to_allocate) 
            return block_to_allocate;

        // If the left child failed, try the right child.
        return memSplit(m->right_child, size);
    }

    // Check if the current block can be allocated
    if ((m->size >= size && size > (m->size / 2)) 
        || (m->size == 8 && size < 8))
        return m;

    return NULL; // Unable to split this block.
}


/*******************************************************
 * MabPtr memAlloc(MabPtr m, int size) - Allocate a memory block.
 *
 * Parameters:
 *   m - The root block/node of the binary tree.
 *   size - The size of memory to be allocated.
 *
 * Returns:
 *   A pointer to the allocated memory block or NULL if no suitable block is found.
 ******************************************************/
MabPtr memAlloc(MabPtr m, int size) {
    if (m == NULL || size < 1 || size > 2048) 
        return NULL;  // No suitable block found.

    // Check if the current block can be allocated without having to split blocks
    if (!m->allocated) 
    {
        if ((m->size >= size && size > (m->size / 2)) 
            || (m->size == 8 && size < 8)) 
        {
                m->allocated = 1;
                return m;
        }
    }

    // Try left_child first, then try right_child
    if (m->left_child)
    {
        MabPtr left_result = memAlloc(m->left_child, size);
        if (left_result) {
            return left_result;
        } else {
            MabPtr right_result = memAlloc(m->right_child, size);
            if (right_result)
                return right_result;
        }
    }


    /* If no suitable unallocated block is found during traversal, 
        split until a suitable block is found */
    MabPtr allocated_block = memSplit(m, size);
    if (allocated_block) {
        allocated_block->allocated = 1;
        return allocated_block;
    }

    return NULL; // No suitable block found in the entire tree.
}

/*******************************************************
 * MabPtr memFree(MabPtr m) - Free memory block.
 *
 * returns: A pointer to the freed memory block.
 ******************************************************/
MabPtr memFree(MabPtr m) {
    if (m == NULL || !m->allocated) 
        return NULL;  // Cannot free an already unallocated block or NULL.

    // Mark the provided block as unallocated.
    m->allocated = 0;

    // Perform block merging (start from root).
    MabPtr root = m;
    while (root->parent)
        root = root->parent;
    memMerge(root);

    printf("\n");
    print_mem_info(root);

    return NULL;
}
