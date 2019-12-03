#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main()
{
  printf("Running Calloc Test...\n");
  char * ptr1 = ( char * ) calloc ( 2, 1024 );
  char * ptr4 = ( char * ) calloc ( 1, 65 );
  char * ptr2 = ( char * ) calloc ( 3, 256 );

  printf("Worst fit should pick this one: %p\n", ptr1 );
  printf("Best fit should pick this one: %p\n", ptr2 );

  free( ptr1 ); 
  free( ptr2 ); 

  ptr4 = ptr4;

  char * ptr3 = ( char * ) calloc ( 2, 128 );
  printf("Chosen address: %p\n", ptr3 );

  return 0;
}