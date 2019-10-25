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
 */

#include <stdio.h>

#define FREE 0
#define ALLOCATED 1

#define BYTE_COUNT 256
#define MIN_PAYLOAD_SIZE 2
#define MIN_BLOCK_SIZE 6

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
    printf( "  top status is    %d\n", *block_ptr );
    printf( "  top size is      %d\n", *(block_ptr+1) );
    printf( "  bottom size is   %d\n", *(block_ptr + *(block_ptr+1) + 2) );
    printf( "  bottom status is %d\n", *(block_ptr + *(block_ptr+1) + 3) );
    block_ptr += *(block_ptr+1) + 4;
  }
}

unsigned char *simple_allocate( unsigned int req_size ){
  unsigned char *block_ptr;
  unsigned int original_payload_size, remaining_payload_size;

  /* immediately reject requests that are too large */
  if( req_size > BYTE_COUNT - 4 ) return NULL;

  /* start search */
  block_ptr = area;

  while( block_ptr < ( area + BYTE_COUNT ) ){
    if( ( *block_ptr == FREE ) && ( *(block_ptr+1) >= req_size ) ){
      if( ( *(block_ptr+1) - req_size ) < MIN_BLOCK_SIZE ){
        *block_ptr = ALLOCATED;
        *(block_ptr + ( *(block_ptr+1) + 3 )) = ALLOCATED;
        return ( block_ptr + 2 );
      }else{
        original_payload_size = *(block_ptr+1);
        remaining_payload_size = *(block_ptr+1) - req_size - 4;
        *block_ptr = ALLOCATED;
        *(block_ptr+1) = req_size;
        *(block_ptr + req_size + 2) = req_size;
        *(block_ptr + req_size + 3) = ALLOCATED;
        *(block_ptr + req_size + 4) = FREE;
        *(block_ptr + req_size + 5) = remaining_payload_size;
        *(block_ptr + original_payload_size + 2) = remaining_payload_size;
        return ( block_ptr + 2 );
      }
    }
    block_ptr += *(block_ptr+1) + 4;
  }

  return NULL;
}

void simple_release( unsigned char *usr_ptr ){
  *(usr_ptr-2) = FREE;
  *(usr_ptr + *(usr_ptr-1) + 1) = FREE;
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
