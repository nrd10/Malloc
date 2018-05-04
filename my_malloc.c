#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "my_malloc.h"
//28 as that's how many bits each metadata block took up
#define HEADER 28 //16 for size, prev, next, free: each 4 bytes (32 bits ints/pointers)
 
//print out Linked List

unsigned long printList(struct mem_block *head) {
  struct mem_block *temp = (head);
  while (temp != NULL) {
    //   printf("Block %d data starts at address:%p\n", count, temp->header_end);
    // printf("This block has size:%d\n", temp->size);
    //printf("This block's free bit is:%d\n",temp->free);
    temp = temp->next;
    free_total++;;
  }
}


//Call this on previous node and next node
void single_merge(struct mem_block * mynode) {
  struct mem_block *temp = mynode;
  int length = temp->size + HEADER;
  int difference = temp->next->header_end - temp->header_end;
  if (length == difference) {
    temp->size += HEADER + temp->next->size;
     temp->next = temp->next->next;
     if (temp->next!= NULL) {
       temp->next->prev = temp;
     }
  }
}


//occurs when we free data and we want to merge free blocks to stop fragmentation
void  merge_blocks(struct mem_block* head) {
 struct mem_block *temp = (head);
 //Traverse freelist
 while (temp != NULL) {
   int length = temp->size + HEADER;
   // printf("Length of block is:%d\n", length);
   int difference = temp->next->header_end - temp->header_end;
   // printf("Difference between curr and next is:%d\n", difference);
   if (length == difference) {
     //printf("MERGE!!\n");
     temp->size += HEADER + temp->next->size;
     temp->next = temp->next->next;
     if (temp->next!= NULL) {
       temp->next->prev = temp;
     }
     continue;
   }
   temp = temp->next;
 }
}

//determines distance between blocks in heap address space
void blockdifference(struct mem_block * one, struct mem_block * two) {
  printf("Block A starts at:%p\n", one->header_end);
  printf("Block B starts at:%p\n", two->header_end);
  printf("Block A and B are apart by this many bytes:%d\n", (two->header_end - one->header_end));
}

//sorted insert into Free List (Doubly Linked List)
void sortedInsert(struct mem_block **head, struct mem_block **new_node) {
  struct mem_block *current = NULL;
  //case for  No head
  if ((*head) == NULL){
    (*new_node)->next = NULL;
    (*new_node)->prev = NULL;
    (*head) = (*new_node);
  }
  //case for adding to front
  else if ((*head)->header_end >= (*new_node)->header_end) {
    (*head)->prev = (*new_node);
    (*new_node)->next = (*head);
    (*new_node)->prev = NULL;
    (*head) = (*new_node);
  }
  else {
    (current) = (*head);
    while ((current)->next != NULL && ((current)->next->header_end < (*new_node)->header_end)) {
      current = current->next;
    }
    //case for adding to end
    if (current->next == NULL) {
      (*new_node)->next = current->next;
      (*new_node)->prev = current;
      current->next = (*new_node);
    }
    //case for adding to middle
    else {
      (*new_node)->next = current->next;
      (*new_node)->prev = current;
      current->next->prev = (*new_node);
      current->next = (*new_node);
    }
  }
}

//removes Node from Free List
void RemoveNode(struct mem_block ** head, struct mem_block ** removal) { 
  if (head == NULL) {
    return;
  }
  //Node to remove is the head
  if ((*head) == (*removal)) {
    ((*head) = (*removal)->next);
  }
  //Change next if removal is NOT last node
  if ((*removal)->next != NULL) {
    (*removal)->next->prev = (*removal)->prev;
  }
  //Change prev if removal is NOT 1st node
  if ((*removal)->prev != NULL) {
    (*removal)->prev->next = (*removal)->next;
  }
  //make Node that will now be malloced, have NULL next and prev pointers
  (*removal)->free = 0; //not free anymore!
  (*removal)->next = NULL;
  (*removal)->prev = NULL;
}

//iterates through linked list of FREE memory blocks until we find first one LARGE ENOUGH
struct mem_block * FirstFitFind(struct mem_block ** free_head, size_t mysize) {
  //set current pointer to head of memory
  struct mem_block * current = (*free_head);
  //tracks the last visited memory block we get to
  //stops us from having to traverse the entire linkedlist when adding a new block
  //last visited becomes last memory node in the case where we don't find big enough block
  while (current != NULL) {
    //iterate through list until 1st free node that's big enough
    //to store requested allocation
    if ((current)->size >= mysize) {
      //printf("Found block big enough!\n");
      //  printf("Block size is:%zu\n",(current)->size);
      return (current);
    }
    (current) = (current)->next;
  }
  // printf("No big enough block found!\n");
  return NULL;
}



struct mem_block * BestFitFind(struct mem_block **head, size_t size){
  int min = 100000;
  int index = 0;
  int final_index = -1;
  struct mem_block * curr = (*head);
  while (curr != NULL) {
    if (((curr->size - size) <= min) && ((curr->size - size) >= 0)) {
      min = curr->size - size;
      final_index = index;
    }
    index++;
    curr = curr->next;
  }
  //  curr = (*head);
  struct mem_block * new_curr = (*head);
  //no block big enough
  if (final_index == -1) {
    //printf("No block big enough for best fit!\n");
    return NULL;
  }
  else {
    for (int i =0; i < final_index; i++) {
      new_curr = new_curr->next;
    }
    return new_curr;
  }
}

