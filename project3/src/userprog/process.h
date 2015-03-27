#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include <list.h>
#include "threads/synch.h"

typedef int tid_t;		// thread id type
typedef int pid_t;		// process id type

enum load_status {
	NOT_LOADED,
	SUCCESS_LOADED,
	FAIL_LOADED
};

struct process {
	/* process id */
	pid_t pid;
	/* the load status of a process */
	enum load_status load_status;
	/* the exit status of a process */
	int exit_status;
	/* indicates if the current process is being waited by its parent process */
	bool is_being_waited;
	/* indicates if the current process exits or not */
	bool is_exit;
	/* semaphore to make sure a process will wait until its subprocess's loading finishes */
	struct semaphore loading_sema;
	/* semaphore to make sure a process could be waited by only one process */
	struct semaphore waiting_sema;
	/* list element */
	struct list_elem elem;
};

#define CLOSE_ALL_FD -1
struct opened_file{
	int fd;					/* file descriptor */
	struct file *file;			/* opened file */
	struct list_elem elem;
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
