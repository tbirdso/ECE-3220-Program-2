/* CPSC/ECE 3220-002 - Fall 2019 - program 2 skeleton
 *
 * This program maintains a free list of memory allocation blocks
 * for dynamic allocation.
 *
 * The list is a circular, doubly-linked, integrated free list with
 * backward and forward pointers at the top of the available memory
 * in a free block (just below the top tag block). There is a header
 * node that is maintained even when the list is empty.
 *
 * Released blocks of memory that cannot be coalesced with existing
 * free blocks should be added at the head of the free list; there
 * is no need to keep the free list in sorted order by address since
 * the boundary tags are used for coalescing contiguous blocks.
 *
 * Here is the initial state of the memory area. Note that there are
 * 80 bytes beyond the size of the area that can be allocated because
 * of the four tag blocks and free block header.
 *
 *      =============  special ending tag block at start of region
 *      | tag=1     |    1 byte, this tag is always equal to one
 *      | signature |   11 bytes = "end_region"
 *      | empty     |    4 bytes = 0
 *      =============
 *      | tag       |    1 byte, 0 if free, 1 when allocated
 *      | signature |   11 bytes = "top_memblk"
 *      | size      |    4 bytes for size of free block
 *      +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
 * ptr->| back_link |    8 bytes, used when part of free list       A
 *      | fwd_link  |    8 bytes, used when part of free list       |
 *      |           |                                               |
 *      |           |                                size of free block
 *        ...                                         (multiple of 16)
 *      |           |                                               |
 *      |           |                                               V
 *      +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
 *      | tag       |    1 bytes, 0 if free, 1 when allocated
 *      | signature |   11 bytes = "end_memblk"
 *      | size      |    4 bytes for size of free block
 *      =============  special starting tag block at end of region
 *      | tag=1     |    1 byte, this tag is always equal to one
 *      | signature |   11 bytes = "top_region"
 *      | empty     |    4 bytes = 0
 *      +-----------+  free list header node
 * hdr->| back_link |    8 bytes, points to self if empty or to last node
 *      | fwd_link  |    8 bytes, points to hdr back_link if empty or to
 *      =============      first node
 *
 * To give some example addresses and block sizes, assume that the data
 * structure starts at 0x100 and has 0x300 bytes to allocate.
 * 
 *      =============  special ending tag block at start of region
 * 0x100|       1   |    1 byte, this tag is always equal to one
 * 0x101|<signature>|   11 bytes = "end_region"
 * 0x10c|       0   |    4 bytes = 0, size
 *      =============
 * 0x110|       0   |    1 byte, free = 0
 * 0x111|<signature>|   11 bytes = "top_memblk"
 * 0x11c|   0x300   |    4 bytes, size
 *      +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
 * 0x120|   0x440   |    8 bytes, used when part of free list       A
 * 0x128|   0x440   |    8 bytes, used when part of free list       |
 *      |           |                                               |
 *      |           |                                size of free block
 *        ...                                     (multiple of 16 = 0x10)
 *      |           |                                0x300 = 768 bytes
 *      |           |                                               V
 *      +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
 * 0x420|       0   |    1 bytes, free = 0
 * 0x421|<signature>|   11 bytes = "end_memblk"
 * 0x42c|   0x300   |    4 bytes, size
 *      =============  special starting tag block at end of region
 * 0x430|       1   |    1 byte, this tag is always equal to one
 * 0x431|<signature>|   11 bytes = "top_region"
 * 0x43c|       0   |    4 bytes = 0
 *      +-----------+  free list header node
 * 0x440|   0x120   |    8 bytes, points to self if empty or to last node
 * 0x448|   0x120   |    8 bytes, points to hdr back_link if empty or to
 *      =============      first node
 *
 * When a large enough free block is found, an allocation is made from
 * the higher-address end of the free block (so that only the size of
 * the free list block needs to change and not the free list pointers
 * to that block). Thus, if we allocate 0x60 bytes from the 0x300-byte
 * free block above, the data structures will now be:
 *
 *      =============  special ending tag block at start of region
 * 0x100|       1   |    1 byte, this tag is always equal to one
 * 0x101|<signature>|   11 bytes = "end_region"
 * 0x10c|       0   |    4 bytes = 0, size
 *      =============
 * 0x110|       0   |    1 byte, free = 0
 * 0x111|<signature>|   11 bytes = "top_memblk"
 * 0x11c|   0x280   |    4 bytes, size
 *      +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
 * 0x120|   0x440   |    8 bytes, used when part of free list       A
 * 0x128|   0x440   |    8 bytes, used when part of free list       |
 *      |           |                                               |
 *        ...                                         0x280 = 640 bytes
 *      |           |                                               V
 *      +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
 * 0x3a0|       0   |    1 byte, free = 0
 * 0x3a1|<signature>|   11 bytes = "end_memblk"
 * 0x3ac|   0x280   |    4 bytes, size
 *      =============
 * 0x3b0|       1   |    1 byte, allocated = 1
 * 0x3b1|<signature>|   11 bytes = "top_memblk"
 * 0x3bc|    0x60   |    4 bytes, size
 *      +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
 * 0x3c0|           |                                               A
 *        ...                                           0x60 = 96 bytes
 *      |           |                                               V
 *      +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
 * 0x420|       1   |    1 byte, allocated = 1
 * 0x421|<signature>|   11 bytes = "end_memblk"
 * 0x42c|    0x60   |    4 bytes, size
 *      =============  special starting tag block at end of region
 * 0x430|       1   |    1 byte, this tag is always equal to one
 * 0x431|<signature>|   11 bytes = "top_region"
 * 0x43c|       0   |    4 bytes = 0
 *      +-----------+  free list header node
 * 0x440|   0x120   |    8 bytes, points to self if empty or to last node
 * 0x448|   0x120   |    8 bytes, points to hdr back_link if empty or to
 *      =============      first node
 *
 * In the normal case, each allocation uses 32 bytes beyond thei
 * requested amount since additional tag blocks will be needed. For the
 * example above, 0x300 - 0x60 - 0x20 = 0x280, or, in decimal,
 *
 *  768   starting free space of 768 bytes (0x300 bytes)
 * - 96   minus request of 96 bytes (0x60 bytes)
 * - 32   minus extra tag block space of 32 bytes (0x20 bytes)
 * ----
 *  640   equals resulting free space of 640 bytes (0x280 bytes)
 *
 * When there is not at least 48 bytes left over in a free block after
 * an allocation, the whole free block is allocated. In this case, the
 * additional tags are not needed since the existing tags can be used.
 * (Of course, the whole block is then removed from the free list.)
 *
 * When there is not enough free memory available to satisfy a request,
 * a NULL pointer should be returned. A request to allocate zero bytes
 * should also return a NULL pointer.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* global data structures */

