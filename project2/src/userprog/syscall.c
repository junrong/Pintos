#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

#define CODE_SEGMENT_BOTTOM ((void *) 0x08048000)

static void syscall_handler (struct intr_frame *);

/* check if the given address is a valid user address*/
bool
is_valid_user_address (const void *vaddr) {
	if (vaddr >= PHYS_BASE || vaddr < CODE_SEGMENT_BOTTOM)
		return false;
	else
		return true;
}

/* returns the kernel virtual address corresponding to the user virtual address */
int
get_kernel_virtual_addr (const void *vaddr) {
	if (!is_valid_user_address(vaddr))
		syscall_exit(-1);

	void *kernel_vaddr = pagedir_get_page(thread_current()->pagedir, vaddr);
	if (kernel_vaddr == NULL)
		syscall_exit(-1);

	return (int)kernel_vaddr;
}

/* check if the given buffer address is valid or not. If not, exit */
void
check_buffer_validity (void *buffer, unsigned size) {
	if (!is_valid_user_address((const void *)buffer))
		syscall_exit(-1);

	if (!is_valid_user_address((const void *)(buffer + size - 1)))
		syscall_exit(-1);
}

/* gets the ith arg for a system call */
int
get_arg (struct intr_frame *f, int argc, int i) {
	ASSERT(argc > 0);

	void *cur_esp = f->esp;
	if (argc > 1)
		cur_esp += (argc + 1) * 4;

	// if the arg address is not a valid address, then exit
	int *arg = (int *)cur_esp + i;
	if (!is_valid_user_address((const void *)arg))
		syscall_exit(-1);

	return *arg;
}

/* gets the opened file from the files list of current thread */
struct file *
get_opened_file(int fd) {
	// traverses the opened files list, and returns the opened file with same given fd
	struct list_elem *e = list_begin(&thread_current()->opened_files);
	for (; e != list_end(&thread_current()->opened_files); e = list_next(e)) {
		struct opened_file *of = list_entry(e, struct opened_file, elem);
		if (of->fd == fd)
			return of->file;
	}
	return NULL;
}

/* exit system call */
void
syscall_exit (int exit_status) {
	thread_current()->process->exit_status = exit_status;
	printf ("%s: exit(%d)\n", thread_current()->name, exit_status);
	thread_exit();
}

/* executes a process, and returns pid */
pid_t
syscall_exec (const char *cmd_line) {
	int pid = process_execute(cmd_line);

	struct process *subprocess = get_subprocess(pid);
	if (subprocess == NULL)
		return -1;

	// waits until the subprocess's loading finishes
	if (subprocess->load_status == NOT_LOADED)
		sema_down(&subprocess->loading_sema);

	// if the subprocess can not be loaded successfully, frees it
	if (subprocess->load_status == FAIL_LOADED) {
		list_remove(&subprocess->elem);
		free(subprocess);
		return -1;
	}

	return pid;
}

/* open a new file */
int
syscall_open (const char *name) {
	struct file *file = filesys_open(name);
	// can't open the file, return -1
	if (!file)
		return -1;

	// records the information of opened file
	struct opened_file *opened_file = malloc(sizeof(struct opened_file));
	opened_file->file = file;
	// records the current file descriptor, and increase the fd of current thread by one
	opened_file->fd = thread_current()->fd++;
	// adds this new opened file into the opened file list
	list_push_back(&thread_current()->opened_files, &opened_file->elem);

	return opened_file->fd;
}

/* reads from a file or keyboard, and stores in the given buffer */
int
syscall_read (int fd, void *buffer, unsigned size) {
	// reads from keyboard
	if (fd == STDIN_FILENO) {
		uint8_t *cur_buffer_addr = (uint8_t *)buffer;
		unsigned i = 0;
		while (i < size)
			cur_buffer_addr[i++] = input_getc();
		return size;
	}

	// reads from opened file
	struct file *file = get_opened_file(fd);
	if (!file)
		return -1;

	return file_read(file, buffer, size);
}

