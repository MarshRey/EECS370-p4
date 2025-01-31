/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

// **Note** this is a preprocessor macro. This is not the same as a function.
// Powers of 2 have exactly one 1 and the rest 0's, and 0 isn't a power of 2.
#define is_power_of_2(val) (val && !(val & (val - 1)))


/*
 * Accesses 1 word of memory.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads and 1 for writes.
 * write_data is a word, and is only valid if write_flag is 1.
 * If write flag is 1, mem_access does: state.mem[addr] = write_data.
 * The return of mem_access is state.mem[addr].
 */
extern int mem_access(int addr, int write_flag, int write_data);

/*
 * Returns the number of times mem_access has been called.
 */
extern int get_num_mem_accesses(void);

//Use this when calling printAction. Do not modify the enumerated type below.
enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

/* You may add or remove variables from these structs */
typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int dirty;
    int lruLabel;
    int tag;
    // my added variables (if needed)
    int offset;
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
    // my added variables (if needed)
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printAction(int, int, enum actionType);
void printCache(void);

/*
 * Set up the cache with given command line parameters. This is
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet)
{
    if (blockSize <= 0 || numSets <= 0 || blocksPerSet <= 0) {
        printf("error: input parameters must be positive numbers\n");
        exit(1);
    }
    if (blocksPerSet * numSets > MAX_CACHE_SIZE) {
        printf("error: cache must be no larger than %d blocks\n", MAX_CACHE_SIZE);
        exit(1);
    }
    if (blockSize > MAX_BLOCK_SIZE) {
        printf("error: blocks must be no larger than %d words\n", MAX_BLOCK_SIZE);
        exit(1);
    }
    if (!is_power_of_2(blockSize)) {
        printf("warning: blockSize %d is not a power of 2\n", blockSize);
    }
    if (!is_power_of_2(numSets)) {
        printf("warning: numSets %d is not a power of 2\n", numSets);
    }
    printf("Simulating a cache with %d total lines; each line has %d words\n",
        numSets * blocksPerSet, blockSize);
    printf("Each set in the cache contains %d lines; there are %d sets\n",
        blocksPerSet, numSets);

    cache.blockSize = blockSize;
    cache.numSets = numSets;
    cache.blocksPerSet = blocksPerSet;

    // Set all values in the cache blocks to -1
    for (int i = 0; i < MAX_CACHE_SIZE; ++i) {
        for (int j = 0; j < blockSize; ++j) {
            cache.blocks[i].data[j] = -1;
        }
        cache.blocks[i].dirty = 0;
        cache.blocks[i].lruLabel = cache.blocksPerSet - 1 - (i % cache.blocksPerSet);
        cache.blocks[i].tag = -1;
        cache.blocks[i].offset = -1;
    }
    // void
    return;
}

int getBlockOffset(int addr) {
    int mask = 0;
    // find the block offset
    for (int i = 0; i < (int)log2(cache.blockSize); ++i) {
        mask = mask << 1;
        mask = mask | 1;
    }
    return addr & mask;
}

int getSetOffset(int addr) {
    int mask = 0;
    // find the set offset
    for (int i = 0; i < (int)log2(cache.numSets); ++i) {
        mask = mask << 1;
        mask = mask | 1;
    }
    return (addr >> (int)log2(cache.blockSize)) & mask;
}

int lruBlock(setStart) {
    int lruBlockIndex = -1;
    for (int i = 0; i < cache.blocksPerSet; ++i) {
        if (cache.blocks[setStart + i].lruLabel == cache.blocksPerSet - 1) {
            lruBlockIndex = setStart + i;
            break;
        }
    }
    return lruBlockIndex;
}

void writeBlockToCache(int addr, int tag, int blockIndex, int dirty){
    // caclulate the address of the block to evict
            int evictAddr = (cache.blocks[blockIndex].tag << ((int)log2(cache.numSets) + (int)log2(cache.blockSize))) 
            + (cache.blocks[blockIndex].offset << (int)log2(cache.blockSize));
            // printf("evictAddr = %d\n", evictAddr);
            // printf("blockIndex = %d\n", blockIndex);
            // printf("cache.blocks[blockIndex].tag: %d\n", cache.blocks[blockIndex].tag);
            // printf("cache.blocks[blockIndex].offset: %d\n", cache.blocks[blockIndex].offset);
            
            // is the block dirty?
            if (cache.blocks[blockIndex].dirty == 1) {
                // if so, write blocks to memory
                for (int i = 0; i < cache.blockSize; ++i) {
                    mem_access(evictAddr + i, 1, cache.blocks[blockIndex].data[i]);
                }
                 // reset dirty bit
                cache.blocks[blockIndex].dirty = 0;
                // maybe switch thses staments around
                printAction(evictAddr, cache.blockSize, cacheToMemory);
                
            } else {
                // if not, check if it's valid
                if (cache.blocks[blockIndex].tag != -1) {
                    // if valid, write it to nowhere
                    printAction(evictAddr, cache.blockSize, cacheToNowhere);
                }
            }

            // update the block
            int memIndex = addr - (addr % cache.blockSize);
            // printAction(memIndex, cache.blockSize, memoryToCache);
            // what we are about to do
            printAction(memIndex, cache.blockSize, memoryToCache);
            for (int i = 0; i < cache.blockSize; ++i) {
                cache.blocks[blockIndex].data[i] = mem_access(memIndex + i, 0, 0);
            }
            // update the block atributes
            cache.blocks[blockIndex].tag = tag;
            cache.blocks[blockIndex].offset = getSetOffset(addr);
            cache.blocks[blockIndex].dirty = dirty;

}

void updateLRU(int setStart, int lruBlockIndex){
    // out of bounds check
    if (lruBlockIndex < setStart || lruBlockIndex >= setStart + cache.blocksPerSet) {
        printf("error: lruBlockIndex out of bounds\n");
        return ;
    }
    // update the LRU labels
    // hold the current LRU label
    int lruLabel = cache.blocks[lruBlockIndex].lruLabel;
    // update the LRU label of the block we just updated
    cache.blocks[lruBlockIndex].lruLabel = 0;
    // increment the LRU label of all other blocks
    for (int i = 0; i < cache.blocksPerSet; ++i) {
        if ((i + setStart) != lruBlockIndex && cache.blocks[setStart + i].lruLabel < lruLabel) {
            cache.blocks[setStart + i].lruLabel++;
        }
    }
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */
int cache_access(int addr, int write_flag, int write_data){
    int setOffset = getSetOffset(addr);
    // annoying math to get the offset and tag
    int tag = addr >> ((int)log2(cache.blockSize) + (int)log2(cache.numSets));

    // find the block in the set
    int setStart = setOffset * cache.blocksPerSet;
    
    // find the block with the matching tag
    
    int blockIndex = -1;
    for (int i = 0; i < cache.blocksPerSet; ++i) {
        if (cache.blocks[setStart + i].tag == tag) {
            blockIndex = setStart + i;
            break;
        }
    }

    // is lw or sw?
    if (!write_flag){ // lw
        bool hit;
        if (blockIndex != -1) {
            hit = true; // found a matching tag
        } else {
            hit = false; // didn't find a matching tag
        }

        // if not a hit, find the LRU block
        if (!hit) {
            int lruBlockIndex = lruBlock(setStart);

            // call a function to write the block to the cache
            writeBlockToCache(addr, tag, lruBlockIndex, 0);

            updateLRU(setStart, lruBlockIndex);

            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[lruBlockIndex].data[getBlockOffset(addr)];
        } // end of not hit
        else{ // hit
            int output = cache.blocks[blockIndex].data[getBlockOffset(addr)];
            // need it before updating the LRU labels
            updateLRU(setStart, blockIndex);
            printAction(addr, 1, cacheToProcessor);
            return output;
        }
    } // end of write flag check
    else { // sw
        bool hit;
        if (blockIndex != -1) {
            hit = true; // found a matching tag
        } else {
            hit = false; // didn't find a matching tag
        }

        // if not a hit, find the LRU block
        if (!hit) {
            int lruBlockIndex = lruBlock(setStart);

            // write the block to the cache
            writeBlockToCache(addr, tag, lruBlockIndex, 1);
            // printf("test\n");
            printAction(addr, 1, processorToCache);
            // printf("test\n");
            // print out blcok index
            // printf("blockIndex: %d\n", lruBlockIndex);
            // print out getBlockOffset(addr)
            // printf("getBlockOffset(addr): %d\n", getBlockOffset(addr));
            // print out next line
            // printf("cache.blocks[blockIndex].data[getBlockOffset(addr)]: %d\n", cache.blocks[blockIndex].data[getBlockOffset(addr)]);
            // printf("write_data: %d\n", write_data);
            cache.blocks[lruBlockIndex].data[getBlockOffset(addr)] = write_data;
            // printf("test after\n");
            // now update lru labels
            updateLRU(setOffset, lruBlockIndex);
        } // end of not hit
        else{ // hit
            // update the block to be dirty
            cache.blocks[blockIndex].dirty = 1;
            cache.blocks[blockIndex].data[getBlockOffset(addr)] = write_data;
            printAction(addr, 1, processorToCache);
            // now update lru labels
            updateLRU(setOffset, blockIndex);

        }
        return -1; // always return -1 for sw
    } // end of sw
    /* The next line is a placeholder to connect the simulator to
    memory with no cache. You will remove this line and implement
    a cache which interfaces between the simulator and memory. */
    // return mem_access(addr, write_flag, write_data);
}


/*
 * print end of run statistics like in the spec. **This is not required**,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(void)
{
    return;
}

/*
 * Log the specifics of each cache action.
 *
 *DO NOT modify the content below.
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
    else {
        printf("Error: unrecognized action\n");
        exit(1);
    }

}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache(void)
{
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set) {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block) {
            printf("\t\t[ %i ]: {", block);
            for (int index = 0; index < cache.blockSize; ++index) {
                printf(" %i", cache.blocks[set * cache.blocksPerSet + block].data[index]);
            }
            printf(" }\n");
        }
    }
    printf("end cache\n");
}
