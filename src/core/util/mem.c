#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "mem.h"
#include "debug.h"

#ifdef TEST

typedef struct mem_p {
	struct mem_p* next;
	void* ptr;
	size_t size;
	int ct;

} mem_p_t;

static mem_p_t* mem_tracker=NULL;
static int mem_count=0;
static size_t c_mem=0;
static size_t max_mem=0;
static int track_count=-1;

void* t_malloc(size_t size, char *file, const char *func, int line) {
   void* ptr  = malloc(size);
   mem_p_t* t = malloc(sizeof(mem_p_t));
   t->next=mem_tracker;
   t->ptr=ptr;
   t->size=size;
   t->ct=++mem_count;
   t->next=mem_tracker;
   if (track_count==mem_count) 
      printf("Found allocated memory ( %zu bytes ) in %s : %s : %i\n", size,file,func,line);
	mem_tracker=t;
	c_mem+=size;
	if (max_mem<c_mem) max_mem=c_mem;
	return ptr;
}

void* t_calloc(size_t n, size_t size, char *file, const char *func, int line) {
	void* ptr = t_malloc(n*size, file,func,line);
	memset(ptr,0,n*size);
	return ptr;
}

void t_free(void* ptr, char *file, const char *func, int line) {
	if (ptr==NULL)
      printf("trying to free a null-pointer in %s : %s : %i\n", file,func,line);
	  
	mem_p_t* t = mem_tracker, *prev=NULL;
	while (t) {
		if (ptr==t->ptr) {
            c_mem-=t->size;
         	if (max_mem<c_mem) max_mem=c_mem;
			free(ptr);
			if (prev==NULL)
			   mem_tracker = t->next;
			else
			   prev->next = t->next;

			free(t);
			return;
		}
		prev=t;
		t=t->next;
	}

   printf("freeing a pointer which was not allocated anymore %s : %s : %i\n", file,func,line);

}
void* t_realloc(void* ptr,size_t size, char *file, const char *func, int line) {
	if (ptr==NULL)
      printf("trying to free a null-pointer in %s : %s : %i\n", file,func,line);
	
	mem_p_t* t = mem_tracker;
	while (t) {
		if (ptr==t->ptr) {
            c_mem+=size - t->size;
        	if (max_mem<c_mem) max_mem=c_mem;
			t->ptr  = realloc(ptr,size);
			t->size = size;
			return t->ptr;
		}
		t=t->next;
	}
   printf("realloc a pointer which was not allocated anymore %s : %s : %i\n", file,func,line);
   return realloc(ptr,size);
}

size_t mem_get_max_heap() {
	return max_mem;
}


int mem_get_memleak_cnt() {
	if (mem_tracker!=NULL)
  	  return mem_tracker->ct;
	return 0;
}


void mem_reset(int cnt) {
	track_count = cnt;
	max_mem = 0;
	c_mem = 0;
	mem_count = 0;
    /*
	mem_p_t* t = mem_tracker,*n;
	while (t) {
		free(t->ptr);
		n=t;
		t=t->next;
		free(n);
	}
    */
	mem_tracker=NULL;
}

#endif

#ifdef __ZEPHYR__

void *k_realloc(void *ptr, size_t size, size_t oldsize)
{
	void *new;

	new = k_malloc(size);
	if (!new)
		goto error;

	if (ptr && oldsize) {
		memcpy(new, ptr, oldsize);
		k_free(ptr);
	}

	return new;

error:
	return NULL;
}

#endif

