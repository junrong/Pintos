#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/palloc.h"

static struct list frame_table;	/* the frame table */
static struct lock ftable_lock;		/* the lock used for frame table */

struct frame
{
	struct thread *t;		/* pointer to the owner thread */
	uint8_t *pte;			/* pointer to the user page */
	struct list_elem elem;	/* list element */

	struct sup_page* spage; /* pointer to the supplemental page table entry*/
	bool pinned;  			 /* if true, the frame should not be evicted*/
};

void frame_table_init(void);
struct frame *falloc_get_frame(enum palloc_flags);
void falloc_free_frame(struct frame *);

#endif /* vm/frame.h */