//size is length of the original allocation block e.g. 1000 byte block split into
//size of 800 (requested) and remainder of 200 bytes
void partition(struct mem_block  **head_node, struct mem_block **oversized, size_t size) {
  struct mem_block *remainder = NULL;

  //points at size bytes after data field
  //remainder = (struct mem_block *) (*oversized)->header_end + size;

  //points at size bytes after data field
  char * pointer = (char *) (*oversized)->header_end;
  for (int i = 0; i < size; i++) {
    pointer++;
  }

  //size of partition is original size - requested size - size of HEADER
  remainder = (struct mem_block *) pointer;

  (remainder)->size = (*oversized)->size - size - HEADER;
  
  (remainder)->prev = NULL;
  (remainder)->next = NULL;
  (remainder)->free = 1;
  //ADD remainder to Free LinkedList
  sortedInsert(head_node, &remainder);
  //set size of original block
  (*oversized)->size = size;
}

//call this function when we don't find the right fit for a block
struct mem_block * addblock(size_t my_size) {
  struct mem_block * break_point;
  //find the current break address
  //break_point = sbrk(0); 
  //increments program break by the size of HEADER and amount of mem. requested
  //  printf("I'm incrementing memory by:%d\n", HEADER + my_size);
  void * result = sbrk(HEADER + my_size);
  break_point = (struct mem_block *) result;
  // printf("Result is located at:%p\n", result);

  total = total + my_size;
    //error checking
    if (result == (void*) - 1) { 
      perror("sbrk");
      return NULL;
    }
    else {
      //set fields of header of new memory block
      break_point->size = my_size;
      break_point->next = NULL;
      break_point->prev = NULL;
      break_point->free = 0;
    
      
      return break_point;
    }
}

void printblock(struct mem_block * A) {
  printf("Block size is:%lu\n", A->size);
  printf("Block's free bit is:%d\n", A->free);
  printf("Data for this block starts at address:%p\n", A->header_end);
}


size_t align(size_t x) {
  //printf("Nearest multiple of %lu is:%d\n", x,  ((x-1)|15)+1);
  size_t y;
  //  y = (x/4)*4 + 4;
  y = (x/24)*24 + 24;
  // y = ((x-1)|15)+1;
  return y;
}

void *ff_malloc(size_t size) {
  struct mem_block * BLOCK;
  size_t mysize = align(size);
  //find a block
  BLOCK = FirstFitFind(&base, mysize); 
  //split the block if it is not NULL and it has the space for it
  if ((BLOCK) != NULL) {
    if (((BLOCK)->size - mysize) >= (HEADER + 32)) {
	partition(&base, &BLOCK, size);
      }
    //remove the node from the free list
    RemoveNode(&base, &BLOCK);
  }
  else {
    //extend the heap as there are no fitting blocks
    BLOCK = addblock(mysize);
  }
  
  return ((BLOCK->header_end));
}


void *bf_malloc(size_t size) {
  struct mem_block * BLOCK;
  size_t mysize = align(size);
  //find a block
  BLOCK = BestFitFind(&base, mysize);
  //split the block if it is not NULL and it has the space for it
  if (BLOCK != NULL) {
    if ((BLOCK->size - mysize) >= (HEADER + 32)) {
	partition(&base, &BLOCK, size);
      }
    //remove the node from the free list
    RemoveNode(&base, &BLOCK);
  }
  else {
    //extend the heap as there are no fitting blocks
    BLOCK = addblock(mysize);
  }
  //return start of the memory block!!
  return (BLOCK->header_end);
}

//NOT IN USE
struct mem_block * get_block(void * pointer) {
  char * temp;
  temp = pointer;
  return (struct mem_block *)(temp- HEADER);
}
 

void ff_free(void *ptr) {
  //get to actual HEADER of the block since malloc returns data section
  //where we store memory
  char * pointer = (char *) ptr;
  for (int i =0; i<28; i++) {
    pointer--;

  }
  struct mem_block * correct_start = (struct mem_block *) pointer;
  struct mem_block * start = get_block(ptr);
  //struct mem_block * start;
  //start = (struct mem_block *) ptr;
  start->free = 1;
  correct_start->free = 1;
  //insert the memory block back into the Free list
  sortedInsert(&base, &correct_start);
  //try to merge blocks 
  //merge_blocks(base);
  if(correct_start->prev != NULL) {
     single_merge(correct_start->prev);
   }
   if(correct_start->next != NULL) {
     single_merge(correct_start->next);
   }
  
}

void bf_free(void * ptr) {
  //get to actual HEADER of the block since malloc returns data section
  //where we store memory

  char * pointer = (char *) ptr;
  for (int i =0; i<28; i++) {
    pointer--;

  }
  struct mem_block * correct_start = (struct mem_block *) pointer;
  //  struct mem_block * start = get_block(ptr);
  //struct mem_block * start;
  //start = (struct mem_block *) ptr;
  //  start->free = 1;
  correct_start->free = 1;
  //insert the memory block back into the Free list
  sortedInsert(&base, &correct_start);
  //try to merge blocks 
  // merge_blocks(base);
  
  if(correct_start->prev != NULL) {
     single_merge(correct_start->prev);
   }
   if(correct_start->next != NULL) {
     single_merge(correct_start->next);
   }
  
}


unsigned long get_data_segment_free_space_size() {
  unsigned long answer = printList(base);
  return answer;;
}

unsigned long get_data_segment_size() {
  return total;

}

