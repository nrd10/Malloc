#define TEST_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


struct mem_block {
  size_t size;
  struct mem_block *prev;
  struct mem_block *next;
  int free;
  char header_end[1]; //marks the end of the header and the beginning of addresses available to heap
};

//Global HEAD NODE
struct mem_block * base;

//Global count of data segments
unsigned total;

void printList(struct mem_block *head);

void merge_blocks(struct mem_block* head);

void sortedInsert(struct mem_block **head, struct mem_block **new_node);

void RemoveNode(struct mem_block **head, struct mem_block ** removal);

struct mem_block * FirstFitFind(struct mem_block **free_head, size_t mysize);

struct mem_block * BestFitFind(struct mem_block **free_head, size_t mysize);

void partition(struct mem_block **head_node, struct mem_block **oversized, size_t size);

struct mem_block * addblock(size_t size);

void printblock(struct mem_block * A);

size_t align(size_t x);

void * ff_malloc(size_t size);

struct mem_block * get_block(void * pointer);

void ff_free(void * pointer);

unsigned long get_data_segment_size();

unsigned long get_data_segment_free_space_size();
