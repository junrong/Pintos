#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "filesys/file.h"

/* supplemental page type */
enum sup_page_type{
	FILE = 000,
	SWAP = 001,
	MMAP = 010
};


struct sup_page{
	enum sup_page_type type;
	void *uva;
	bool writable;

	// file info
	struct file *file;
	off_t file_offset;
	uint32_t file_read_bytes;
	uint32_t file_zero_bytes;

	// For swap
	  size_t swap_index;

	struct hash_elem elem;
};

void page_table_init(struct hash *sptable);
void page_table_destroy(struct hash *sptable);

bool page_load(void *fault_addr);
bool page_grow_stack(void *esp, void* fault_addr);
bool page_table_add_file(struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes, bool writable);

#endif /* vm/page.h */
