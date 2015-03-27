#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/malloc.h"


static struct list frame_table;	/* the frame table */
static struct lock ftable_lock;		/* the lock used for frame table */


void frame_table_init(void){
	list_init(&frame_table);
	lock_init(&ftable_lock);
}

struct frame *frame_create(struct thread *t, void *page){
	struct frame *f = malloc(sizeof(struct frame));
	f->t = t;
	f->pte = page;
	return f;
}

void *falloc_get_frame(enum palloc_flags flags){
	void *page = palloc_get_page(PAL_USER | flags);
	if(page != NULL){
		struct frame *f = frame_create(thread_current(),page);
		// add the new frame to frame table
		lock_acquire(&ftable_lock);
		list_push_back(&frame_table, &f->elem);
		lock_release(&ftable_lock);
	} else {
		PANIC("Need to swap frame.");
	}
	return page;
}

void falloc_free_frame(void *page){
	lock_acquire(&ftable_lock);
	struct list_elem *e = list_begin(&frame_table);
	for(;e != list_end(&frame_table); e = list_next(e)){
		struct frame *f = list_entry(e,struct frame,elem);
		if(f->pte == page){
			list_remove(e);
			free(f);
			break;
		}
	}
	lock_release(&ftable_lock);
	palloc_free_page(page);
}

void frame_evict(void *frame){

}