#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/palloc.h"

struct frame
{
	struct thread *t;		/* pointer to the owner thread */
	uint8_t *pte;			/* pointer to the user page */
	struct list_elem elem;	/* list element */

	struct sup_page* spage; /* pointer to the supplemental page table entry*/
	bool pinned; /* if true, the frame should not be evicted*/
	//struct sup_page *spte;
};

void frame_table_init(void);
//struct frame *falloc_get_frame(enum palloc_flags);
struct frame *falloc_get_frame(enum palloc_flags flags, struct sup_page *spte);
void falloc_free_frame(struct frame *);
struct frame *frame_evict(enum palloc_flags flags);

#endif /* vm/frame.h */
