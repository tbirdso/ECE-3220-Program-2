/* CPSC/ECE 3220 simple memory block allocation program
 *
 * the functions work on a single array of memory blocks, each of which
 *   can either be free or allocated and each of which has a status byte
 *   and payload size byte at each end (i.e., header and trailer fields)
 *
 * block structure
 *
 *   +--------+--------+------------------------+--------+--------+
 *   | status |  size  |    area to allocate    |  size  | status |
 *   +--------+--------+------------------------+--------+--------+
 *   |<--- header ---->|<---- payload size ---->|<--- trailer --->|
 *   |<----------------------- block size ----------------------->|
 *
 *     status byte: 0 => free, 1 => allocated
 *     size byte: payload size is limited to 255
 *
 *
 * block structure annotated with pointer values
 *
 *   block pointer when considering this block for allocation
 *   |   => *(block_pointer) == status
 *   |
 *   |        block pointer + 1 => *(block_pointer + 1) == size
 *   |        |
 *   |        |        block pointer + 2 == pointer returned to user
 *   v        v        v
 *   +--------+--------+------------------------+--------+--------+
 *   | status |  size  |    area to allocate    |  size  | status |
 *   +--------+--------+------------------------+--------+--------+
 *                                              ^        ^        ^
 *                       block pointer + size + 2        |        |
 *                                                       |        |
 *                                block pointer + size + 3        |
 *                                                                |
 *                                         block pointer + size + 4
 *                                          == start of next block
 *
 * the allocate function is first fit and traverses blocks until a free
 *   block of adequate payload size is found; the top of the free block
 *   is split off for allocation if the remaining space is large enough
 *   to support a free block of MIN_PAYLOAD_SIZE in size along with new
 *   header and trailer, otherwise the complete free block is allocated
 *
 * the release function merely changes the status of an allocated block
 *   back to free; no coalescing is done in the current implementation
 * 
 * this version adds macros
 */

#include <stdio.h>

#define FREE 0
#define ALLOCATED 1

#define BYTE_COUNT 256
#define MIN_PAYLOAD_SIZE 2
#define MIN_BLOCK_SIZE 6

/* macros for header and trailer fields based */
/*   on block_ptr variable and top size field */
#define HEADER_SIZE 2
#define CONTROL_FIELDS_SIZE 4
#define USER_PTR (block_ptr+HEADER_SIZE)
#define TOP_STATUS (*(block_ptr))
#define TOP_SIZE (*(block_ptr+1))
#define PAYLOAD_SIZE (TOP_SIZE)
#define BLOCK_SIZE (TOP_SIZE+CONTROL_FIELDS_SIZE)
#define BOTTOM_SIZE (*(block_ptr+PAYLOAD_SIZE+2))
#define BOTTOM_STATUS (*(block_ptr+PAYLOAD_SIZE+3))
#define TOP_STATUS_OF_NEXT_BLOCK (*(block_ptr+PAYLOAD_SIZE+4))
#define TOP_SIZE_OF_NEXT_BLOCK (*(block_ptr+PAYLOAD_SIZE+5))


unsigned char __attribute__ ((aligned (65536))) area[BYTE_COUNT];


void simple_init(){
  /* top status    */ area[0] = FREE;
  /* top size      */ area[1] = BYTE_COUNT - 4;
  /* bottom size   */ area[BYTE_COUNT - 2] = BYTE_COUNT - 4;
  /* bottom status */ area[BYTE_COUNT - 1] = FREE;
}

void print_blocks(){
  unsigned char *block_ptr = area;

  printf( "\nblock allocation list\n" );
  while( block_ptr < ( area + BYTE_COUNT ) ){
    printf( "--block at %p\n", block_ptr );
    printf( "  top status is    %d\n", TOP_STATUS );
    printf( "  top size is      %d\n", TOP_SIZE );
    printf( "  bottom size is   %d\n", BOTTOM_SIZE );
    printf( "  bottom status is %d\n", BOTTOM_STATUS );
    block_ptr += BLOCK_SIZE;
  }
}

unsigned char *simple_allocate( unsigned int req_size ){
  unsigned char *block_ptr;
  unsigned int remaining_payload_size;

  /* immediately reject requests that are too large */
  if( req_size > BYTE_COUNT - CONTROL_FIELDS_SIZE ) return NULL;

  /* start search */
  block_ptr = area;

  while( block_ptr < ( area + BYTE_COUNT ) ){
    if( ( *block_ptr == FREE ) && ( PAYLOAD_SIZE >= req_size ) ){
      if( ( PAYLOAD_SIZE - req_size ) < ( MIN_BLOCK_SIZE ) ){
        TOP_STATUS = ALLOCATED;
        BOTTOM_STATUS = ALLOCATED;
        return ( USER_PTR );
      }else{
        remaining_payload_size = PAYLOAD_SIZE - req_size - CONTROL_FIELDS_SIZE;

        /* change bottom size before changing top size */
        BOTTOM_SIZE = remaining_payload_size;

        TOP_STATUS = ALLOCATED;
        TOP_SIZE = req_size;
        BOTTOM_SIZE = req_size;
        BOTTOM_STATUS = ALLOCATED;

        TOP_STATUS_OF_NEXT_BLOCK = FREE;
        TOP_SIZE_OF_NEXT_BLOCK = remaining_payload_size;

        return ( USER_PTR );
      }
    }
    block_ptr += BLOCK_SIZE;
  }

  return NULL;
}

void simple_release( unsigned char *user_ptr ){
  unsigned char *block_ptr = user_ptr - HEADER_SIZE;
  TOP_STATUS = FREE;
  BOTTOM_STATUS = FREE;
}


/* test driver */

int main(){
  unsigned char *p[8];

  simple_init();

  print_blocks();

  p[0] = simple_allocate( 252 ); /* uses all 256 bytes */
  print_blocks();

  simple_release( p[0] );
  print_blocks();

  p[1] = simple_allocate( 12 );  /* uses 16 bytes */
  p[2] = simple_allocate( 12 );  /* uses 16 bytes */
  p[3] = simple_allocate( 12 );  /* uses 16 bytes */
  print_blocks();

  simple_release( p[2] );
  print_blocks();

  simple_release( p[1] ); /* does not coalesce in this version */
  print_blocks();

  return 0;
}