struct tag_block { char tag; char sig[11]; unsigned int size; };
struct free_block { struct free_block *back_link, *fwd_link; };
struct free_block *free_list;

/* signature check macro */

#define SIGCHK(w,x,y,z) {struct tag_block *scptr = (struct tag_block *)(w);\
if(strncmp((char *)(scptr)+1,(x),(y))!=0){printf("*** sigchk fail\n");\
printf("*** at %s, ptr is %p, sig is %s\n",(z),(w),(char *)(w)+1);}}

#define TOPSIGCHK(a,b) {SIGCHK((a),"top_",4,(b))}
#define ENDSIGCHK(a,b) {SIGCHK((a),"end_",4,(b))}

char *region_base;


/* function headers */
int free_size();

void init_region(){
  struct tag_block *ptr;
  struct free_block *links1, *links2;

  region_base = (char *) malloc( 1680 );
  if( region_base == NULL ){ printf( "no memory!\n" ); exit(0); }

  ptr = (struct tag_block *) region_base;
  ptr->tag = 1;
  strcpy( ptr->sig, "end_region" );
  ptr->size = 0;

  ptr = (struct tag_block *)(region_base + 16);
  ptr->tag = 0;
  strcpy( ptr->sig, "top_memblk" );
  ptr->size = 1600;

  ptr = (struct tag_block *)(region_base + 1632);
  ptr->tag = 0;
  strcpy( ptr->sig, "end_memblk" );
  ptr->size = 1600;

  ptr = (struct tag_block *)(region_base + 1648);
  ptr->tag = 1;
  strcpy( ptr->sig, "top_region" );
  ptr->size = 0;

  links1 = (struct free_block *)(region_base + 32);
  links2 = (struct free_block *)(region_base + 1664);
  links1->back_link = links2;
  links1->fwd_link = links2;
  links2->back_link = links1;
  links2->fwd_link = links1;

  free_list = links2;

  printf( "data structure starts at %p\n", region_base );
  printf( "free_list is located at %p\n", free_list);
}

