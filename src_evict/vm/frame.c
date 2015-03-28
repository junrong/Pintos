#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include  "userprog/pagedir.h"
#include "userprog/process.h"
#include "vm/page.h"

void frame_table_init(void) {
	list_init(&frame_table);
	lock_init(&ftable_lock);
}

struct frame *frame_create(struct thread *t, void *page) {
	struct frame *f = malloc(sizeof(struct frame));
	f->t = t;
	f->pte = page;
	return f;
}

struct frame *falloc_get_frame(enum palloc_flags flags) {
	uint8_t *page = palloc_get_page(PAL_USER | flags);
	if (page != NULL) {
		struct frame *f = frame_create(thread_current(), page);
		// add the new frame to frame table
		lock_acquire(&ftable_lock);
		list_push_back(&frame_table, &f->elem);
		lock_release(&ftable_lock);
		return f;
	} else {
		//PANIC("Need to swap frame.");
		lock_acquire(&ftable_lock);
		struct frame *f = frame_evict();
		lock_release(&ftable_lock);

		if (f == NULL) {
			PANIC("NO MORE MEMORY TO DO SWAP");
			return NULL;
		}
		return f;
	}
	//return NULL;
}

void falloc_free_frame(struct frame *f) {
	lock_acquire(&ftable_lock);
	list_remove(&f->elem);
	lock_release(&ftable_lock);
	palloc_free_page(f->pte);
	free(f);
}

void frame_evict(void) {
	lock_acquire(&ftable_lock);
	struct list_elem *first_e = list_begin(&frame_table);

	while (1) {
		struct frame *f = list_entry(first_e, struct frame, elem);

		if (!f->pinned) {
			//struct thread *t = f->t;
			if (pagedir_is_accessed(f->t->pagedir, f->spage->uva)) {
				pagedir_set_accessed(f->t->pagedir, f->spage->uva, false);
			} else {
				if (pagedir_is_dirty(f->t->pagedir, f->spage->uva)
						|| (f->spage->type == SWAP)) {
					PANIC("TODO swapout");
					switch (f->spage->type){
					case SWAP:
						f->spage->type = SWAP;
						printf("need to do swap");
						break;
					case FILE: //To-do mmap
						printf("need to to mmap");
						break;
					default:
						PANIC("Unknown location to evict frame to ....");
					}


					//page_evict(f->t, f->spage);

					/*if (fte->spte->type == MMAP)
							    {
							      lock_acquire(&filesys_lock);
							      file_write_at(fte->spte->file, fte->frame,
									    fte->spte->read_bytes,
									    fte->spte->offset);
							      lock_release(&filesys_lock);
							    }
							  else
							    {
							      fte->spte->type = SWAP;
							      fte->spte->swap_index = swap_out(fte->frame);
							    }*/

				} else {
					list_remove(&f->elem);
					//clean page in frame
					pagedir_clear_page(f->t->pagedir, f->spage->uva);
					palloc_free_page(f->pte);
					free(f);
				}

			}
		} else {
			first_e = list_next(first_e);
			if (first_e == list_end(&frame_table)) {
				first_e = list_begin(&frame_table);
			}
		}
	}
}
