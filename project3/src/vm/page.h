#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "filesys/file.h"

/* supplemental page type */
enum sup_page_type{
	FILE = 000,
	SWAP = 001
};

struct sup_page_file{
	struct file *file; //file pointer point to file on filesystem
	off_t offset;
	uint32_t read_bytes;
	uint32_t zero_bytes;
};

struct sup_page_swap{
	off_t swap_index;
};

union loaded_by_type{
	struct sup_page_file file;
	struct sup_page_swap swap;
};

struct sup_page{
	enum sup_page_type type;
	void *uva;
	bool writable;
	union loaded_by_type loaded;
	struct hash_elem elem;
};

bool page_load(void *fault_addr);
bool page_table_add_file(struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes, bool writable);

#endif /* vm/page.h */