void prt_free_block( struct free_block *fb ){
  struct tag_block *tb = (struct tag_block *)((char *)(fb) - 16);
  printf( "   free block at %p of size 0x%x\n", (char *)fb, tb->size );
  TOPSIGCHK(tb,"prt_free_block")
  ENDSIGCHK((char *)(tb)+(tb->size)+16,"prt_free_block")
}

void prt_free_list(){
  struct free_block *ptr;
  if( free_list->fwd_link == free_list ){
    printf( "   ----------free list is empty-----------\n" );
    return;
  }
  printf( "   ---------------free list---------------\n" );
  ptr = free_list->fwd_link;
  prt_free_block( ptr );
  ptr = ptr->fwd_link;
  while( ptr != free_list ){
    prt_free_block( ptr );
    ptr = ptr->fwd_link;
  }
  printf( "   --------------end of list--------------\n" );
}



/* void *alloc_mem( unsigned int amount )
 *
 * input parameter
 *   amount is the number of bytes requested
 *
 * return value
 *   alloc_mem() returns a pointer to the start of the allocated
 *   block of free memory, beyond the allocation tag block. Note
 *   that the size of the beginning and ending tag blocks is over
 *   and above the number of bytes requested in "amount". If a
 *   suitable block cannot be found, alloc_mem() returns NULL.
 *   A request for zero bytes also returns NULL.
 *
 * description
 *   alloc_mem() rounds up the "amount" of memory requested to
 *   the nearest positive multiple of 16 bytes. It then searches
 *   in a first-fit manner for a block of free memory that can
 *   satisfy the requested amount of memory. There must be at
 *   least 48 bytes remaining in the free block after the
 *   allocation (i.e., enough leftover space for two tag blocks
 *   and a 16-byte remaining free area); otherwise, the whole
 *   free area must be allocated. If there is free memory left
 *   over, it is left at the top of the free block. When a
 *   suitable block is found, alloc_mem() sets the tags, sizes,
 *   and signatures appropriately and returns a pointer to the
 *   beginning of the allocated memory (i.e., to the location
 *   immediately below the starting tag block).
 */

void *alloc_mem( unsigned int amount ){

  /* your code here */
	if(amount == 0)	return NULL;

	struct free_block *mem_ptr = NULL;
	struct free_block *ptr;
	struct tag_block *tag_ptr, *tag_ptr_f, *tag_ptr_a, *end_ptr;
	int req_amt = (amount % 16 == 0) ? amount : ((amount / 16) + 1) * 16;

	// Step through the free list
	ptr = free_list -> fwd_link;
	tag_ptr = ((struct tag_block *) (ptr)) - 1;

	while(ptr != NULL && ptr != free_list && mem_ptr == NULL) {
		// Select first available node
		if(tag_ptr->size >= req_amt) {
			mem_ptr = ptr;
		} else {
			ptr = ptr -> fwd_link;
			tag_ptr = ((struct tag_block *) ptr) - 1;
		}
	}

	// If no sufficient free block could be found, return NULL
	if(ptr == NULL || ptr == free_list) return NULL;

	// If block is larger than the request, split it
	if(tag_ptr->size >= req_amt + 48) {
		// Top tag block will be assigned to new, smaller mem block
		tag_ptr->tag = 0;
		// Bottom tag block will be assigned to allocated memblock
		end_ptr = tag_ptr + (tag_ptr->size / 16) + 1;
		end_ptr->tag = 1;
		
		// Add tag at bottom of free block
		tag_ptr->size = tag_ptr->size - req_amt - 2 * sizeof(struct tag_block);
		tag_ptr_f = tag_ptr + (tag_ptr->size / 16) + 1;
		tag_ptr_f->tag = 0;
		strcpy(tag_ptr_f->sig, tag_ptr->sig);
		tag_ptr_f->size = tag_ptr->size;

		// Create new tag block for allocated memblk
		end_ptr->size = req_amt;

		tag_ptr_a = end_ptr - (req_amt / 16) - 1;
		tag_ptr_a->tag = 1;
		// FIXME: reassign signature?
		strcpy(tag_ptr_a->sig, end_ptr->sig);
		tag_ptr_a->size = end_ptr->size;
			
		// Don't need to edit links because free block location did not change
		// Assign memory pointer to pass out
		mem_ptr = (struct free_block *) (end_ptr - (req_amt / 16));

	// If block is approximately the same size as the request, allocate it
	} else {

		tag_ptr->tag=1;
		end_ptr = tag_ptr + (tag_ptr->size / 16) + 1;
		end_ptr->size = tag_ptr->size;
		end_ptr->tag = 1;

		struct free_block *prev = ptr->back_link;
		prev->fwd_link = ptr->fwd_link;
		ptr->fwd_link->back_link = prev;
	}
 
	return mem_ptr;
}


