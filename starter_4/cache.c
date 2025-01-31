/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <stdio.h>
#include <stdlib.h>
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


int getLruLabel(int index);
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
    int setOrigin; // Which set the block is in

    int enabled; // 1 if the block is enabled, 0 if it is not
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
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
    for (int i = 0; i < MAX_CACHE_SIZE; ++i) {
        cache.blocks[i].setOrigin = -1;
        cache.blocks[i].dirty = 0;
        cache.blocks[i].enabled = 0;
        cache.blocks[i].lruLabel = getLruLabel(i); // TODO: check this
        printf("block %d lruLabel = %d\n", i, cache.blocks[i].lruLabel);
        cache.blocks[i].tag = -1; // When tag is -1, assume invalid (unused)
        for (int j = 0; j < MAX_BLOCK_SIZE; ++j) {
            cache.blocks[i].data[j] = 0; // Clear all data
        }
    }

    return;
}


int getLruLabel(int index) {
    return cache.blocksPerSet - 1 - (index % cache.blocksPerSet);
}

// TODO: Double check this
int getSetOffsetFromAddr(int addr) {
    int bitMask = 0b0;
    for (int i = 0; i < (int)(log2(cache.numSets)); i++){
        bitMask = bitMask << 1;
        bitMask = bitMask | 0b1;
    }
    return (addr >> (int)(log2(cache.blockSize))) & bitMask;
}
int getBlockOffsetFromAddr(int addr) {
    int bitMask = 0b0;
    for (int i = 0; i < (int)(log2(cache.blockSize)); i++){
        bitMask = bitMask << 1;
        bitMask = bitMask | 0b1;
    }
    return addr & bitMask;
}

int getBlockFromTag(int tag, int setStartIndex) {
    for (int i = setStartIndex; i < setStartIndex + cache.blocksPerSet; i++) {
        if (cache.blocks[i].tag == tag) {
            return i;
        }
    }
    return -1; // Not found
}

int getLeastRecentlyUsedBlock(int setStartIndex) {
    // int lruBlockIndex = setStartIndex;
    for (int i = setStartIndex; i < setStartIndex + cache.blocksPerSet; i++) {
        if (cache.blocks[i].lruLabel == cache.blocksPerSet - 1) { // If the lrulabel is the last possible value
            return i;
        }
    }
    // return lruBlockIndex;
    return -1;
}

