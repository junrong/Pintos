#include "vm/page.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "vm/frame.h"
#include "userprog/pagedir.h"





static bool
install_page(void *upage, void *kpage, bool writable) {
	struct thread *t = thread_current();

	/* Verify that there's not already a page at that virtual
	 address, then map our page there. */
	return (pagedir_get_page(t->pagedir, upage) == NULL
			&& pagedir_set_page(t->pagedir, upage, kpage, writable));
}

unsigned page_hash(const struct hash_elem *e, void *aux UNUSED){
	struct sup_page *spage = hash_entry(e, struct sup_page,elem);
	return hash_int((int)spage->uva);
}

bool page_less(const struct hash_elem *a,const struct hash_elem *b, void *aux UNUSED){
	struct sup_page *spa = hash_entry(a, struct sup_page,elem);
	struct sup_page *spb = hash_entry(b, struct sup_page,elem);
	return spa->uva < spb->uva;
}

void page_destroy(const struct hash_elem *e, void *aux UNUSED){
	struct sup_page *spage = hash_entry(e, struct sup_page,elem);
	if(spage->uva != NULL){
		struct thread *t = thread_current();
		falloc_free_frame(pagedir_get_page(t->pagedir,spage->uva));
		pagedir_clear_page(t->pagedir,spage->uva);
	}
	free(spage);
}

void page_table_init(struct hash *sptable){
	hash_init(sptable,page_hash,page_less,NULL);
}

void page_table_destroy(struct hash *sptable){
	hash_destroy(sptable,page_destroy);
}

struct sup_page *page_find(void *addr){
	struct sup_page spage;
	spage.uva = pg_round_down(addr);
	struct hash_elem *e = hash_find(&thread_current()->sptable,&spage.elem);
	return e == NULL ? NULL : hash_entry(e,struct sup_page,elem);
}

bool page_load_file(struct sup_page *spage){
	ASSERT (spage != NULL);
	ASSERT (spage->loaded.file.file);


    //return the kernel virtual address
	void *addr = pagedir_get_page(thread_current()->pagedir,spage->uva);

	 //Get a frame table of memory.
	void *kpage = falloc_get_frame(PAL_USER);

	if(kpage == NULL)
		return false;

    // supplemental page pointer
	struct sup_page_file *supagefile = &spage->loaded.file;
	ASSERT(supagefile->file != NULL); //Doubt?

	 //Load this page.
	if (file_read(supagefile->file, kpage,
			supagefile->read_bytes) != supagefile->read_bytes) {
		free (spage); ///free supplemental page
		falloc_free_frame(kpage);
		return false;
	}
	memset(kpage + spage->loaded.file.read_bytes, 0, spage->loaded.file.zero_bytes);

	 //Add the page to the process's address space.
	if (!install_page(spage->uva, kpage, spage->writable)) {
		falloc_free_frame(kpage);
		return false;
	}

	return true;
}

bool page_load_swap(struct sup_page *spage){
	return false;
}

bool page_load(void *fault_addr){

	ASSERT (fault_addr < PHYS_BASE);

	struct sup_page *spage = page_find(fault_addr);
	if(spage==NULL)
		return false;
	bool result = false;
	switch(spage->type){
		case FILE:
			result = page_load_file(spage);
			break;
		case SWAP:
			result = page_load_swap(spage);
			break;
	}
	return result;
}

struct sup_page *
create_spage(enum sup_page_type type, void *upage, bool writable){
	ASSERT(upage < PHYS_BASE);

	struct sup_page *spage= malloc(sizeof(struct sup_page));
	if(spage == NULL){
		PANIC("KERNEL POOL FULL, KERNEL PANIC...");
		return NULL;
	}


	spage->type = type;
	spage->uva = pg_round_down(upage);
	spage->writable = writable;

	hash_insert(&thread_current()->sptable, &spage->elem);

	return spage;
};


bool page_memory(void *uaddr){
	ASSERT (uaddr < PHYS_BASE);
	  struct sup_page *spage = create_spage(SWAP, uaddr, true);

	    if (spage == NULL)
	      return false;
	    return true;
}

bool page_table_add_file(struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes, bool writable){

	struct sup_page *spage = create_spage(FILE, upage, writable);

	spage->loaded.file.file = file;
	spage->loaded.file.offset = ofs;
	spage->loaded.file.read_bytes = read_bytes;
	spage->loaded.file.zero_bytes = zero_bytes;

	return true;
}


