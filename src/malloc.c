#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)           (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)       ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)
#define BLOCK_SIZE(size)    (size + sizeof(struct _block)) // datasize + block header size

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("sizeof(struct _block) = %lu\n", sizeof(struct _block));
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block *freeList = NULL; /* Free list to track the _blocks available */

#if defined NEXT && NEXT == 0
struct _block *lastAllocation = NULL; /* marks recent block for next fit */
#endif
/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 * 
 * \moves 'last' to the end of the list if not found a suitable block
 * 
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = freeList;
   
   if (freeList) // skip searching if not freelist
   {

#if defined FIT && FIT == 0
      /* First fit */
      while (curr && !(curr->free && curr->size >= size)) 
      {
         *last = curr;
         curr = curr->next;
      }
#endif

#if defined BEST && BEST == 0
      bool flag = true;
      size_t s = 0;
      struct _block *best = NULL;
      while (curr) 
      {
         if (curr->free && curr->size >= size)
         {
            if (flag || curr->size - size < s)
            {
               flag = false;
               s = curr->size - size;
               best = curr;
            }   
         }
         *last = curr;
         curr = curr->next;
      }
      if (best)
      {
         curr = best;
      }
#endif

#if defined WORST && WORST == 0
      bool flag = true;
      size_t s = 0;
      struct _block *worst = NULL;
      while (curr) 
      {
         if (curr->free && curr->size >= size)
         {
            if (flag || curr->size - size > s)
            {
               flag = false;
               s = curr->size - size;
               worst = curr;
               printf("@@@s = %lu, ptr = %p\n",s,worst);
            }   
         }
         *last = curr;
         curr = curr->next;
      }
      if (worst)
      {
         curr = worst;
      }
#endif

#if defined NEXT && NEXT == 0
      while (lastAllocation && !(lastAllocation->free && lastAllocation->size >= size)) 
      {
         lastAllocation  = lastAllocation->next;
      }
      if (!lastAllocation)
      {
         while (curr && !(curr->free && curr->size >= size)) 
         {
            *last = curr;
            curr = curr->next;
         }
         if (curr)
         {
            lastAllocation = curr;
         }
      }
      else
      {
         curr = lastAllocation;
      }
      
#endif

   } // end if freelist

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update freeList if not set */
   if (freeList == NULL) 
   {
      freeList = curr;
   }

   /* Attach new _block to prev _block */
   if (last) 
   {
      last->next = curr;
      curr->prev = last;
   }

   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   ++num_grows;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{
   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = freeList;
   struct _block *next = findFreeBlock(&last, size);

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
      if (next)
      {
         ++num_blocks;
         max_heap += BLOCK_SIZE(size);
      }
   }
   else
   {
      ++num_reuses;
      /* TODO: Split free _block if possible */
      if ( next->size > size + sizeof(struct _block) )
      {
         char *ptr = (char*)next + size;
         struct _block *leftover = (struct _block*)ptr;
         leftover->size = next->size - BLOCK_SIZE(size);
         leftover->free = true;
         leftover->prev = next;
         leftover->next = next->next;
         if (next->next)
         {
            next->next->prev = leftover;
         }
         next->next = leftover;
         ++num_splits;
      }
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }


   /* Mark _block as in use */
   next->free = false;

   ++num_mallocs;
   num_requested += size;
   //printf("allocated %lu Bytes at header = %p, data = %p \n", size, next, BLOCK_DATA(next) );

   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;
   /* TODO: Coalesce free _blocks if needed */
   if (curr->next && curr->next->free)
   {
      //printf("\\\\merging next block\n");
      curr->size += BLOCK_SIZE(curr->next->size);
      curr->next = curr->next->next;
      ++num_coalesces;
   }
   if (curr->prev && curr->prev->free)
   {
      //printf("\\\\merging prev block\n");
      curr->prev->size += BLOCK_SIZE(curr->size);
      curr->prev->next = curr->next;
      ++num_coalesces;
      curr->prev->free = true;
   }

   ++num_frees;
}

void *calloc(size_t nmemb, size_t size)
{
   char *data = (char*)malloc(nmemb * size);
   if (data == NULL)
   {
      return NULL;
   }
   memset(data, 0, nmemb *size);
   return data;
}

void *realloc(void* ptr, size_t size)
{
   if (ptr == NULL)
   {
      return NULL;
   }
   if (size == 0)
   {
      free(ptr);
      return NULL;
   }

   struct _block *header = BLOCK_HEADER(ptr);
   if (header->free)
   {
      return NULL;
   }
   size_t mvsize = header->size > size ? size : header->size; // choose lesser size to move
   printf("mvsize = %lu\n",mvsize);
   char* newData = (char*)malloc(size); // malloc a new block
   if (newData == NULL)
   {
      return NULL;
   }
   memcpy(newData,ptr,mvsize); // copy data to destination
   free(ptr); // free old block
   return newData;
}


/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
