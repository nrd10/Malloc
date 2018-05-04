#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test.h"
#define HEADER 28 //16 for size, prev, next, free: each 4 bytes (32 bits ints/pointers)
 
//print out Linked List
void printList(struct mem_block *head) {
  struct mem_block *temp = head;
  int count = 0;
  while (temp != NULL) {
    printf("Block %d data starts at address:%p\n", count, temp);
    printf("This block has size:%d\n", (temp)->size);
    printf("This block's free bit is:%d\n", (temp)->free);
    temp = temp->next;
    count++;
  }
}

void printlocation(struct mem_block * A) {
  printf("Block size starts at:%p\n", &(A->size));
  printf("Block's free bit is:%p\n", &(A->free));
  printf("Data for this block starts at address:%p\n", A->header_end);
}


/*
struct mem_block * FindStart(struct mem_block ** node) {
   printlocation(*node);
  char * ptr = (char *) (*node);
  printf("location of ptr is:%p\n", ptr);
  for (int i = 0; i<28; i++) {
    ptr--;
  }
  printf("location of ptr now is:%p\n", ptr);
  struct mem_block * new_node = (struct mem_block *) (ptr);
  printf("new Node starts at:%p\n", new_node);
  printf("LETS FIND SIZE!\n");
  printf(" SIZE IS:%d\n", new_node->size);
  return new_node;
}
*/


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
    //  printf("No block big enough for best fit!\n");
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
  char * pointer = (char *) (*oversized)->header_end;
  for (int i = 0; i < size; i++) {
    pointer++;
  }
  
  //  remainder = (struct mem_block *) (*oversized)->header_end + size;

  remainder = (struct mem_block *) pointer;

  //size of partition is original size - requested size - size of HEADER
  (remainder)->size = (*oversized)->size - size - HEADER;
  (remainder)->prev = NULL;
  (remainder)->next = NULL;
  (remainder)->free = 1;
  //ADD remainder to Free LinkedList
  sortedInsert(head_node, &remainder);
  //set size of original block
  (*oversized)->size = size;
}



size_t align(size_t x) {
  //printf("Nearest multiple of %lu is:%d\n", x,  ((x-1)|15)+1);
  size_t y;
  y = (x/4)*4 + 4;
  //  y = ((x-1)|15)+1;
  return y;
}


//call this function when we don't find the right fit for a block
struct mem_block * addblock(size_t my_size) {
  struct mem_block * break_point;
  //find the current break address
  //break_point = sbrk(0); 
  //  printf("Original block is at:%p\n", break_point);
  //increments program break by the size of HEADER and amount of mem. requested
  //  printf("I'm incrementing memory by:%d\n", HEADER + my_size);
  size_t real_size = align(my_size);
  void * result_sbrk = sbrk(HEADER + real_size);
  struct mem_block * result;
  result = (struct mem_block *) result_sbrk;
  //printf("result is located at:%p\n", result);
  int total = HEADER+real_size;
  //printf("total is:%d\n", total);
  //printf("result - total is:%p\n", result-total);
  //increment the total data segement global by the amount
  //you allocate in terms of data bytes NOT HEADERS
  total += (real_size);
    //error checking
    if (result == (void*) - 1) { 
      perror("sbrk");
      return NULL;
    }
    else {
      //set fields of header of new memory block
      result->size = real_size;
      result->next = NULL;
      result->prev = NULL;
      result->free = 0;
      return result;;
    }
}

void printblock(struct mem_block * A) {
  printf("Block size is:%lu\n", A->size);
  printf("Block's free bit is:%d\n", A->free);
  printf("Data for this block starts at address:%p\n", A->header_end);
}



