#include <stdio.h>
#include <stdlib.h>
#include "myalloc.h"

/* change me to 1 for more debugging information
 * change me to 0 for time testing and to clear your mind
 */
#define DEBUG 0
#define MALLOC 0
#define FREE 0
#define COALESCE 1

void *__heap = NULL;
node_t *__head = NULL;

inline header_t *get_header(void *ptr)
{
	return (header_t *)(ptr - sizeof(header_t));
}

inline void print_header(header_t *header)
{
	printf("[header_t @ %p | buffer @ %p size: %lu magic: %08lx]\n", 
		   header, 
		   ((void *)header + sizeof(header_t)),
		   header->size, 
		   header->magic);
}

inline void print_node(node_t *node)
{
	printf("[node @ %p | free region @ %p size: %lu next: %p]\n", 
		   node, 
		   ((void *)node + sizeof(node_t)), 
		   node->size, 
		   node->next);
}

void print_freelist_from(node_t *node)
{
	printf("\nPrinting freelist from %p\n", node);
	while (node != NULL)
	{
		print_node(node);
		node = node->next;
	}
}

inline void coalesce_freelist(node_t *listhead)
{
	/* coalesce all neighboring free regions in the free list */

	if (COALESCE) printf("In coalesce freelist...\n");
	node_t *target = listhead;	//lock
	node_t *node = target->next;
	node_t *prev = target;

	if (COALESCE) {
		printf("Before Sorting, head is @ %p with size %lu and points to %p\n", target, target->size, target->next);
		print_freelist_from(target);
	}
	target = sort(target);	//lock
	if (COALESCE) {
		printf("After Sorting, head is @ %p with size %lu and points to %p\n", target, target->size, target->next);
		print_freelist_from(target);
	}

	node = target->next;	//need to reassign cause sort may have changed these
	prev = target;

	while (node != NULL) {	//loop through the list		//lock
		int check = (void*)target + target->size + 16;
		if (COALESCE) printf("target + size + 16 = %d and node = %d\n", check, node);
		if ((void*)target + target->size + 16 == (void*)node) {	//check for adjacency
			target->next = node->next;	//adjacent nodes are combined
			target->size += node->size + 16;
			__head = target;
		}
		prev = target;
		node = target->next;
		target = node;
		node = node->next;
	}

}

void destroy_heap()
{
	/* after calling this the heap and free list will be wiped
	 * and you can make new allocations and frees on a "blank slate"
	 */
	free(__heap);
	__heap = NULL;
	__head = NULL;
}

/* In reality, the kernel or memory allocator sets up the initial heap. But in
 * our memory allocator, we have to allocate our heap manually, using malloc().
 * YOU MUST NOT ADD MALLOC CALLS TO YOUR FINAL PROGRAM!
 */
inline void init_heap()
{
	/* FOR OFFICE USE ONLY */

	if ((__heap = malloc(HEAPSIZE)) == NULL)
	{
		printf("Couldn't initialize heap!\n");
		exit(1);
	}

	__head = (node_t*)__heap;
	__head->size = HEAPSIZE - sizeof(header_t);
	__head->next = NULL;

	if (DEBUG) printf("heap: %p\n", __heap);
	if (DEBUG) print_node(__head);

}

