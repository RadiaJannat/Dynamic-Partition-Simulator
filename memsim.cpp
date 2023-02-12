// -------------------------------------------------------------------------------------
/// this is the only file you need to edit
/// -------------------------------------------------------------------------------------
///
/// (c) 2022, Pavol Federl, pfederl@ucalgary.ca
/// Do not distribute this file.



#include "memsim.h"
#include <cassert>
#include <iostream>
#include <iomanip>
#include <list>
#include <unordered_map> // Step 2
#include <set> // Step 3

#define SPACES 20

using namespace std;


//Reference for this assignment was completely from stephane's pseudo code from his tutorial github:
//https://github.com/stephanedorotich/t457/blob/main/7-Memsim/memsim.cpp 


//Struct given by Professor in assignent document in page 5
struct Partition {
    int tag; 
    int64_t size, addr;

    //constructor
    Partition(int tag, int64_t size, int64_t addr) {
        this->tag = tag;
        this->size = size;
        this->addr = addr;

    }

};

//Given by Professor in assignent document in page 5
typedef std::list<Partition>::iterator PartitionRef;


//Struct given by Professor in assignent document in page 5
struct scmp {
  bool operator()(const PartitionRef & c1, const PartitionRef & c2) const {
    if (c1->size == c2->size)
      return c1->addr < c2->addr;
    else return c1->size > c2->size;

  }

};


//Struct given by Professor in assignent document in page 5
struct Simulator {
    MemSimResult result;
    int64_t page_size;

    // Step 1
    // all partitions, in a linked list
    std::list<Partition> all_blocks; 

    // // Step 2
    // // quick access to all tagged partitions
    std::unordered_map<long, std::vector<PartitionRef>> tagged_blocks; 

    // // Step 3
    // // sorted partitions by size/address
    std::set<PartitionRef, scmp> free_blocks; 

    
    

     
    //constructor
    Simulator(int64_t page_size) {
        this->page_size = page_size;
        result.n_pages_requested = 0;
        result.max_free_partition_size = 0;
        result.max_free_partition_address = 0;

    }





    //Reference for this fucntion and its implementation is from stephane's tutorial github:
    //https://github.com/stephanedorotich/t457/blob/main/7-Memsim/memsim.cpp 
    PartitionRef create_new_block(int size, PartitionRef partition) {

        // - if no suitable partition found:
        //     - get minimum number of pages from OS, but consider the
        //       case when last partition is free
        //     - add the new memory at the end of partition list
        //     - the last partition will be the best partition

        if(all_blocks.empty()) { // If program has just started, all_blocks is empty so we have to fill it up

            int pages_req = 1 + ((size - 1) / page_size);

            result.n_pages_requested += pages_req; //Adding it to the total pages_requested

            all_blocks.emplace_back(-1, pages_req * page_size, 0); // adding it to the all_blocks that will be tagged free 
            // and will be of size pages_req * page_size

            free_blocks.insert(all_blocks.begin()); //insert this in free_blocks 
            
        }
        else{

            int pages_req; 

            if(all_blocks.back().tag != -1) {  

                pages_req = 1 + ((size - 1) / page_size);
                
                //adding this block to the all_blocks that will be tagged free
                int64_t var = all_blocks.back().addr + all_blocks.back().size;
                all_blocks.emplace_back(-1, pages_req * page_size, var); 

                free_blocks.insert(std::prev(all_blocks.end())); //insert this in free_blocks 

                partition = all_blocks.end(); // partition is set to be the end value of all_blocks 
                partition = std::prev(partition); //partition is now set to be the previous value of the current
            }
            else{
                partition = all_blocks.end(); // partition is set to be the end value of all_blocks 
                partition = std::prev(partition); //partition is now set to be the previous value of the current

                free_blocks.erase(partition); //erase the current partition

                //calculation for pages_req and all_blocks.back() size incremented accordingly
                int64_t calc = size - all_blocks.back().size - 1;
                pages_req = 1 + ( calc / page_size); 
                all_blocks.back().size += (pages_req * page_size); 

                free_blocks.insert(partition); // free block inserted in free_blocks
            }
            result.n_pages_requested += pages_req; //total pages_requested is updated
        }
        return std::prev(all_blocks.end()); // correct
    }




