#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/palloc.h"

struct frame
{
	struct thread *t;		/* pointer to the owner thread */
	uint32_t *pte;			/* pointer to the user page */
	struct list_elem elem;	/* list element */
};

void frame_table_init(void);
void *falloc_get_frame(enum palloc_flags);
void falloc_free_frame(void *);

#endif /* vm/frame.h */