/* Step through the free list and count block sizes
 */
int free_size() {
	// if list is full return 0
	if (free_list == NULL) return 0;
	if (free_list == free_list->fwd_link) return 0;

	// otherwise step through list to calculate size
	struct free_block *ptr = free_list -> fwd_link;
	struct tag_block *tag_ptr = ((struct tag_block *) (ptr)) - 1;
	int size = tag_ptr->size;
	ptr = ptr -> fwd_link;

	
	while(!(ptr == NULL) && !(ptr == free_list)) {
		tag_ptr = ((struct tag_block *) (ptr)) - 1;
		size += tag_ptr->size;
		ptr = ptr -> fwd_link;
	}

	return size;
}


/* void release_mem( void *ptr )
 *
 * input parameter
 *   ptr is a pointer to the start of a block of memory that
 *   had been previously allocated by alloc_mem(); note that a
 *   tag block immediately precedes this address for a valid
 *   address.
  *
 * return value
 *   release_mem() returns a code of 0 for valid pointers and
 *   a nonzero for invalid pointers. (Validity is determined
 *   by the presence of a tag block immediately preceding the
 *   "ptr" address with the tag set to allocated.)
 *
 * description
 *   If the pointer yields a valid allocated block then the
 *   block is returned into the free list, possibly with
 *   coalescing taking place. The four possible valid cases
 *   can be processed as described below, and each valid case
 *   results in a return value of zero.
 *
 *   1) Both above and below blocks are allocated - add the
 *      returned block to the free list at the head of free
 *      list (thus the size of free list increases by one
 *      node); change the tags from allocated to free.
 *
 *   2) Above block is free but below block is allocated -
 *      coalesce the returned block with the block above;
 *      change the tags and sizes appropriately (thus the free
 *      list size and all the free list node pointers remain
 *      the same; you are only updating other fields in an
 *      existing free list node); change the signatures in the
 *      previous ending tag block of the block above and the
 *      starting tag block of the returned block (so that
 *      signature checks will fail if a dangling pointer is
 *      later used)
 *
 *   3) Above block is allocated but below block is free -
 *      coalesce the returned block with the block below;
 *      change the tags and sizes appropriately and change the
 *      free list pointers in the backward and forward free
 *      nodes to point to the top of the newly-merged free
 *      block (pointers change but the size of the free list
 *      does not change); change signatures in the ending tag
 *      block of the returned block and the previous starting
 *      tag block of the block below (so that signature checks
 *      will fail if a dangling pointer is later used)
 *
 *   4) Both above and below block are free - coalesce the
 *      returned block with both the above and below blocks
 *      into a single free block and remove the node for the
 *      bottom block (thus reducing the size of the free list
 *      by one node); change the tags and sizes appropriately;
 *      change signatures in all tag blocks except in the
 *      starting tag block of the block above and in the ending
 *      tag block of the block below (so that signature checks
 *      will fail if a dangling pointer is later used)
 *
 *   Invalid pointers are detected by performing a signature
 *   check on the tag block just above the pointer. Invalid
 *   pointers result in a nonzero return code (value of 1),
 *   with no other release actions performed.
 *
 *   As mentioned in cases 2, 3, and 4, you should change
 *   signatures in any unused tag blocks to hint strings, e.g.,
 *   "old_top_mb" and "old_end_mb". This will cause signature
 *   checking to fail when you use a dangling pointer to an old
 *   tag block; and, since the signature check macro prints a
 *   failing character string, these hints can sometimes help
 *   you in debugging. You can also include an event count
 *   number in the hint string by using sprintf(), such as
 *     sprintf(hint_string,"old_top%03d",event_count);
 *   Remember that the hint string should be at most ten
 *   printable characters and a termininating null character
 *   (since the signature field is 11 bytes).
 */