void writeNewBlockToCache(int addr, int tag, int dirty, int blockIndex) {
    int memAddress = (cache.blocks[blockIndex].tag << ((int)log2(cache.numSets) + (int)log2(cache.blockSize)))
        + (cache.blocks[blockIndex].setOrigin << (int)log2(cache.blockSize));
    // printf("memAddress = %d\n", memAddress);
    // printf("setOrigin = %d\n", cache.blocks[blockIndex].setOrigin);
    // Write old block to memory if dirty
    if (cache.blocks[blockIndex].enabled == 1 && cache.blocks[blockIndex].dirty == 1) { // Write to memory
        for (int j = 0; j < cache.blockSize; j++) {
            mem_access(memAddress + j, 1, cache.blocks[blockIndex].data[j]);
        }
        cache.blocks[blockIndex].dirty = 0; // Reset dirty bit
        // cache.blocks[blockIndex].tag = -1; // Invalidate the block but this probably isn't necessary
        cache.blocks[blockIndex].enabled = 0; // Disable the block
        printAction(memAddress, cache.blockSize, cacheToMemory);
    }
    else{
        if (cache.blocks[blockIndex].enabled == 1 && cache.blocks[blockIndex].tag != -1) {
            // cache.blocks[blockIndex].tag = -1; // Invalidate the block but this probably isn't necessary
            cache.blocks[blockIndex].enabled = 0; // Disable the block
            printAction(memAddress, cache.blockSize, cacheToNowhere);
        }
    }

    int memoryBlockStartPosition = addr - (addr % cache.blockSize);
    printAction(memoryBlockStartPosition, cache.blockSize, memoryToCache);
    for (int j = 0; j < cache.blockSize; j++){
        cache.blocks[blockIndex].data[j] = mem_access(memoryBlockStartPosition + j, 0, 0);
    }
    cache.blocks[blockIndex].tag = tag;
    cache.blocks[blockIndex].dirty = dirty;
    cache.blocks[blockIndex].enabled = 1;
    cache.blocks[blockIndex].setOrigin = getSetOffsetFromAddr(addr);

    // cache.blocks[blockIndex].set = setOffset;

}
void updateBlockLruLabel(int setStartIndex, int blockIndex) {
    if (blockIndex < 0 || blockIndex >= cache.blocksPerSet) {
        // printf("error: blockIndex is out of bounds\n");
        return;
    }
    
    printf("start index = %d\n", setStartIndex);
    printf("block index = %d\n", blockIndex);
    printf("looping through blocks %d to %d\n", setStartIndex, setStartIndex + cache.blocksPerSet);
    int lastLruLabel = cache.blocks[blockIndex].lruLabel;
    cache.blocks[blockIndex].lruLabel = 0; // Just used



    for (int i = setStartIndex; i < setStartIndex + cache.blocksPerSet; i++) {
        if (i != blockIndex && cache.blocks[i].lruLabel < lastLruLabel) {
            cache.blocks[i].lruLabel++;
        }
        printf("block %d lruLabel = %d\n", i, cache.blocks[i].lruLabel);
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
int cache_access(int addr, int write_flag, int write_data)
{   
    // printf("$$$ accessing cache at address %d, write_flag = %d, write_data = %d\n", addr, write_flag, write_data);
    // Display cache state
    int offset = getSetOffsetFromAddr(addr);
    int tag = addr >> ((int)log2(cache.numSets) + (int)log2(cache.blockSize));

    int setStartIndex = offset * cache.blocksPerSet;
    int blockIndex = getBlockFromTag(tag, setStartIndex);

    if (write_flag == 0){ 
        // Steps:
        // 1. Access cache and check to see if the block is in the cache
        // 2. If it is, return the data, then update the lruLabel for the block
        // 3. If it is not, grab from memory, then write to cache replacing the lowest lruLabel block
        if (blockIndex != -1){ // Found a block in the cache
            int toRet = cache.blocks[blockIndex].data[getBlockOffsetFromAddr(addr)];
            updateBlockLruLabel(setStartIndex, blockIndex);
            printAction(addr, 1, cacheToProcessor);
            return toRet;
        }
        else{ // Cache is a miss
            // int readData = mem_access(addr, write_flag, write_data); // Read from memory
            // Find the least recently used block
            int lruBlockIndex = getLeastRecentlyUsedBlock(setStartIndex);
            // cache.blocks[lruBlockIndex].tag = tag;
            // cache.blocks[lruBlockIndex].dirty = 0;
            // cache.blocks[lruBlockIndex].data[addr % cache.blockSize] = readData;

            writeNewBlockToCache(addr, tag, 0, lruBlockIndex); // Write to cache
            updateBlockLruLabel(setStartIndex, lruBlockIndex);
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[lruBlockIndex].data[getBlockOffsetFromAddr(addr)];
        }

    }
    else{ // Store word
        // Steps
        // 1. Access cache and see if store word is in the cache
        // 2. If it is, update the data in the cache, then update the lruLabel for the block
        // 3. If it is not, grab from memory, then write to cache replacing the lowest lruLabel block
        if (blockIndex != -1){ // Found a block in the cache
            cache.blocks[blockIndex].data[getBlockOffsetFromAddr(addr)] = write_data;
            cache.blocks[blockIndex].dirty = 1;
            printAction(addr, 1, processorToCache);
            updateBlockLruLabel(setStartIndex, blockIndex);
        }
        else{ // Cache is a miss
            // int readData = mem_access(addr, write_flag, write_data); // Read from memory
            // Find the least recently used block
            int lruBlockIndex = getLeastRecentlyUsedBlock(setStartIndex);
            writeNewBlockToCache(addr, tag, 1, lruBlockIndex); // Write to cache

            printAction(addr, 1, processorToCache);
            cache.blocks[lruBlockIndex].data[getBlockOffsetFromAddr(addr)] = write_data;
            updateBlockLruLabel(setStartIndex, lruBlockIndex);
        }
        return -1;
    }
    
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
    // printf("End of run statistics:\n");
    // printf("hits: %d

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