void *ff_malloc(size_t size) {
  struct mem_block * BLOCK;
  size_t mysize = align(size);
  //find a block
  BLOCK = FirstFitFind(&base, mysize); 
  //split the block if it is not NULL and it has the space for it
  if ((BLOCK) != NULL) {
    if (((BLOCK)->size - mysize) >= (HEADER + 16)) {
	partition(&base, &BLOCK, mysize);
      }
    //remove the node from the free list
    RemoveNode(&base, &BLOCK);
  }
  else {
    //extend the heap as there are no fitting blocks
    BLOCK = addblock(size);
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
    if ((BLOCK->size - mysize) >= (HEADER + 16)) {
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
  // printf("Ptr is at:%p\n", ptr);
  char * pointer = (char *) ptr;
  for (int i = 0; i<28; i++) {
    pointer--;
  }  
  // printf("Pointer after decrement is at:%p\n", pointer);
   struct mem_block * start = get_block(ptr);
   struct mem_block * correct_start = (struct mem_block *) pointer;
   //printf("THe start block is at:%p\n", start);
   // printf("Correct start block is at:%p\n", correct_start);
   start->free = 1;
   correct_start->free = 1;
  //insert the memory block back into the Free list
   sortedInsert(&base, &correct_start);
  //try to merge blocks
   if(correct_start->prev != NULL) {
     single_merge(correct_start->prev);
   }
   if(correct_start->next != NULL) {
     single_merge(correct_start->next);
   }
       //   single_merge
   //merge_blocks(base);
}

void bf_free(void * ptr) {
  //get to actual HEADER of the block since malloc returns data section
  //where we store memory

  char * pointer = (char *) ptr;
  for (int i = 0; i<28; i++) {
    pointer--;
  }
  struct mem_block * correct_start = (struct mem_block *) pointer;
 
  //  struct mem_block * start = get_block(ptr);
  //struct mem_block * start;
  //start = (struct mem_block *) ptr;
  correct_start->free = 1;
  //insert the memory block back into the Free list
  sortedInsert(&base, &correct_start);
  //try to merge blocks 

 if(correct_start->prev != NULL) {
     single_merge(correct_start->prev);
   }
   if(correct_start->next != NULL) {
     single_merge(correct_start->next);
   }
  

  //merge_blocks(base);
}


unsigned long get_data_segment_free_space_size() {
  //set current = base;
  struct mem_block * current = base;
  unsigned long free_total = 0;
  while (current != NULL) {
    free_total += current->size;
    current = current->next;
  }
  return free_total;
}

unsigned long get_data_segment_size() {
  return total;

}


int main(void) { 


  //Test 1: check that addblock successfully changes the program break
  //prints out each block to see that blocks are next to one another
  //and spaced out correctly with enough room for allocated size and headers
    
  /*
  struct mem_block * X = addblock(10);
  struct mem_block * Y = addblock(30);
  struct mem_block * Z = addblock(1000);
  struct mem_block * W = addblock(20);
  X->free = 1;
  Y->free = 1;
  Z->free = 1;
  W->free = 1;
  */
  /*
  sortedInsert(&base, &X);
  sortedInsert(&base, &Y);
  sortedInsert(&base, &Z);
  sortedInsert(&base, &W);
  printList(base);
  */
  //Test 7: Check minarray
  /*
 struct mem_block * mynode = BestFitFind(&base, 15);
  //  printList(base);
  printblock(mynode);
  */
  /*
  printblock(X);
  printblock(Y);
  printblock(Z);
  */
  
  //Test 2: test SortedInsert --> determine that free list can successfully be added to
  //test that each memory block is added into correct place according
  //to address

  //Test 2A: see if Nodes are in correct order by address after in order adds
  /*
  sortedInsert(&base, &X);
  sortedInsert(&base, &Y);
  sortedInsert(&base, &Z);
  printList(base);
  */
  //Test 2B: Out of order Adds
  /*
  sortedInsert(&base, &Z);
  sortedInsert(&base, &X);
  sortedInsert(&base, &Y);
  printList(base);
  */

  //Test 2C: Other set of out of order adds
  /*
  sortedInsert(&head_node, &Y);
  sortedInsert(&head_node, &Z);
  sortedInsert(&head_node, &X);
  printList((head_node));
  */
  
  //Test 3:test removeNode: see if Nodes can successfully be removed while maintaining free list order
  /*
  sortedInsert(&base, &Y);
  sortedInsert(&base, &Z);
  sortedInsert(&base, &X);
  printList(base);
  //Test 3A: remove middle
  RemoveNode(&base, &Y); 
  printList(base);

  //Tes 3B: remove end
  RemoveNode(&base, &Z); 
  printList(base);
  */
  //Test 3C:remove head
  /*
  sortedInsert(&base, &Y);
  sortedInsert(&base, &Z);
  sortedInsert(&base, &X);
  printList(base);
  RemoveNode(&base, &X);
  printList(base);
  */

  //Test 4:Check FirstFitFind finds the correct block
  /*
  sortedInsert(&base, &Y);
  sortedInsert(&base, &Z);
  sortedInsert(&base, &X); 
  struct mem_block * allocation = FirstFitFind(&base, 100);
  allocation = FirstFitFind(&base, 500);
  allocation = FirstFitFind(&base, 250);
  allocation = FirstFitFind(&base, 2000);
  */

  //Test 5:Test partition function
  /*  
  sortedInsert(&base, &Y);
  sortedInsert(&base, &Z);
  sortedInsert(&base, &X);
  printList(base);
  partition(&base, &Z, 500);
  printList(base);
  */

  /*
  unsigned long mytotal = get_data_segment_size();
  printf("Total allocation is:%lu\n", mytotal);

  unsigned long free_total = get_data_segment_free_space_size();
  printf("total free bytes is:%lu\n", free_total);
   */

  //block difference is equal to size+HEADER 
  /*
  blockdifference(X, Y);
  printblock(X);
  printblock(Y);
  */
 
  //Test 6: test that merge works
  /*
  sortedInsert(&base, &Z);
  sortedInsert(&base, &X);
  merge_blocks(base);
  printList(base);
  sortedInsert(&base, &Y);
  printList(base);
  merge_blocks(base);
  printList(base);
  */


  //Test 8: Check malloc and free
  /*
  struct mem_block * myarray = ff_malloc(10);
  struct mem_block * secarray = ff_malloc(30);
  struct mem_block * thirdarray = ff_malloc(50);
  
  ff_free(myarray);
  // printlocation(myarray);
  ff_free(thirdarray);
  printList(base);
  ff_free(secarray);
  printList(base);
  */

  /*
  const unsigned NUM_ITEMS = 10;
  int i;
  int size;
  int sum = 0;
  int expected_sum = 0;
  int *array[NUM_ITEMS];

  size = 4;
  expected_sum += size * size;
  array[0] = (int *)bf_malloc(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[0][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[0][i];
  } //for i

  size = 16;
  expected_sum += size * size;
  array[1] = (int *)bf_malloc(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[1][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[1][i];
  } //for i

  size = 8;
  expected_sum += size * size;
  array[2] = (int *)bf_malloc(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[2][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[2][i];
  } //for i

  size = 32;
  expected_sum += size * size;
  array[3] = (int *)bf_malloc(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[3][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[3][i];
  } //for i

  bf_free(array[0]);
  bf_free(array[2]);

  size = 7;
  expected_sum += size * size;
  array[4] = (int *)bf_malloc(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[4][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[4][i];
  } //for i

  size = 256;
  expected_sum += size * size;
  array[5] = (int *)bf_malloc(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[5][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[5][i];
  } //for i

  bf_free(array[5]);
  bf_free(array[1]);
  bf_free(array[3]);

  size = 23;
  expected_sum += size * size;
  array[6] = (int *)bf_malloc(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[6][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[6][i];
  } //for i

  size = 4;
  expected_sum += size * size;
  array[7] = (int *)bf_malloc(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[7][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[7][i];
  } //for i

  ff_free(array[4]);

  size = 10;
  expected_sum += size * size;
  array[8] = (int *)bf_malloc(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[8][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[8][i];
  } //for i

  size = 32;
  expected_sum += size * size;
  array[9] = (int *)bf_malloc(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[9][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[9][i];
  } //for i

  bf_free(array[6]);
  bf_free(array[7]);
  bf_free(array[8]);
  bf_free(array[9]);

  printList(base);
  */
  int* array[10000];
  for (int i =0;i< 10000; i++) {
    array[i] = ff_malloc(128);
  }
  for (int i = 0; i <10000; i++) {
    ff_free(array[i]);
  }




  //printf("POST MALLOC BLOCK INFO!\n");
  //  struct mem_block * myblock = FindStart(&myarray);
  //printblock(myblock);

  /*
  char * ptr = (char *) myarray;
  printf("location of my original ptr is:%p\n", ptr);
  for (int i = 0; i<28; i++) {
    ptr--;
  } 
  printf("location of ptr after increments is:%p\n", ptr);
  */
 return EXIT_SUCCESS; 

}