unsigned int release_mem( void *ptr ){

	// Check for bad pointer
	if(ptr == NULL) return 1;

	int coalesce_lower = 0, coalesce_upper = 0;
	struct free_block *f_ptr = (struct free_block *)ptr;
	struct tag_block *tag_ptr = (struct tag_block *)ptr - 1;
	struct tag_block *end_ptr = tag_ptr + 1 + (tag_ptr->size / 16);

	if((void *)tag_ptr == (void *)free_list) return 1;
	if(tag_ptr->tag != 1 || end_ptr->tag != 1) return 1; 
	if(tag_ptr->size == 0 || end_ptr->size == 0) return 1;

	// Check upper and lower blocks
	coalesce_lower = (end_ptr + 1)->tag == 0 ? 1 : 0;
	coalesce_upper = (tag_ptr - 1)->tag == 0 ? 1 :  0;

	// Case 1: No coalesce
	if(!coalesce_lower && !coalesce_upper) {
		// Reset tag block status
		tag_ptr->tag = 0;
		end_ptr->tag = 0;
		
		// Insert into free list
		struct free_block *temp_ptr = free_list->fwd_link;
		free_list->fwd_link = f_ptr;
		f_ptr->back_link = free_list;
		f_ptr->fwd_link = temp_ptr;
		f_ptr->fwd_link->back_link = f_ptr;

	}
	// Case 2: Coalesce with upper
	else if(!coalesce_lower && coalesce_upper) {
	
		struct tag_block *upper_lower_tag = tag_ptr - 1;
		struct tag_block *top_tag = upper_lower_tag - (upper_lower_tag->size / 16) - 1;

		top_tag->tag = 0;
		end_ptr->tag = 0;

		top_tag->size += tag_ptr->size + 2 * sizeof(struct tag_block);
		end_ptr->size = top_tag->size;
	}
	// Case 3: Coalesce with lower
	else if(coalesce_lower && !coalesce_upper) {

		struct tag_block *lower_upper_tag = tag_ptr - 1;
		struct tag_block *bottom_tag = lower_upper_tag - (lower_upper_tag->size / 16) - 1;
		struct free_block *bottom_block = (struct free_block *)(bottom_tag + 1);

		bottom_tag->tag = 0;
		tag_ptr->tag = 0;

		bottom_tag->size += tag_ptr->size + 2 * sizeof(struct tag_block);
		tag_ptr->size = bottom_tag->size;

		f_ptr->fwd_link = bottom_block->fwd_link;
		f_ptr->fwd_link->back_link = f_ptr;
		f_ptr->back_link = bottom_block->back_link;
		f_ptr->back_link->fwd_link = f_ptr;
	}
	// Case 4: Coalesce with upper and lower
	else {
		struct tag_block *upper_lower_tag = tag_ptr - 1;
		struct tag_block *top_tag = upper_lower_tag - (upper_lower_tag->size / 16) - 1;
		struct free_block *top_block = (struct free_block *)(top_tag + 1);

		struct tag_block *lower_upper_tag = tag_ptr - 1;
		struct tag_block *bottom_tag = lower_upper_tag - (lower_upper_tag->size / 16) - 1;
		struct free_block *bottom_block = (struct free_block *)(bottom_tag + 1);

		top_tag->size += bottom_tag->size + 4 * sizeof(struct tag_block);
		bottom_tag->size = top_tag->size;

		top_block->fwd_link = bottom_block->fwd_link;
		top_block->fwd_link->back_link = top_block;

	}
	// Return status integer
	return 0;
}


