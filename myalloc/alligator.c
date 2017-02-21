#include <stdio.h>
#include <stdlib.h>
#include "myalloc.h"

void *alloc_check(size_t size)
{

	/* this is helper function that verifies whether the
	 * allocation succeeded or failed and prints out some useful
	 * information
	 */

	void *ptr; /* temp ptr for allocation */

	if ((ptr = myalloc(size)) == NULL)
	{
		
		printf("!!! Allocation of size %u failed!\n", (unsigned)size);
		return NULL;

	} else {

		printf("Allocation of size %u succeeded @ %p!\n", (unsigned)size, ptr);
		return ptr;

	}
}

int main(int argc, char *argv[])
{

	void *ptr[100];

	/* let's print some sizes of types so we know what they are */
	printf("sizes:\n");
	printf("void *:\t\t%lu\n", sizeof(void*));
	printf("long unsigned:\t%lu\n", sizeof(long unsigned));
	printf("node_t:\t\t%lu\n", sizeof(node_t));
	printf("header_t:\t%lu\n", sizeof(header_t));

	printf("allocating 1024, 1024 and 512 byte regions\n");
	ptr[0] = alloc_check(1024);
	ptr[1] = alloc_check(1024);
	ptr[2] = alloc_check(512);

	printf("Freeing allocated buffers:\n");
	print_freelist_from(__head);

	printf("Free 1024 byte region:\n");
	myfree(ptr[0]);
	printf("Freelist after free:\n");
	print_freelist_from(__head);

	printf("Free 2nd 1024 byte region:\n");
	myfree(ptr[1]);
	printf("Freelist after free:\n");
	print_freelist_from(__head);


	printf("Free 512 byte region:\n");
	myfree(ptr[2]);
	printf("Freelist after free:\n");
	print_freelist_from(__head);

	printf("Coalescing freelist:\n");
	coalesce_freelist(__head);

	printf("Printing coalesced freelist:\n");
	print_freelist_from(__head);

	printf("Allocating 1700 and 512 bytes\n");
	ptr[0] = alloc_check(1700);
	ptr[1] = alloc_check(512);

	printf("Freeing the 512 byte allocation (1700 should have failed)\n");
	myfree(ptr[1]);

	printf("Coalescing freelist:\n");
	coalesce_freelist(__head);

	printf("Printing freelist:\n");
	print_freelist_from(__head);

	printf("Destroying heap:\n");
	destroy_heap();

	printf("Allocating 1700 and 512 bytes (both should succeed now)\n");
	ptr[0] = alloc_check(1700);
	ptr[1] = alloc_check(512);

	printf("Freeing the 1700 byte allocation\n");
	myfree(ptr[0]);

	printf("Freeing the 512 byte allocation\n");
	myfree(ptr[1]);

	printf("Coalescing freelist:\n");
	coalesce_freelist(__head);

	printf("Printing freelist:\n");
	print_freelist_from(__head);

	return 0;
}

