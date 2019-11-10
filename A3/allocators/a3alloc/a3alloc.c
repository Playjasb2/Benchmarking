#include <stdlib.h>
#include "memlib.h"

#define PAGES_IN_SUPERBLOCK 2

#define NUM_BLOCK_SIZES 10
const int BLOCK_SIZES[NUM_BLOCK_SIZES] = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 6144 };
#define MAX_BLOCK_SIZE (BLOCK_SIZES[NUM_BLOCK_SIZES - 1])

typedef ptrdiff_t vaddr_t;

struct superblock
{
	unsigned int size;
	superblock* next;
};

struct free_block
{
	free_block* next;
};

struct processor_heap
{
	pthread_mutex_t lock;

	superblock* allocations;
	free_block* free_blocks[NUM_BLOCK_SIZES];
};

struct allocation_header
{
	unsigned int size;
	unsigned int padding;
};

void* page_zero; // page dedicated for heap data

unsigned int superblock_size;
unsigned int num_processors;

pthread_mutex_t global_heap_lock = PTHREAD_MUTEX_INITIALIZER;

processor_heap *processor_heaps;

void initialize()
{
	num_processors = getNumProcessors();
	superblock_size = PAGES_IN_SUPERBLOCK * mem_pagesize();

	page_zero = mem_sbrk(mem_pagesize());
	processor_heaps = (processor_heap*) page_zero;

	for(unsigned int i = 0; i < num_processors; i++)
	{
		memset(&processor_heaps[i], 0, sizeof(processor_heap));
		processor_heaps[i].lock = PTHREAD_MUTEX_INITIALIZER;
	}
}

unsigned int calculate_size_class(size_t sz)
{
	unsigned int size_class = 0;
	for(unsigned int i = 0; i < NUM_BLOCK_SIZES; i++)
	{
		if(sz <= BLOCK_SIZES[i])
		{
			size_class = i;
			break;
		}
	}
	return size_class;
}

void* alloc_superblock()
{
	pthread_mutex_lock(&global_heap_lock);

	superblock* block = (superblock*) mem_sbrk(PAGES_IN_SUPERBLOCK * mem_pagesize());
	if(block != NULL)
	{
		block->size = sizeof(superblock);
		block->next = NULL;
	}

	pthread_mutex_unlock(&global_heap_lock);
	return block;
}

void* alloc_small_block(size_t sz)
{
	void* mem = NULL;
	processor_heap* heap = processor_heaps[getTID()];
	pthread_mutex_lock(&heap->lock);

	unsigned int size_class = calculate_size_class(sz);
	unsigned int size = BLOCK_SIZES[size_class];

	// check if there are any blocks of the size class which are available for reuse
	if(heap->free_blocks[size_class] != NULL)
	{
		mem = heap->free_blocks[size_class];
		heap->free_blocks = heap->free_blocks->next;
	}

	if(mem == NULL)
	{
		// try to find a superblock to place the allocation in
		superblock *block = NULL, *tail = NULL;
		for(block = heap->allocations; block != NULL; block = block->next)
		{
			if(superblock_size - block->size >= size)
			{
				break;
			}
			tail = block;
		}

		// allocate a new superblock and insert it into this heap
		if(block == NULL)
		{
			block = alloc_superblock();
			tail->next = block;
		}

		mem = (void*) block + block->size;
		block->size += size;
	}

	if(mem != NULL)
	{
		allocation_header* header = (allocation_header*) mem;
		header->size = size;
	}

	pthread_mutex_unlock(&heap->lock);
	return mem;
}

void* alloc_large_block(size_t sz)
{
	void* mem = NULL;
	processor_heap* heap = processor_heaps[getTID()];
	pthread_mutex_lock(&heap->lock);
	
	pthread_mutex_unlock(&heap->lock);
	return mem;
}

void *mm_malloc(size_t sz)
{
	void* mem = NULL;
	size_t size = sz + sizeof(allocation_header);

	if(size > MAX_BLOCK_SIZE)
	{
		mem = alloc_small_block(size);
	}
	else
	{
		mem = alloc_large_block(size);
	}

	return (unsigned char*) mem + sizeof(allocation_header);
}

void mm_free(void *ptr)
{
	(void)ptr; /* Avoid warning about unused variable */
}

int mm_init(void)
{
	if(dseg_lo == NULL && dseg_hi == NULL)
	{
		pthread_mutex_lock(&global_heap_lock);

		int result = mem_init();
		if(result == 0)
		{
			initialize();
		}
		
		pthread_mutex_unlock(&global_heap_lock);
		return result;
	}

	return 0;
}