int main(){
  void *ptr[20];
  unsigned int rc;

  printf("start memory allocation test, pointer size is %lu bytes\n",
    sizeof(void *));

  init_region();
  prt_free_list();

  printf("alloc 0x640\n");
  ptr[0] = alloc_mem(0x640); if(ptr[0]==NULL) printf("ptr[0] gets NULL\n");
  prt_free_list();
  printf("release 0x640\n");
  rc=release_mem(ptr[0]); if(rc) printf("*** release_mem() fails\n");
  prt_free_list();

  printf("alloc 6 blocks\n");
  ptr[1] = alloc_mem(0x100); if(ptr[1]==NULL) printf("ptr[1] gets NULL\n");
  ptr[2] = alloc_mem(0x100); if(ptr[2]==NULL) printf("ptr[2] gets NULL\n");
  ptr[3] = alloc_mem(0x100); if(ptr[3]==NULL) printf("ptr[3] gets NULL\n");
  ptr[4] = alloc_mem(0x100); if(ptr[4]==NULL) printf("ptr[4] gets NULL\n");
  ptr[5] = alloc_mem(0x100); if(ptr[5]==NULL) printf("ptr[5] gets NULL\n");
  ptr[6] = alloc_mem(0xa0);  if(ptr[6]==NULL) printf("ptr[6] gets NULL\n");
  prt_free_list();

  printf("try to alloc 0xa0 more\n");
  ptr[7] = alloc_mem(0xa0);
  if(ptr[7]==NULL) printf("*** alloc_mem() returns NULL\n");
  prt_free_list();
  printf("release ptr[1] - tests case 1\n"); rc=release_mem(ptr[1]);
  if(rc) printf("*** release_mem() fails\n");
  prt_free_list();
  printf("release ptr[4] - tests case 1\n"); rc=release_mem(ptr[4]);
  if(rc) printf("*** release_mem() fails\n");
  prt_free_list();
  printf("release ptr[3] - tests case 2\n"); rc=release_mem(ptr[3]);
  if(rc) printf("*** release_mem() fails\n");
  prt_free_list();
  printf("release ptr[5] - tests case 3\n"); rc=release_mem(ptr[5]);
  if(rc) printf("*** release_mem() fails\n");
  prt_free_list();
  printf("release ptr[2] - tests case 4\n"); rc=release_mem(ptr[2]);
  if(rc) printf("*** release_mem() fails\n");
  prt_free_list();
  printf("release ptr[6] - tests case 3\n"); rc=release_mem(ptr[6]);
  if(rc) printf("*** release_mem() fails\n");
  prt_free_list();
  printf("re-release ptr[2] - logical error\n"); rc=release_mem(ptr[2]);
  if(rc) printf("*** release_mem() fails\n");

  printf("alloc 12 blocks and release 5 to create 6 free blocks\n");
  ptr[1] = alloc_mem(0x60); if(ptr[1]==NULL) printf("ptr[1] gets NULL\n");
  ptr[2] = alloc_mem(0x50); if(ptr[2]==NULL) printf("ptr[2] gets NULL\n");
  ptr[3] = alloc_mem(0x50); if(ptr[3]==NULL) printf("ptr[3] gets NULL\n");
  ptr[4] = alloc_mem(0x40); if(ptr[4]==NULL) printf("ptr[4] gets NULL\n");
  ptr[5] = alloc_mem(0x40); if(ptr[5]==NULL) printf("ptr[5] gets NULL\n");
  ptr[6] = alloc_mem(0x30); if(ptr[6]==NULL) printf("ptr[6] gets NULL\n");
  ptr[7] = alloc_mem(0x30); if(ptr[7]==NULL) printf("ptr[7] gets NULL\n");
  ptr[8] = alloc_mem(0x20); if(ptr[8]==NULL) printf("ptr[8] gets NULL\n");
  ptr[9] = alloc_mem(0x20); if(ptr[9]==NULL) printf("ptr[9] gets NULL\n");
  ptr[10] = alloc_mem(0x10); if(ptr[10]==NULL) printf("ptr[10] gets NULL\n");
  ptr[11] = alloc_mem(0x10); if(ptr[11]==NULL) printf("ptr[11] gets NULL\n");
  ptr[12] = alloc_mem(0x293); if(ptr[12]==NULL) printf("ptr[12] gets NULL\n");
  rc=release_mem(ptr[2]); if(rc) printf("*** release_mem() fails\n");
  rc=release_mem(ptr[4]); if(rc) printf("*** release_mem() fails\n");
  rc=release_mem(ptr[6]); if(rc) printf("*** release_mem() fails\n");
  rc=release_mem(ptr[8]); if(rc) printf("*** release_mem() fails\n");
  rc=release_mem(ptr[10]); if(rc) printf("*** release_mem() fails\n");
  prt_free_list();
  ptr[13] = alloc_mem(0x20); if(ptr[13]==NULL) printf("ptr[13] gets NULL\n");
  prt_free_list();
  ptr[14] = alloc_mem(0x20); if(ptr[14]==NULL) printf("ptr[14] gets NULL\n");
  prt_free_list();
  ptr[15] = alloc_mem(0x20); if(ptr[15]==NULL) printf("ptr[15] gets NULL\n");
  prt_free_list();
  ptr[16] = alloc_mem(0x20); if(ptr[16]==NULL) printf("ptr[16] gets NULL\n");
  prt_free_list();
  ptr[17] = alloc_mem(0x20);
  if(ptr[17]==NULL) printf("*** alloc_mem() returns NULL\n");
  prt_free_list();
  return 0;
}