void *first_fit(size_t req_size)
{
	void *ptr = NULL; /* pointer to the match that we'll return */

	node_t *listitem = __head; /* cursor into our linked list */	//lock
	node_t *prev = NULL; /* if listitem is __head, then prev must be null */
	header_t *alloc; /* a pointer to a header you can use for your allocation */

	if (MALLOC) {
		printf("Before allocation:\n");
		print_freelist_from(__head);
	}

	while (listitem->next != NULL) {	//loop through the free list	//lock
		if (req_size <= listitem->size) {	//stop if an appropriate sized node is found
			break;
		}
		prev = listitem;
		listitem = listitem->next;
	}

	int leftovers = listitem->size - req_size;	//Will there be any remaining bites after allocation?
	if (MALLOC) printf("We want to reserve %lu bites, selected node has %lu bites, and there are %d bites leftover.\n", req_size, listitem->size, leftovers);
	if ((leftovers < 16) && (leftovers > 0)) {	//there is leftover memory too small for a new free list node
		__head = listitem->next;	//next node in free list becomes the head

		alloc = (header_t*)listitem;		//current node is transformed into allocated memory head
		alloc->size = req_size + leftovers;	//add leftovers to size, will be slightly bigger but wont lose nodes
		alloc->magic = HEAPMAGIC;
		ptr = (void*)alloc + 16;

		//Helpful Debug stuff
		if (MALLOC) {
			printf("After allocation:\n");
			printf("There is reserved space @ %p with size %lu and magic %08lx\n", alloc, alloc->size, alloc->magic);
			print_freelist_from(__head);
		}
	}
	else if (req_size + 16 <= listitem->size) {		//normal allocation
		__head = (void *)listitem + req_size + 16;	//lock
		__head->size = listitem->size - req_size - 16;	//Head needs to move from old free list node past allocated header and past actual allocated space
		__head->next = listitem->next;	//hook new node into list

		alloc = (header_t*)listitem;	//current node is transformed into allocated memory head
		alloc->size = req_size;
		alloc->magic = HEAPMAGIC;
		ptr = (void*)alloc + 16;

		//Helpful Debug stuff
		if (MALLOC) {
			printf("After allocation:\n");
			printf("There is reserved space @ %p with size %lu and magic %08lx\n", alloc, alloc->size, alloc->magic);
			print_freelist_from(__head);
		}
	}
	else {
		//Incorrect size, do nothing so that NULL is returned
	}

	if (MALLOC) printf("Returning pointer: %p\n", ptr);
	return ptr;

}

/* myalloc returns a void pointer to size bytes or NULL if it can't.
 * myalloc will check the free regions in the free list, which is pointed to by
 * the pointer __head.
 */

void *myalloc(size_t size)
{
	if (MALLOC) printf("\nTrying to allocate some memory.\n");	//originally DEBUG
	void *ptr = NULL;

	/* initialize the heap if it hasn't been */
	if (__heap == NULL) {
		if (MALLOC) printf("*** Heap is NULL: Initializing ***\n");	//originally DEBUG
		init_heap();
	}

	/* perform allocation */
	/* search __head for first fit */

	ptr = first_fit(size); /* all the work really happens in first_fit */

	if (MALLOC) printf("__head is now @ %p\n", __head);	//originally DEBUG

	return ptr;

}

/* myfree takes in a pointer _that was allocated by myfree_ and deallocates it,
 * returning it to the free list (__head) like free(), myfree() returns
 * nothing.  If a user tries to myfree() a buffer that was already freed, was
 * allocated by malloc(), or basically any other use, the behavior is
 * undefined.
 */
void myfree(void *ptr)
{

	if (FREE) printf("\nIn myfree with pointer %p\n", ptr);

	header_t *header = get_header(ptr); /* get the start of a header from a pointer */

	if (FREE) { print_header(header); }

	if (header->magic != HEAPMAGIC) {
		printf("Header is missing its magic number!!\n");
		printf("It should be '%08lx'\n", HEAPMAGIC);
		printf("But it is '%08lx'\n", header->magic);
		printf("The heap is corrupt!\n");
		return;
	}

	node_t *new_node = __head;	//old head of list becomes new node in chain	//lock

	__head = (node_t*)header;	//header of allocated memory becomes head of list	//lock
	__head->size = header->size;
	__head->next = new_node;	//hook into free list
}

/*
 * A helper function to sort the free list.
 * @ param ptr is the head of the current free list
 * After running the list is reordered from largest to smallest
 */
void *sort(node_t* ptr) {	//lock
	//sort the list starting with head which is target
	node_t *target = ptr;
	node_t *node = target->next;
	node_t *prev = target;
	node_t *theHead = ptr;
	int first = 0;
	while (node->next != NULL) {
		if ((void*)target > (void*)node) {
			prev->next = target->next;
			target->next = node->next;
			node->next = target;
		}
		if (first == 0) {
			theHead = node;
		}
		node = node->next;
		first++;
	}
	return theHead;
}
