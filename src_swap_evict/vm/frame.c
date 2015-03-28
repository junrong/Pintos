#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"

#include "vm/page.h"
#include "vm/swap.h"

static struct list frame_table; /* the frame table */
static struct lock ftable_lock; /* the lock used for frame table */

void frame_table_init(void) {
	list_init(&frame_table);
	lock_init(&ftable_lock);
}

/*
struct frame *frame_create(struct thread *t, void *page) {
	struct frame *f = malloc(sizeof(struct frame));
	f->t = t;
	f->pte = page;
	return f;
}
*/

void frame_install(void *page, struct sup_page *spte){
	struct frame *f = malloc(sizeof (struct frame));
	f->pte = page;
	f->spage = spte;
	f->t = thread_current();
	lock_acquire(&ftable_lock);
	list_push_back(&frame_table, &f->elem);
	lock_release(&ftable_lock);
}

struct frame *falloc_get_frame(enum palloc_flags flags, struct sup_page *spte) {
	uint8_t *frame_page = palloc_get_page(PAL_USER | flags);
	if (frame_page) {

		frame_install(frame_page, spte);
	} else {
		while (!frame_page) {
			//printf("need to do");
			frame_page = frame_evict(flags);
		}
		if(frame_page == NULL){
			PANIC("Couldn't to swap, the swap is full!");
		}

		frame_install(frame_page, spte);
	}
	return frame_page;
}

void falloc_free_frame(struct frame *f) {
	lock_acquire(&ftable_lock);
	list_remove(&f->elem);
	lock_release(&ftable_lock);
	palloc_free_page(f->pte);
	free(f);
}



struct frame
*frame_evict(enum palloc_flags flags) {
	lock_acquire(&ftable_lock);
	struct list_elem *first_e = list_begin(&frame_table);
	while (1) {
		struct frame *f = list_entry(first_e, struct frame, elem);
		//if (!f->pinned) {

			if (pagedir_is_accessed(f->t->pagedir, f->spage->uva)) {
				pagedir_set_accessed(f->t->pagedir, f->spage->uva, false);
			} else {
				if (pagedir_is_dirty(f->t->pagedir, f->spage->uva)
						|| (f->spage->type == SWAP)) {

					switch (f->spage->type) {
					case SWAP:
						f->spage->type = SWAP;
						f->spage->swap_index = swap_to_slot(f->pte);
						//return f;
						//printf("need to do swap");
						break;
					case FILE: //To-do mmap
						printf("need to to mmap");
						//return f;
						break;
					default:
						PANIC("Unknown location to evict frame to ....");
					}


				}// not dirty

				//f->spage->is_loaded = false;
				list_remove(&f->elem);
					//clean page in frame
					pagedir_clear_page(f->t->pagedir, f->spage->uva);
					palloc_free_page(f->pte);
					free(f);
					return palloc_get_page(flags);
			}// accessed
		//}// pinned

			first_e = list_next(first_e);
			if (first_e == list_end(&frame_table)) {
				first_e = list_begin(&frame_table);
			}//check the if

	}//while

	lock_release(&ftable_lock);
	return NULL;
}