/* running this code should produce ouput such as follows
   (note: your starting address and block addresses might differ)

start memory allocation test, pointer size is 8 bytes
data structure starts at 0x215e420
free_list is located at 0x215eaa0
   ---------------free list---------------
   free block at 0x215e440 of size 0x640
   --------------end of list--------------
alloc 0x640
   ----------free list is empty-----------
release 0x640
   ---------------free list---------------
   free block at 0x215e440 of size 0x640
   --------------end of list--------------
alloc 6 blocks
   ----------free list is empty-----------
try to alloc 0xa0 more
*** alloc_mem() returns NULL
   ----------free list is empty-----------
release ptr[1] - tests case 1
   ---------------free list---------------
   free block at 0x215e980 of size 0x100
   --------------end of list--------------
release ptr[4] - tests case 1
   ---------------free list---------------
   free block at 0x215e620 of size 0x100
   free block at 0x215e980 of size 0x100
   --------------end of list--------------
release ptr[3] - tests case 2
   ---------------free list---------------
   free block at 0x215e620 of size 0x220
   free block at 0x215e980 of size 0x100
   --------------end of list--------------
release ptr[5] - tests case 3
   ---------------free list---------------
   free block at 0x215e500 of size 0x340
   free block at 0x215e980 of size 0x100
   --------------end of list--------------
release ptr[2] - tests case 4
   ---------------free list---------------
   free block at 0x215e500 of size 0x580
   --------------end of list--------------
release ptr[6] - tests case 3
   ---------------free list---------------
   free block at 0x215e440 of size 0x640
   --------------end of list--------------
re-release ptr[2] - logical error
*** release_mem() fails
alloc 12 blocks and release 5 to create 6 free blocks
   ---------------free list---------------
   free block at 0x215e730 of size 0x10
   free block at 0x215e7a0 of size 0x20
   free block at 0x215e830 of size 0x30
   free block at 0x215e8e0 of size 0x40
   free block at 0x215e9b0 of size 0x50
   --------------end of list--------------
   ---------------free list---------------
   free block at 0x215e730 of size 0x10
   free block at 0x215e830 of size 0x30
   free block at 0x215e8e0 of size 0x40
   free block at 0x215e9b0 of size 0x50
   --------------end of list--------------
   ---------------free list---------------
   free block at 0x215e730 of size 0x10
   free block at 0x215e8e0 of size 0x40
   free block at 0x215e9b0 of size 0x50
   --------------end of list--------------
   ---------------free list---------------
   free block at 0x215e730 of size 0x10
   free block at 0x215e9b0 of size 0x50
   --------------end of list--------------
   ---------------free list---------------
   free block at 0x215e730 of size 0x10
   free block at 0x215e9b0 of size 0x10
   --------------end of list--------------
*** alloc_mem() returns NULL
   ---------------free list---------------
   free block at 0x215e730 of size 0x10
   free block at 0x215e9b0 of size 0x10
   --------------end of list--------------

*/
