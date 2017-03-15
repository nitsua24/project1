#include <stdio.h>
#include <stdlib.h>
#include "myalloc.h"

/* change me to 1 for more debugging information
 * change me to 0 for time testing and to clear your mind
 */
#define DEBUG 0

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

	if (DEBUG) printf("In coalesce freelist...\n");
	node_t *target = listhead;
	node_t *node = target->next;
	node_t *prev = target;

	/* traverse the free list, coalescing neighboring regions!
	 * some hints:
	 * --> it might be easier if you sort the free list first!
	 * --> it might require multiple passes over the free list!
	 * --> it might be easier if you call some helper functions from here
	 * --> see print_free_list_from for basic code for traversing a
	 *     linked list!
	 */

	if (DEBUG) printf("UNsorted head is @ %p with size %lu and points to %p\n", target, target->size, target->next);
	target = sort(target);
	if (DEBUG) printf("sorted head is @ %p with size %lu and points to %p\n", target, target->size, target->next);

	node = target->next;
	prev = target;

	while (node != NULL) {	//loop through the list
		int check = (void*)target + target->size * 16;
		if (DEBUG) printf("target + size * 16 = %d and node = %d\n", check, node);
		if ((void*)target + target->size * 16 == (void*)node) {	//check for adjacency
			target->next = node->next;
			target->size += node->size + 16;
			if (DEBUG) printf("coalesced space is  @ %p with size %lu and points to %p\n", target, target->size, target->next);
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

	node_t *listitem = __head; /* cursor into our linked list */
	node_t *prev = NULL; /* if listitem is __head, then prev must be null */
	header_t *alloc; /* a pointer to a header you can use for your allocation */

	if (DEBUG) printf("Before allocation, freelist header @ %p with size %lu\n", listitem, listitem->size);
	if (DEBUG) printf("Before allocation, prev is NULL\n");

	/* traverse the free list from __head! when you encounter a region that
	 * is large enough to hold the buffer and required header, use it!
	 * If the region is larger than you need, split the buffer into two
	 * regions: first, the region that you allocate and second, a new (smaller)
	 * free region that goes on the free list in the same spot as the old free
	 * list node_t.
	 *
	 * If you traverse the whole list and can't find space, return a null
	 * pointer! :(
	 *
	 * Hints:
	 * --> see print_freelist_from to see how to traverse a linked list
	 * --> remember to keep track of the previous free region (prev) so
	 *     that, when you divide a free region, you can splice the linked
	 *     list together (you'll either use an entire free region, so you
	 *     point prev to what used to be next, or you'll create a new
	 *     (smaller) free region, which should have the same prev and the next
	 *     of the old region.
	 * --> If you divide a region, remember to update prev's next pointer!
	 */

	while (listitem != NULL) {	//loop through the list
		prev = listitem;
		listitem = listitem->next;
		if (req_size + 16 <= prev->size) {
			break;
		}
	if (prev == NULL) printf("prev is NULL\n");
	else if (DEBUG) printf("The prev is now @ %p with size %lu and points to %p\n", prev, prev->size, prev->next);
	if (listitem == NULL) printf("listitem is NULL\n");
	else if (DEBUG) printf("The listitem is now @ %p with size %lu and points to %p\n", listitem, listitem->size, listitem->next);
	}
	//After loop, prev == freelist head, listitem == NULL

	//Now: Update the header so that it is located after the memory we want to allocat and size is reduced apropriately.
	//Also, change old node that was the head into a header for the allocated memory. Init size and magic number.

	if (DEBUG) printf("req_size is %lu and prev->size is %lu\n", req_size, prev->size);
	if (req_size + 16 <= prev->size) {
		__head = prev + req_size;
		__head->size = prev->size - req_size - 16;	//New_size = Old_size - size_of_allocated_chunk - header_for_allocated_chunk
		__head->next = prev->next;

		alloc = (header_t*)prev;
		alloc->size = req_size;
		alloc->magic = HEAPMAGIC;
		ptr = (void*)alloc + 16;

	//Helpful Debug stuff
	if (DEBUG) printf("After allocation:\n");
	if (DEBUG) printf("We wanted to reserve %lu bites\n", req_size);
	if (DEBUG) printf("There is reserved space @ %p with size %lu and magic %08lx\n", alloc, alloc->size, alloc->magic);
	if (DEBUG) printf("The freelist header is now @ %p with size %lu and points to %p\n", __head, __head->size, __head->next);

	}

	if (DEBUG) printf("Returning pointer: %p\n", ptr);
	return ptr;

}

/* myalloc returns a void pointer to size bytes or NULL if it can't.
 * myalloc will check the free regions in the free list, which is pointed to by
 * the pointer __head.
 */

void *myalloc(size_t size)
{
	if (DEBUG) printf("\nTrying to allocate some memory.\n");
	void *ptr = NULL;

	/* initialize the heap if it hasn't been */
	if (__heap == NULL) {
		if (DEBUG) printf("*** Heap is NULL: Initializing ***\n");
		init_heap();
	}

	/* perform allocation */
	/* search __head for first fit */
	if (DEBUG) printf("Going to do allocation.\n");

	ptr = first_fit(size); /* all the work really happens in first_fit */

	if (DEBUG) printf("__head is now @ %p\n", __head);

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

	if (DEBUG) printf("\nIn myfree with pointer %p\n", ptr);

	header_t *header = get_header(ptr); /* get the start of a header from a pointer */

	if (DEBUG) { print_header(header); }

	if (header->magic != HEAPMAGIC) {
		printf("Header is missing its magic number!!\n");
		printf("It should be '%08lx'\n", HEAPMAGIC);
		printf("But it is '%08lx'\n", header->magic);
		printf("The heap is corrupt!\n");
		return;
	}

	/* free the buffer pointed to by ptr!
	 * To do this, save the location of the old head (hint, it's __head).
	 * Then, change the allocation header_t to a node_t. Point __head
	 * at the new node_t and update the new head's next to point to the
	 * old head. Voila! You've just turned an allocated buffer into a
	 * free region!
	 */

	/* save the current __head of the freelist */
	/* ??? */

	/* now set the __head to point to the header_t for the buffer being freed */
	/* ??? */

	/* set the new head's next to point to the old head that you saved */
	/* ??? */

	/* PROFIT!!! */


	//header to be freed becomes head node of list
	//old head of list becomes node in the chain
	node_t *new_node = __head;

	__head = (node_t*)header;
	__head->size = header->size;
	__head->next = new_node;
}

void *sort(node_t* ptr) {
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
		if (DEBUG) printf("first node @ %p with size %lu and points to %p\n", node, node->size, node->next);
		if (DEBUG) printf("second node @ %p with size %lu and points to %p\n", target, target->size, target->next);
		if (first == 0) {
			theHead = node;
			if (DEBUG) printf("return node @ %p with size %lu and points to %p\n", ptr, ptr->size, ptr->next);
		}
		node = node->next;
		first++;
	}
	return theHead;
}
