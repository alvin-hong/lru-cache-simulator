# lru-cache-simulator
  *Author: Alvin Hong
  *Date Completed: April 15, 2022
  *Course: ECE/CS 250, Duke University, Durham, NC
  *Term: Spring 2022
  *Professor Daniel J. Sorin
  
 # Cache Characteristics
  LRU (Least Recently Used) replacement algorithm
  Algorithm takes in input:
    file.name cacheSize associativity cacheType blockSize
      ex) data.txt 32 2 wt 4
      This indicates a cache size of 32 bytes, 2-way associative, write-through/write no allocate cache with a block size of 2
      
  Based on given parameters, the simulator reads the txt file of load/store instructions to assign hit or miss statements
  based on what is stored in the cache.
