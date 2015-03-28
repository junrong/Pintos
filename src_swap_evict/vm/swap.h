#ifndef SWAP_H_
#define SWAP_H_

#include "threads/synch.h"
#include "threads/vaddr.h"
#include <bitmap.h>
#include "devices/block.h"


#define SWAP_FREE 0
#define SWAP_IN_USE 1

#define SECTORS_PER_PAGE (PGSIZE/BLOCK_SECTOR_SIZE)

struct bitmap *swap_t;	/* swap table data structure */
struct lock swap_lock;		/* swap table lock */
struct block *swap_block;



void swap_init (void);

size_t swap_to_slot (uint8_t *kpage);
void swap_page_back(size_t slot_index, uint8_t *vaddr);


#endif /* SWAP_H_ */