/* writes the buffer to a file or the console */
int
syscall_write (int fd, void* buffer, unsigned size) {
	// writes to the console
	if (fd == STDOUT_FILENO) {
		putbuf(buffer, size);
		return size;
	}

	// writes opened file
	struct file *file = get_opened_file(fd);
	if (!file)
		return -1;

	return file_write(file, buffer, size);
}

/* closes the opened file whose fd is the given value */
void
syscall_close (int fd) {
	// traverse the opened file list, and removes the file with the given fd
	struct list_elem *next, *e = list_begin(&thread_current()->opened_files);
	for (; e != list_end(&thread_current()->opened_files); e = next) {
		next = list_next(e);

		// check if the current opened file has the same given fd
		struct opened_file *of = list_entry(e, struct opened_file, elem);
		if (of->fd == fd || fd == CLOSE_ALL_FD) {
			file_close(of->file);
			list_remove(&of->elem);
			free(of);

			if (fd != CLOSE_ALL_FD)
				return;
		}
	}
}


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	switch(*(int*)f->esp) {
		/* Projects 2 and later. */
		case SYS_HALT: {
			shutdown_power_off();
			break;
		}
		case SYS_EXIT: {
			int exit_status = get_arg(f, 1, 1);
			syscall_exit(exit_status);
			break;
		}
		case SYS_EXEC: {
			int cmd_line = get_arg(f, 1, 1);
			cmd_line = get_kernel_virtual_addr((const void *)cmd_line);
			f->eax = syscall_exec((const char *)cmd_line);
			break;
		}
		case SYS_WAIT: {
			int pid = get_arg(f, 1, 1);
			f->eax = process_wait((pid_t)pid);
			break;
		}
		case SYS_CREATE: {
			int name = get_kernel_virtual_addr((const void *)get_arg(f, 2, 1));
			int initial_size = get_arg(f, 2, 2);
			f->eax = filesys_create((const char *)name, (unsigned)initial_size);
			break;
		}
		case SYS_REMOVE: {
			int name = get_kernel_virtual_addr((const void *)get_arg(f, 1, 1));
			f->eax = filesys_remove((const char *)name);
			break;
		}
		case SYS_OPEN: {
			int name = get_kernel_virtual_addr((const void *)get_arg(f, 1, 1));
			f->eax = syscall_open((const char *)name);
			break;
		}
		case SYS_FILESIZE: {
			int fd = get_arg(f, 1, 1);
			struct file *file = get_opened_file(fd);
			f->eax = file == NULL ? -1 : file_length(file);
			break;
		}
		case SYS_READ: {
			int fd = get_arg(f, 3, 1);
			int buffer = get_arg(f, 3, 2);
			int size = get_arg(f, 3, 3);
			check_buffer_validity((void *)buffer, (unsigned)size);
			buffer = get_kernel_virtual_addr((const void *)buffer);
			f->eax = syscall_read(fd, (void *)buffer, (unsigned)size);
			break;
		}
		case SYS_WRITE: {
			int fd = get_arg(f, 3, 1);
			int buffer = get_arg(f, 3, 2);
			int size = get_arg(f, 3, 3);
			check_buffer_validity((void *)buffer, (unsigned)size);
			buffer = get_kernel_virtual_addr((const void *)buffer);
			f->eax = syscall_write(fd, (void *)buffer, (unsigned)size);
			break;
		}
		case SYS_SEEK: {
			int fd = get_arg(f, 2, 1);
			int position = get_arg(f, 2, 2);
			struct file *file = get_opened_file(fd);
			if (f)
				file_seek(file, (int32_t)position);
			break;
		}
		case SYS_TELL: {
			int fd = get_arg(f, 1, 1);
			struct file *file = get_opened_file(fd);
			f->eax = file == NULL ? -1 : file_tell(file);
			break;
		}
		case SYS_CLOSE: {
			int fd = get_arg(f, 1, 1);
			syscall_close(fd);
			break;
		}

		/* Project 3 and optionally project 4. */
		case SYS_MMAP:
			break;
		case SYS_MUNMAP:
			break;

		/* Project 4 only. */
		case SYS_CHDIR:
			break;
		case SYS_MKDIR:
			break;
		case SYS_READDIR:
			break;
		case SYS_ISDIR:
			break;
		case SYS_INUMBER:
			break;
	}
}
