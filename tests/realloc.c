#include <stdlib.h>
#include <stdio.h>

int main()
{
  printf("Running Realloc Test...\n");

  char * ptr = ( char * ) malloc ( 256 );
  printf("checkpoint #1\n");
  ptr = (char*)realloc(ptr,300);
  if (!ptr)
  {
    printf("\\\\failed\n");
    return;
  }
  free( ptr ); 

  return 0;
}