    void allocate(int tag, int size) { //Function for allocation
        
        PartitionRef p; 

        if(free_blocks.empty()){ // if free_blocks is empty
            p = all_blocks.end(); // p is set to the last value of all_blocks
        }
        else{
            p = *(free_blocks.begin()); 
        } 
        

        if(p->size < size || p == all_blocks.end()) { // if the partition size is lesser than the size needed or if p is end value of all_blocks
            p = create_new_block(size, p); //create a new block
            
        }

        
        if((size < p->size) ==  true) { //if partition size is greater than size needed
            
            all_blocks.insert(p, Partition(tag, size, p->addr)); //insert it into all_blocks
            
            free_blocks.erase(p); // erase this partition from free blocks

            //updating the address and size
            p->addr += size;
            p->size -= size;
            
            //merge the neighbouring free blocks
            free_and_merge(p);

            tagged_blocks[tag].push_back(std::prev(p)); // push previous value of partition to tagged_blocks

            free_blocks.insert(p); //insert partition to free_blocks
        }
        else if((size == p->size) == true) { //if partition size and size is same

            free_blocks.erase(p); //erase this from free_block list
            
            p->tag = tag; //update tag
            
            tagged_blocks[tag].push_back(p); //push_back p in tagged_blocks
        }
    }





    void deallocate(int tag) { //Function for deallocation
        for(PartitionRef it : tagged_blocks[tag]) {
            
            while(it->tag == tag) { //while the iterator's tag is equal to tag
                free_and_merge(it); // merge neighbouring free blocks
            
            }

        }
        tagged_blocks[tag].clear(); //clear the tagged_blocks for the tag
    }





    //Reference for this fucntion and its implementation is from stephane's tutorial github:
    //https://github.com/stephanedorotich/t457/blob/main/7-Memsim/memsim.cpp
    void free_and_merge(PartitionRef p) {

        //         - mark the partition free (use tag of -1)
        // one line
        p->tag = -1; 

        //         - merge any adjacent free partitions

        // if right neighbour (r.n.) is free
        if(std::next(p) != all_blocks.end()) { // if next is not the end, then we can merger with the right neighbor
            if(std::next(p)->tag == -1) { //check if it is free
                p->size += std::next(p)->size; //update size
                free_blocks.erase(std::next(p)); //erase from free_blocks
                all_blocks.erase(std::next(p));  //erase from all_blocks
            }
        }

        // if left neighbour (l.n.) is free
        if(p != all_blocks.begin()) { // if next is not the end, then we can merger with the right neighbor
            if(std::prev(p)->tag == -1) { //check if it is free
                p->size += std::prev(p)->size; //update size
                p->addr = std::prev(p)->addr; //update address
                free_blocks.erase(std::prev(p)); //erase from free_blocks
                all_blocks.erase(std::prev(p)); //erase from all_blocks
            }
        }
        
        
        free_blocks.insert(p); //inser this into the list of free_blocks
    }




    MemSimResult getStats() {
        if(all_blocks.empty() == false) {  //if all blocks is not empty
        //iterate through all_blocks
            for(PartitionRef it = all_blocks.begin(); it != all_blocks.end(); it=std::next(it)) {  
                if(it->tag == -1) { // if free
                    if(result.max_free_partition_size < it->size) {
                        // doing this till we get max partition size and address
                        result.max_free_partition_size = it->size;
                        result.max_free_partition_address = it->addr;

                    }

                }

            }

        }
        
        return result;
    }





};

// re-implement the following function
// ===================================
// parameters:
//    page_size: integer in range [1..1,000,000]
//    requests: array of requests
// return:
//    some statistics at the end of simulation

MemSimResult mem_sim(int64_t page_size, const std::vector<Request> & requests)
{
    // if you decide to use the simulator class above, you probably do not need
    // to modify below at all

    Simulator sim(page_size);
    for (const auto & req : requests) {
        if (req.tag < 0) {
            sim.deallocate(-req.tag);
        } else {
            sim.allocate(req.tag, req.size);
        }
        //sim.check_consistency();
    }
    return sim.getStats();
}