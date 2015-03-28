#include "vm/swap.h"
//#include "vm/frame.h"

void swap_init (void){
  swap_block = block_get_role(BLOCK_SWAP);
  size_t page_siz;

  if (!swap_block)
	  PANIC ("NO swap block available...");
  else
	  page_siz = block_size (swap_block) / 8;


  swap_t = bitmap_create (page_siz);

  if (!swap_t)
    PANIC (" can not be created..");

  bitmap_set_all(swap_t, SWAP_FREE);

  lock_init (&swap_lock);

}

//swap the frame page to a free slot in swap table
size_t swap_to_slot (uint8_t *kpage){

	lock_acquire(&swap_lock);
	size_t free_slot = bitmap_scan_and_flip (swap_t, 0, 1, SWAP_FREE);

	if(free_slot != BITMAP_ERROR){
		size_t i;
		for(i=0; i<8; i++){
			block_write(swap_block, free_slot * 8+i, kpage +i *BLOCK_SECTOR_SIZE);

		}

	}else{
		PANIC("NO MORE FREE SWAP SLOT...");
	}
	lock_release(&swap_lock);
	return free_slot;
}


//re-load the page back
void swap_page_back(size_t slot_index, uint8_t *vaddr){

	lock_acquire(&swap_lock);
	if (bitmap_test(swap_t, slot_index) == SWAP_FREE){
		PANIC("Trying to read page from a free slot!");
	}

	bitmap_flip(swap_t, slot_index);

	size_t i;
	for (i=0; i<8; i++){
		block_read(swap_block, slot_index*8+i, vaddr+i*BLOCK_SECTOR_SIZE);
	}

	lock_release(&swap_lock);

}








