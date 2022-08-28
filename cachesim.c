#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

FILE *fr;

//Frame Data Structure
//Stores tag, valid bit, least Recent used counter, block address, and block data
struct frame{
    int tag;
    int valid;
    int lruCounter;
    int startAddress;
    char blockData[64];
};

//helper functions
void addStore (char *hex, int dataSize, int index);
void resetLRU (int index, int columns, struct frame **twoDarray );

//main memory array
unsigned char main_memory[16384];


int main (int argc, char* argv[])
{
    fr = fopen (argv[1], "r");          /* open the file for reading */

    int cacheSize = atoi(argv[2])*pow(2.0,10); //cache size converted to bytes
    int associativity = atoi(argv[3]); //# of ways

    char cachetype[3];
    strcpy(cachetype, argv[4]);     //wt/wna or wb/wa cache

    int blockSize = atoi(argv[5]);  //size of block

    int sets = cacheSize / blockSize / associativity;   //
    int columns = associativity;

    //create the 2-D array for the cache by allocating dynamic memory
    struct frame** cacheArray = (struct frame**)malloc(sizeof(struct frame*)*sets); 
    for (int i = 0; i < sets; i++) {
        struct frame* row = (struct frame*) malloc(sizeof(struct frame)*columns);
        cacheArray[i] = row;
    }

    //sets empty cache to hold 0 valid values and lruCounter of 0
    for (int i = 0; i < sets; i++){       
        for (int j = 0; j < columns; j++){
            cacheArray[i][j].valid = 0;
            cacheArray[i][j].lruCounter = 0;
        }
    }

    char line[200];       //instruction line buffer              


    while (fgets (line, 200, fr) != NULL){  //stores instruction in line

        char accessType[6];     /*variables to store names on each line*/
        unsigned int address, size_of_access;
        char storeValue[128];

        char hit_miss[5];  //for determining if it is a hit or miss

        /*places names into variables from the file*/
        sscanf(line, "%s %x %d %s", accessType, &address, &size_of_access, storeValue);    

        //Address arithmetic to parse address bits 
        int block_offset, index, tag;

        block_offset = address % blockSize;
        index = (address/blockSize) % sets;
        tag = address / (sets*blockSize);

        //Initialize helper variables
        int maxLRU = cacheArray[index][0].lruCounter;
        int maxIndex = 0;
        int hitIndex;

        strcpy(hit_miss, "miss");

        //find Least recently used block in set and determine if hit or miss
        for (int k = 0; k < columns; k++){
            if (cacheArray[index][k].lruCounter > maxLRU) {
                maxLRU = cacheArray[index][k].lruCounter;
                maxIndex = k;
            }
            if (cacheArray[index][k].valid == 1 && cacheArray[index][k].tag == tag){
                strcpy(hit_miss, "hit");
                hitIndex = k;
            }
        }

        //Condition for a hit
        if (strcmp(hit_miss,"hit") == 0){

            if (strcmp(accessType,"load") == 0){            //If it is a load hit
                printf("%s %04x %s ", accessType, address, hit_miss);   //prints values from cache data for load hit
                for (int j = block_offset; j<(block_offset+size_of_access); j++){
                    printf("%02hhx", cacheArray[index][hitIndex].blockData[j]);
                }
                printf("\n");
                resetLRU (index, columns, cacheArray);      //resets LRU data
                cacheArray[index][hitIndex].lruCounter = 0;
            }

            else if (strcmp(accessType,"store") == 0){          //If store hit
                // First modify the L1 cache with the given store data
                // Parse through hex values and put into block data
                char *tempor = storeValue;
                for (int i = block_offset; i < block_offset + size_of_access; i++){
                    sscanf(tempor, "%02hhx", &cacheArray[index][hitIndex].blockData[i]);
                    tempor+=2;
                }
                resetLRU (index, columns, cacheArray);
                cacheArray[index][hitIndex].lruCounter = 0;

                if (strcmp(cachetype,"wt") == 0){   //if it is write-through write to main memory too
                    char *temp = storeValue;
                    addStore(temp, size_of_access, address);
                }

                //print values for store hit
                printf("%s %04x %s\n", accessType, address, hit_miss);
            }
        }



        else if (strcmp(hit_miss,"miss") == 0){          //If it is a miss

            int full = 0;       //if '1' then set is not full, '0' for a full set

            if (strcmp(accessType,"load") == 0){    //if a load miss instruction

                for (int i = 0; i < columns; i++){      
                    if (cacheArray[index][i].valid == 0){       //if frame is empty add block from main memory
                        for (int j = 0; j < blockSize; j++){
                            cacheArray[index][i].blockData[j] = main_memory[address-block_offset+j];
                        }
                        //redefine data in the frame
                        full = 1;
                        cacheArray [index][i].lruCounter=0;
                        cacheArray [index][i].tag = tag;
                        cacheArray[index][i].startAddress = address-block_offset;
                        cacheArray[index][i].valid = 1;
                        break;
                    }
                    if (full == 1) break;
                }
                if (full == 0){             //if cache is full, evict the least recently used (max LRUcounter)
                    if (strcmp(cachetype,"wb") == 0){ //check if wb b/c may need to write evicted value to main memory
                        int addressOfFrame = cacheArray[index][maxIndex].startAddress;

                        for (int k = 0; k < blockSize; k++){        //copy evicted block into main memory
                            main_memory[addressOfFrame+k] = cacheArray[index][maxIndex].blockData[k];
                        }
                    }

                    for (int j = 0; j < blockSize; j++){            //evict block with new block from main memory
                        cacheArray[index][maxIndex].blockData[j] = main_memory[address-block_offset+j];
                    }
                    cacheArray[index][maxIndex].lruCounter=0;
                    cacheArray [index][maxIndex].tag = tag;
                    cacheArray[index][maxIndex].startAddress = address-block_offset;
                }
                resetLRU (index,columns, cacheArray);

                //print data for load miss
                printf("%s %04x %s ", accessType, address, hit_miss); 
                for (int j = address; j<(address+size_of_access); j++){
                    printf("%02hhx", main_memory[j]);
                }
                printf("\n");
            }

            else if (strcmp(accessType,"store") == 0){      // For store miss

                if (strcmp(cachetype, "wt") == 0){
                    // If it is 'wt' ie write-no-allocate store into main mem only
                    char *temp = storeValue;
                    addStore(temp, size_of_access, address);
                }
                

                //check if write allocate / wb to see if we need to update L1 cache only
                if (strcmp(cachetype,"wb") == 0){
                    //update L1 cache with new block from main mem
                    for (int i = 0; i < columns; i++){
                        if (cacheArray[index][i].valid == 0){
                            for (int j = 0; j < blockSize; j++){
                                cacheArray[index][i].blockData[j] = main_memory[address-block_offset+j];
                            }

                            //Now modify the L1 cache with new store data
                            char *tempor = storeValue;
                            for (int k = block_offset; k < block_offset + size_of_access; k++){
                                sscanf(tempor, "%02hhx", &cacheArray[index][i].blockData[k]);
                                tempor+=2;
                            }
                            //update frame data
                            full = 1;
                            cacheArray [index][i].lruCounter=0;
                            cacheArray [index][i].tag = tag;
                            cacheArray[index][i].startAddress = address-block_offset;
                            cacheArray[index][i].valid = 1;
                            break;
                        }
                        if (full == 1) break;  //break if already filled a empty cache
                    }
                    if (full == 0){
                        //If full set have to evict least recently used block
                        int addressOfFrame = cacheArray[index][maxIndex].startAddress;

                        for (int k = 0; k < blockSize; k++){       //update main memory then the cache with new block
                            main_memory[addressOfFrame+k] = cacheArray[index][maxIndex].blockData[k];
                            cacheArray[index][maxIndex].blockData[k] = main_memory[address-block_offset+k];
                        }

                        //Now modify the L1 cache
                        char *tempor = storeValue;
                        for (int i = block_offset; i < block_offset + size_of_access; i++){
                            sscanf(tempor, "%02hhx", &cacheArray[index][maxIndex].blockData[i]);
                            tempor+=2;
                        }

                        cacheArray[index][maxIndex].lruCounter=0;
                        cacheArray [index][maxIndex].tag = tag;
                        cacheArray[index][maxIndex].startAddress = address-block_offset;
                    }
                    
                    resetLRU (index,columns, cacheArray);
                }

                //print values for store miss
                printf("%s %04x %s\n", accessType, address, hit_miss);
            } 
        }

    }
    fclose(fr);   //close file

    return 0;
}

//Function for adding new store values to main memory
void addStore (char *hex, int dataSize, int index){
    for (int i = index; i < index + dataSize; i++){     //start from address to address + data size
        sscanf(hex, "%02hhx", &main_memory[i]);
        hex+=2;
    }
}

//Function for updating least recently used counters of a cache set
void resetLRU (int index, int columns, struct frame **twoDarray ){
    for (int i = 0; i < columns; i++){
        twoDarray[index][i].lruCounter++;
    }
}

