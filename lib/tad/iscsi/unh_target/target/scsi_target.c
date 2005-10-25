/*	target/scsi_target.c

	vi: set autoindent tabstop=4 shiftwidth=4 :

 	This file has all the functions for scsi midlevel on target side

*/
/*
	Copyright (C) 2001-2004 InterOperability Lab (IOL)
					University of New Hampshier (UNH)
					Durham, NH 03824

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
	USA.

	The name of IOL and/or UNH may not be used to endorse or promote products
	derived from this software without specific prior written permission.
*/

#include <te_config.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <semaphore.h>
#include "scsi_target.h"
#include "../common/list.h"
#include "../common/lun_packing.h"
#include "../common/debug.h"
#include <iscsi_common.h>
#include <iscsi_target.h>

#define FAKED_PAGE_SIZE 4096

/* FUNCTION DECLARATIONS */

void scsi_target_process_thread(void *);

# if defined (FILEIO) || defined (GENERICIO)
static int build_filp_table(void);
static void close_filp_table(void);
# endif

/* Bjorn Thordarson, 9-May-2004 */
/*****
static int get_inquiry_response(Scsi_Request *, int);
static int get_read_capacity_response(Scsi_Request *);
*****/
static int get_inquiry_response(Scsi_Request *, int, int);
static int get_read_capacity_response(Target_Scsi_Cmnd *);


static int abort_notify(struct SM *);
static void aen_notify(int, uint64_t);
static int get_space(Scsi_Request *, int);
static int hand_to_front_end(Target_Scsi_Cmnd *);
static int handle_cmd(struct SC *);

# ifdef GENERICIO
static int fill_sg_hdr(Target_Scsi_Cmnd *, int);
void signal_process_thread(void *);
# endif

# ifdef DISKIO
static void te_cmnd_processed(Scsi_Cmnd *);
static int fill_scsi_device(void);
# endif

#ifdef CONFIG_PROC_FS
static int scsi_target_proc_info(char *buffer, char **start, off_t offset,
								 int length);
static int proc_scsi_target_gen_write(struct file *file, const char *buf,
									  unsigned long length, void *data);
static void build_proc_target_dir_entries(Scsi_Target_Device * the_device);
#endif

struct proc_dir_entry *proc_scsi_target;

/* RDR */
#define MAX_FILE_NAME		64		/* limit on number of chars in file name */
#define MAX_FILE_TARGETS	2		/* number of default file targets */
#define MAX_FILE_LUNS		4		/* number of default file luns per target */

struct target_map_item	{
		struct list_head link;		/* for doubly linked target_map_list */
		int			target_id;		/* ordinal number of this target in list */
		Scsi_Device	*the_device;	/* a "real" scsi device */
		struct file	*the_file;		/* simulating scsi device with this file */
		uint32_t		max_blocks;		/* maximum number of blocks in this file */
		uint32_t		bytes_per_block;/* number of bytes in each block in file */
		char		file_name[MAX_FILE_NAME];
		int			in_use;			/* 1 if this item is defined, else 0 */
	};

	/* doubly-linked circular list, one entry for every iscsi target
	 * known to the scsi subsystem on this platform
	 */
	struct list_head target_map_list;

	/* matrix with one entry for every possible target and lun */
	static struct target_map_item target_map[MAX_TARGETS][MAX_LUNS];

/* mutex to control access to target_map array */
pthread_mutex_t target_map_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Scsi_Target_Device: Maximum number of "active" targets at any time
 * Scsi_Target_Template: Maximum number of templates available
 *
 * The naming convention for variables is with the prefix "st" following
 * Eric's comments in the SCSI Initiator mid-level.
 *
 * The idea with the following variables is to have a generic linked
 * list of templates - there is one template for one "type" of device
 * whereas for each device there is a single entry in the device list
 */

Target_Emulator target_data;

/*
 * okay the following is to get something off the ground and working.
 * In the mode that the emulator is talking to a real disk drive, there
 * will be a need to map the id given to us by rx_cmnd to host, channel,
 * id as far as the SCSI mid-level is concerned. For the present moment
 * we map all of these to the same target. So all commands are directed
 * to different LUNs on a single device. I visualize this as being a
 * linked list of scsi disks attached to the system and whenever a
 * command is received, the rx_cmnd function does some id_lookup and map
 * the individual commands to one in the linked list. How this mapping
 * gets done is an upper level management function that I do not want to
 * deal with yet	ASHISH
 */
int target_count = 0;

# ifdef GENERICIO
char device[BYTE];
# endif

# ifdef FILEIO
char device[BYTE * 8];
# endif

/*
 * scsi_target_init_module: Initializes the SCSI target module
 * FUNCTION: carry out all the initialization stuff that is needed
 * INPUT: none
 * OUTPUT: 0: everything is okay
 * 	      else trouble
 */
int
scsi_target_init(void)
{

	/* initialize the global target_data struct */
	/*
	 * initialize the target semaphore as locked because you want
	 * the thread to block after entering the first loop
	 */
	pthread_mutex_init(&target_data.msg_lock, NULL);
	target_data.st_device_list = NULL;
	target_data.st_target_template = NULL;
	pthread_mutex_init(&target_data.cmd_queue_lock, NULL);
	INIT_LIST_HEAD(&target_data.cmd_queue);
	target_data.msgq_start = NULL;
	target_data.msgq_end = NULL;
	target_data.command_id = 0;

	INIT_LIST_HEAD(&target_map_list);


#if 0
    /*
	 * create a list of devices that are available. This is a very
	 * simplistic mapping of these devices. Each new device will
	 * represent a new LUN. A new mapping function will have to
	 * solve this problem - LATER - Ashish
	 */
	if (build_filp_table() < 0) {
		TRACE_ERROR("scsi_target_init_module: build_filp_table returned an error\n");
		return -1;
	}
#endif
    {/* in memory mode, all luns in all targets are always in use */
        uint32_t targ, lun;
        
        for (targ = 0; targ < MAX_TARGETS; targ++) {
            for (lun = 0; lun < MAX_LUNS; lun++) {
                target_map[targ][lun].in_use = 1;
            }
        }
        target_count = MAX_TARGETS * MAX_LUNS;
    }


    return 0;
}

/*
 * scsi_target_cleanup_module: Removal of the module from the kernel
 * FUNCTION: carry out the cleanup stuff
 * INPUT: None allowed
 * OUTPUT: None allowed
 */
void
scsi_target_cleanup(void)
{

	struct list_head *lptr, *next;
	struct target_map_item *this_item;
    
	list_for_each_safe(lptr, next, &target_map_list) {
		list_del(lptr);
		this_item = list_entry(lptr, struct target_map_item, link);
		free(this_item);
	}

}

/*
 * make_target_front_end:
 * FUNCTION: 	to register the individual device with the mid-level
 * 		to allocate the device an device id etc
 * 		start a thread that will be responsible for the target
 * INPUT:	pointer to a struct STD
 * OUTPUT:	Scsi_Target_Device - if everything is okay
 * 		else NULL if there is trouble
 */
Scsi_Target_Device *
make_target_front_end(void)
{
	Scsi_Target_Device *the_device;

	the_device = malloc(sizeof(Scsi_Target_Device));
	if (!the_device) {
        TRACE_ERROR
			("register_target_front_end: Could not allocate space for the device\n");
		return NULL;
	}

	the_device->next = target_data.st_device_list;

	if (the_device->next)
		the_device->id = the_device->next->id + 1;
	else
		the_device->id = 0;		/* first device */

	target_data.st_device_list = the_device;

	return the_device;
}

/*
 * destroy_target_front_end:
 * FUNCTION:	to allow removal of the individual device from the
 * 		midlevel
 * 		free up the device id number for future use
 * 		close the thread responsible for the target after making
 * 		sure that all existing commands have been responded to
 * 		we need to do serious error checking since this can be
 * 		called by different front-ends
 * 		CANNOT BE CALLED FROM INTERRUPT CONTEXT
 * INPUT:	pointer to a struct STD to be removed
 * OUTPUT:	int - 0 if everything is okay
 * 		else < 0 if there is trouble
 */
int
destroy_target_front_end(Scsi_Target_Device * the_device)
{
	Scsi_Target_Device *curr, *previous = NULL;
	Target_Scsi_Cmnd *cmnd;
#if 0
	unsigned long flags;
#endif

	if (!the_device) {
        TRACE_ERROR
			("dereg...end: cannot remove NULL devices corresponding to a NULL template\n");
		return -1;
	}


	/*
	 * go through the device list till we get to this device and
	 * then remove it from the list
	 */
	for (curr = target_data.st_device_list; curr != NULL; curr = curr->next) {
		if (curr == the_device) {
			break;
		} else
			previous = curr;
	}

	if (!curr) {				/* No match found */
		TRACE_ERROR("dereg..end: No match found\n");
		return -1;
	}

	/* remove it from the list */
	if (previous)				/* not the first device */
		previous->next = curr->next;

	else
		target_data.st_device_list = curr->next;

	/* release the device */
#if 0
    iscsi_release(curr);
#endif

#if 0
	/* mark all commands corresponding to this device for dequeuing */
	spin_lock_irqsave(&target_data.cmd_queue_lock, flags);
#endif

	list_for_each_entry(cmnd, &target_data.cmd_queue, link) {
		if (cmnd->dev_id == curr->id)
			cmnd->state = ST_DEQUEUE;
	}

#if 0
	spin_unlock_irqrestore(&target_data.cmd_queue_lock, flags);
#endif

	/* wake up scsi_target_process_thread so it can dequeue stuff */
    
#if 0
	if (atomic_read(&target_data.target_sem.count) <= 0) {
		up(&target_data.target_sem);
	}
#endif

	/* freeing things */
	free(curr);

	return 0;
}

# ifdef GENERICIO
# define SETFL_MASK (O_APPEND | O_NONBLOCK | O_NDELAY | FASYNC)

/*
 * signal_process_thread: This thread is responsible for receiving SIGIO
 * signals when SCSI generic is used for processing. This receives the
 * SIGIO signal, changes the state of the oldest command that is in the
 * ST_PROCESSING state and goes back to sleep. I found this to be
 * necessary because signals tend to screw up the processing of the scsi
 * generic commands. Hopefully this will clear things up
 */
void
signal_process_thread(void *param)
{
	int i, sigioflag = 0;
	struct file *dev_file;
	Target_Scsi_Cmnd *cmd_curr;
	unsigned long arg;
	unsigned long flags;
	uint32_t targ, lun;

	lock_kernel();

/* Ming Zhang, mingz@ele.uri.edu */
#ifdef K26
	daemonize(
#else
	daemonize();
	strcpy(current->comm,
#endif
		   "signal_thread");

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 4, 18)
	/*
	 * Arne Redlich, agr1@users.sourceforge.net:
	 * This prevents the signal thread from becoming a zombie after it exits.
	 */
	reparent_to_init();
#endif

	/* also we want to be able to receive SIGIO */
	siginitsetinv(&current->blocked, IO_SIGS);

	/* mark that this signal thread is alive */
	target_data.signal_id = current;

	unlock_kernel();

	printk("%s Starting pid %d\n", current->comm, current->pid);

	/*
	 * we must now prepare this thread to receive the SIGIO signal
	 * for the opened SCSI device(s)
	 */
	down_interruptible(&target_map_sem);
	for (targ = 0; targ < MAX_TARGETS; targ++) {
		for (lun = 0; lun < MAX_LUNS; lun++) {
			if ((dev_file = target_map[targ][lun].the_file)) {
				arg = dev_file->f_flags | FASYNC;
				lock_kernel();

				/* equal to fcntl (fd, F_SETOWN, getpid()) */
				dev_file->f_owner.pid = current->pid;
				dev_file->f_owner.uid = current->uid;
				dev_file->f_owner.euid = current->euid;

				/* equal to fcntl (fd,F_SETFL,flags|O_ASYNC) */
				if ((arg ^ dev_file->f_flags) & FASYNC) {
					if (dev_file->f_op && dev_file->f_op->fasync) {
						i = dev_file->f_op->fasync(0, dev_file,
												   (arg & FASYNC) != 0);
						if (i < 0) {
							unlock_kernel();
							up(&target_map_sem);
							printk("si..thread: fasync returned %d\n", i);
							return;
						}
					}
				}

				/* required for strict SunOS emulation */
				/*
				   if (O_NONBLOCK != O_NDELAY)
				   if (arg & O_NDELAY)
				   arg |= O_NONBLOCK;
				 */
				dev_file->f_flags =
					(arg & SETFL_MASK) | (dev_file->f_flags & ~SETFL_MASK);
				unlock_kernel();
			}
		}
	}
	up(&target_map_sem);

	while (1) {
		down_interruptible(&target_data.sig_thr_sem);

		sigioflag = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 4, 18)
		if (signal_pending(current)) {
			spin_lock_irq(&current->sigmask_lock);
			if (sigismember(&current->pending.signal, SIGIO)) {
# ifdef DEBUG_SCSI_THREAD
				printk("SIGIO received\n");
# endif
				sigioflag = 1;
				sigdelset(&current->pending.signal, SIGIO);
			}
			recalc_sigpending(current);
			spin_unlock_irq(&current->sigmask_lock);
			if (!sigioflag) {
				printk("si...thread: time to die\n");
				break;
			}
		}
#else
		if (signal_pending(current)) {
			spin_lock_irq(&current->sighand->siglock);
			if (sigtestsetmask(&current->pending.signal, sigmask(SIGIO))) {
# ifdef DEBUG_SCSI_THREAD
				printk("SIGIO received\n");
# endif
				sigioflag = 1;
				sigdelsetmask(&current->pending.signal, sigmask(SIGIO));
			}
			recalc_sigpending_tsk(current);
			spin_unlock_irq(&current->sighand->siglock);
			if (!sigioflag) {
				printk("si...thread: time to die\n");
				break;
			}
		}
#endif

		/* change the state of all PROCESSING commands */
		spin_lock_irqsave(&target_data.cmd_queue_lock, flags);
		list_for_each_entry(cmd_curr, &target_data.cmd_queue, link) {
			if (cmd_curr->state == ST_PROCESSING) {
				cmd_curr->state = ST_PROCESSED;
				/* wake up scsi_target_process_thread */
				if (atomic_read(&target_data.target_sem.count) <= 0) {
					up(&target_data.target_sem);
				}
			}
		}
		spin_unlock_irqrestore(&target_data.cmd_queue_lock, flags);
	}
	up(&target_data.signal_sem);
	printk("%s Exiting pid %d\n", current->comm, current->pid);
}
# endif

/*
 * scsi_target_process_thread: this is the mid-level target thread that
 * is responsible for processing commands.
 */
void
scsi_target_process(void)
{
	int i, found;
# ifdef GENERICIO
	mm_segment_t old_fs;
	struct file *dev_file;
# endif
	struct scatterlist *st_list;
	Target_Scsi_Cmnd *cmd_curr;
	Target_Scsi_Message *msg;
	uint32_t lun;
# ifdef DISKIO
	Scsi_Device *this_device = NULL;
	struct target_map_item *this_item;
# endif
	struct list_head *lptr, *next;

    /* is message received */
    while (target_data.msgq_start) {
        /* house keeping */
        pthread_mutex_lock(&target_data.msg_lock);
        msg = target_data.msgq_start;
        target_data.msgq_start = msg->next;
        if (!target_data.msgq_start)
            target_data.msgq_end = NULL;
        pthread_mutex_unlock(&target_data.msg_lock);

			/* execute function */
        switch (msg->message) 
        {
			case TMF_ABORT_TASK:
            {
                Target_Scsi_Cmnd *cmnd;
                cmnd = (Target_Scsi_Cmnd *)msg->value;
                found = 0;
                pthread_mutex_lock(&target_data.cmd_queue_lock);
                list_for_each_entry(cmd_curr,&target_data.cmd_queue,link) {
                    if ((cmd_curr->id == cmnd->id)
                        && (cmd_curr->lun == cmnd->lun)) {
                        found = 1;
                        break;
                    }
                }
                pthread_mutex_unlock(&target_data.cmd_queue_lock);

                if (found) {
                    cmd_curr->abort_code = CMND_ABORTED;
                    //if (cmd_curr->state != ST_PROCESSING)
                    //	cmd_curr->state = ST_DEQUEUE;
                    if (abort_notify(msg)) {
                        TRACE_ERROR("err aborting command with id %d lun %d\n",
                                    cmd_curr->id, cmd_curr->lun);
                        goto scsi_thread_out;
                    }
                } else
                    TRACE_ERROR("no command with id %d lun %d in list\n",
                                cmnd->id, cmnd->lun);
                break;
            }
            
			case TMF_LUN_RESET:
            {
                /* BAD BAD REALLY BAD */
                uint64_t lun = *((uint64_t *)msg->value);
                
                pthread_mutex_lock(&target_data.cmd_queue_lock);
                list_for_each_entry(cmd_curr,&target_data.cmd_queue,link) {
                    if (cmd_curr->lun == lun)
                        scsi_release(cmd_curr);
                }
                pthread_mutex_unlock(&target_data.cmd_queue_lock);
                
                aen_notify(msg->message, lun);
                break;
            }
            
			case TMF_TARGET_WARM_RESET:
			case TMF_TARGET_COLD_RESET:
            {
                pthread_mutex_lock(&target_data.cmd_queue_lock);
                list_for_each_entry(cmd_curr,&target_data.cmd_queue,link) {
                    scsi_release(cmd_curr);
                }
                pthread_mutex_unlock(&target_data.cmd_queue_lock);
                
                aen_notify(msg->message, 0);
					break;
            }
            
			default:
            {
                TRACE_ERROR("Bad message code %d\n",
                            msg->message);
                break;
            }
        }	/* switch */

        free(msg);
        msg = NULL;
    }

    /* There is a harmless race here.
     * This loop is the ONLY place a command can be removed from the queue.
     * So once we get lptr, it cannot be made invalid elsewhere.
     * However, if a new element is added to the end of the queue
     * (the ONLY place new elements are ever added) as lptr is being
     * picked up or as next is being picked up or after next has been
     * picked up, then we might not see this new element on the
     * next iteration of this loop.
     * That should not cause a problem, because the ONLY place a new
     * command can be added to the queue is in rx_cmnd(), and after
     * adding the new command to the queue, rx_cmnd() also does an "up"
     * on target_sem, which will restart the outer loop of this thread
     * and we will be back into this loop again in no time.
     * Why do we say this race is harmless?
     * The loop initialization generated by list_for_each_safe:
     *		lptr = (&target_data.cmd_queue)->next
     * and the code:
     *		next = lptr->next;
     * will always work properly, because list_add_tail() always sets
     * the referenced "next" field last when adding a new element to
     * the queue (i.e., AFTER all other pointer fields are set up
     * correctly).   Therefore, there is a race with list_add_tail(),
     * but we always get a pointer to a valid structure (either the head
     * of the list, in which case we don't process the new element, or
     * the new element which is completely filled in, in which case we
     * do process the new element).
     */
    list_for_each_safe(lptr, next, &target_data.cmd_queue) {
        cmd_curr = list_entry(lptr, Target_Scsi_Cmnd, link);
        /* is command received */
        if (cmd_curr->state == ST_NEW_CMND) 
        {
            lun = cmd_curr->lun;
            
            cmd_curr->req = (Scsi_Request *)malloc(sizeof(Scsi_Request));
            
            if (cmd_curr->req) 
            {
                memset(cmd_curr->req, 0, sizeof(Scsi_Request));
                
                if (lun >= MAX_LUNS)
                    cmd_curr->req->sr_allowed = 1;
            }
            
            if (!cmd_curr->req) 
            {
                TRACE_ERROR("no space for Scsi_Request\n");
                goto scsi_thread_out;
            }
            
            memcpy(cmd_curr->req->sr_cmnd, cmd_curr->cmd, cmd_curr->len);
            
            if (handle_cmd(cmd_curr)) 
            {
                TRACE_ERROR("error in handle_cmd for command %d\n",
                            cmd_curr->id);
                /* is bailing out a good idea? */
                goto scsi_thread_out;
            }
        }
        /* is a command pending */
        if (cmd_curr->state == ST_PENDING) {
            /* call the rdy_to_xfer function */
            if (hand_to_front_end(cmd_curr)) {
                TRACE_ERROR("error in hand_to_front_end for command %d\n",
                       cmd_curr->id);
                goto scsi_thread_out;
            }
        }
        /* is data received */
        if (cmd_curr->state == ST_TO_PROCESS) {
            /*
             * we have received the data - does this
             * go off to handle_cmd again ?
             */
            if (handle_cmd(cmd_curr)) {
                TRACE_ERROR("error in handle_cmd for command %d\n",
                       cmd_curr->id);
                /* is bailing out a good idea? */
                goto scsi_thread_out;
            }
        }
# ifdef GENERICIO
    /* is command processed */
        if (cmd_curr->state == ST_PROCESSED) {
# ifdef DEBUG_SCSI_THREAD
            printk("%s command %p id %d processed\n",
                   current->comm, cmd_curr, cmd_curr->id);
# endif
            /* we have some reading to do */
            dev_file = cmd_curr->fd;
            if ((dev_file) && (dev_file->f_op) && (dev_file->f_op->read)) {
                old_fs = get_fs();
                set_fs(get_ds());
            i = dev_file->f_op->read(dev_file,
                                     (uint8_t *) cmd_curr->sg,
                                     sizeof(sg_io_hdr_t),
                                     &dev_file->f_pos);
            set_fs(old_fs);
            if ((i < 0) && (i != -EAGAIN)) {
                printk("%s read error %d\n", current->comm, i);
                goto scsi_thread_out;
            }
            cmd_curr->state = ST_DONE;
            }
        }
# endif
        /* is command done */
        if (cmd_curr->state == ST_DONE) {
        /*
         * hand it off to the front end driver
         * to transmit
         */
        
            if (hand_to_front_end(cmd_curr)) {
                TRACE_ERROR("error in hand_to_front_end for command %d\n",
                            cmd_curr->id);
                goto scsi_thread_out;
            }
        }
    
        /* can command be dequeued */
        if (cmd_curr->state == ST_DEQUEUE) {
            /*
             * dequeue the command and free it
             */
# ifdef GENERICIO
            /* free up the SCSI generic stuff */
            if (cmd_curr->sg->dxferp)
                kfree(cmd_curr->sg->dxferp);
            kfree(cmd_curr->sg->sbp);
            kfree(cmd_curr->sg);
# endif
            if (cmd_curr->req) {
                /* free up pages */
                st_list = (struct scatterlist *) cmd_curr->req->sr_buffer;
                for (i = 0; i < cmd_curr->req->sr_use_sg; i++) {
                    free(st_list[i].address);
            }
                
                /* free up scatterlist */
                if (cmd_curr->req->sr_use_sg)
                    free(st_list);
                
                /* free up Scsi_Request */
                free(cmd_curr->req);
            }
            
            /* dequeue and free up Target_Scsi_Cmnd */
            pthread_mutex_lock(&target_data.cmd_queue_lock);
            list_del(lptr);
            pthread_mutex_unlock(&target_data.cmd_queue_lock);
            free(cmd_curr);
        }

# ifdef DEBUG_SCSI_THREAD
		printk("%s going back to sleep again\n", current->comm);
# endif
	}
scsi_thread_out:
	return;
}

/*
 * rx_cmnd: this function is the basic function called by any front end
 * when it receives a SCSI command. The rx_cmnd function then fills up
 * a Target_Scsi_Cmnd struct, adds to the queue list, awakens the mid-
 * level thread and then returns the struct to the front end driver
 * INPUT:	device, target_id, lun, SCSI CDB as an unsigned char
 * 		length of the command (or size of scsi_cdb array if
 * 		unavailable
 * OUTPUT:	Target_Scsi_Cmnd struct or NULL if things go wrong
 */
Target_Scsi_Cmnd *
rx_cmnd(Scsi_Target_Device * device, uint64_t target_id,
	uint64_t lun, uint8_t *scsi_cdb, int len, int datalen, int in_flags,
	Target_Scsi_Cmnd **result_command)
{
	Target_Scsi_Cmnd *command;
#if 0
	unsigned long flags;
#endif

	*result_command = NULL;

	if (!device) {
		TRACE_ERROR("rx_cmnd: No device given !!!!\n");
		return NULL;
	}

	*result_command = command =
		(Target_Scsi_Cmnd *)malloc(sizeof(Target_Scsi_Cmnd));
    
	if (command == NULL) {
		TRACE_ERROR("rx_cmnd: No space for command\n");
		/* sendsig (SIGKILL, target_data.thread_id, 0); */
		return NULL;
	}
# ifdef DEBUG_RX_CMND
	printk("rx_cmnd: filling up command struct %p\n", command);
# endif

	/* fill in Target_Scsi_Cmnd */
	command->req = NULL;
	command->state = ST_NEW_CMND;
	command->abort_code = CMND_OPEN;
	command->device = device;
	command->dev_id = device->id;

	/*ramesh@global.com
	   added data length and flgs to the command structure */
	command->datalen = datalen;
	command->flags = in_flags;

	/* cdeng change target_id later if lun doesn't match */
	command->target_id = target_id;
	command->lun = unpack_lun((uint8_t *)&lun);
	INIT_LIST_HEAD(&command->link);
	if ((len <= MAX_COMMAND_SIZE) && (len > 0))
		command->len = len;
	else {
		command->len = MAX_COMMAND_SIZE;
	}
	memcpy(command->cmd, scsi_cdb, command->len);
    
# if defined (FILEIO) || defined (GENERICIO)
	/* fill in the file pointer */
	command->fd = NULL;
	if (command->target_id < MAX_TARGETS && command->lun < MAX_LUNS
		&& !down_interruptible(&target_map_sem)) {
			struct target_map_item *this_item;

			this_item = &target_map[command->target_id][command->lun];
			if (this_item->in_use) {
				command->fd = this_item->the_file;
# if defined (GENERICIO)
				command->blk_size = this_item->bytes_per_block;
# endif
		}
		up(&target_map_sem);
	}

	if (command->fd == NULL) {
		printk("%s No target for command with targetid %u, lun %u\n",
			   current->comm, command->target_id, command->lun);

		/*
		 * Arne Redlich <agr1@sourceforge.net>:
		 * kfree()'ing the command and returning NULL here will cause
		 * the connection / session to be killed, rendering the target
		 * unusable with initiators that don't understand/use the
		 * REPORT_LUNS response and instead rely on probing LUNs by
		 * INQUIRYs to a range of (possibly not available) LUNs
		 * - which is actually the case with the Cisco (3.4.3)
		 * and UNH (1.5.3, 1.5.4) initiators. Instead, doing nothing
		 * here and letting handle_cmd() generate a TYPE_NO_LUN INQUIRY
		 * response catches this quite gracefully.
		 */

	}
# endif

    pthread_mutex_lock(&target_data.cmd_queue_lock);

	command->id = ++target_data.command_id;
	/* check this to make sure you dont have a command with this id ?????
	 * IGNORE FOR NOW
	 */
	if (!command->id) {
		/* FOR WRAP AROUNDS */
		command->id = ++target_data.command_id;
	}

	list_add_tail(&command->link, &target_data.cmd_queue);

    pthread_mutex_unlock(&target_data.cmd_queue_lock);

	/* wake up scsi_target_process_thread */
    scsi_target_process();

	return command;
}

/*
 * scsi_rx_data: This function is called by the lower-level driver to
 * tell the mid-level that data corresponding to a command has been
 * received. This function can be called from within an interrupt
 * handler (??). All this function does currently is to change the
 * state of a command and then wake the mid-level thread to deal with
 * this command.
 * INPUT: scsi command for which data has been received (MUST not be NULL)
 * OUTPUT: 0 if okay else < 0
 */
int
scsi_rx_data(Target_Scsi_Cmnd * the_command)
{
	the_command->state = ST_TO_PROCESS;

#if 0
	/* wake up the mid-level scsi_target_process_thread */
	if (atomic_read(&target_data.target_sem.count) <= 0) {
		up(&target_data.target_sem);
	}
#endif

	return 0;
}

/*
 * scsi_target_done: This is the function called by the low-level driver
 * to signify that it has completed the execution of a given scsi cmnd
 * This function needs to remove the resources that have been allocated
 * to the given command, dequeue the command etc. This function will be
 * called from within the context of an interrupt handler. So, it may
 * be best to leave it up to the mid-level thread to actually deal with
 * these functions. Right now, I am setting it up so that the status of
 * the command is changed and the mid-level is awakened.
 * INPUT: scsi command to be dequeued
 * OUTPUT: 0 if everything is okay
 * 	   < 0 if there is trouble
 */
int
scsi_target_done(Target_Scsi_Cmnd * the_command)
{
	the_command->state = ST_DEQUEUE;

	/* awaken scsi_target_process_thread to dequeue stuff */
#if 0
	if (atomic_read(&target_data.target_sem.count) <= 0) {
		up(&target_data.target_sem);
	}
#endif

	return 0;
}

/*
 * scsi_release: This function is called by a low-level driver when it
 * determines that it does not responses to certain commands. Situations
 * like this happen when for instance a LIP is received in a Fibre
 * Channel Loop or when a Logout is received in iSCSI before command
 * execution is completed. The low-level driver may no longer care about
 * receiving responses for those commands. This function can be called
 * from within interrupt context
 * INPUT: command to release
 * OUTPUT: 0 if success, < 0 if there is trouble
 */
int
scsi_release(Target_Scsi_Cmnd * cmnd)
{
	cmnd->abort_code = CMND_RELEASED;

	/*
	 * if a command is processing, it is not nice to
	 * dequeue a command, as we will get a response
	 * anyways. This may be an inherent race condn
	 * We catch what we can and move on. A second
	 * check performed when hand_to_front_end is
	 * called should catch the remaining commands
	 * - Ashish
	 */
	if (cmnd->state != ST_PROCESSING)
		cmnd->state = ST_DEQUEUE;

	/* wake up scsi_process_target_thread so it can dequeue stuff */
#if 0
	if (atomic_read(&target_data.target_sem.count) <= 0) {
		up(&target_data.target_sem);
	}
#endif

	return 0;
}

/*
 * rx_task_mgmt_fn: This function is called by a low-level driver to
 * indicate to the Mid-level that it has received a management function.
 * This function will decide the action to be taken in response to this
 * management function. This function will in turn create a message list
 * in the mid-level. CAN BE CALLED FROM INTERRUPT CONTEXT
 * INPUT: device, function, command - if relevant
 * OUTPUT: message or NULL if there is trouble
 * Definition of value:
 * 1. ABORT TASK
 * 	value = Target_Scsi_Cmnd
 * 2. ABORT TASK SET
 * 	This function cannot be directly performed since the Mid-Level
 * 	has no knowledge of what Initiators are logged in to the front-
 * 	end. This functionality can be achieved by issuing ABORTS to
 * 	various commands identified within the Task Set by the front
 * 	-end. value = N/A
 * 3. CLEAR ACA
 * 	I dont understand this one
 * 4. CLEAR TASK SET
 * 	Implement similar to ABORT Task Set - I know that all semantics
 * 	for this cannot be implemented (Any ideas ?)
 * 5. LUN RESET
 * 	set value = pointer to LUN
 * 6. TARGET RESET
 * 	set value = NULL
 */
struct SM *
rx_task_mgmt_fn(struct Scsi_Target_Device *dev, int fn, void *value)
{
#if 0
	unsigned long flags;
#endif
	Target_Scsi_Message *msg;

	if ((fn < TMF_ABORT_TASK) && (fn > TMF_TASK_REASSIGN)) {
		TRACE_ERROR("rx_task_mgmt_fn: Invalid value %d for Task Mgmt function\n",
			   fn);
		return NULL;
	}
	if ((fn == TMF_ABORT_TASK_SET) || (fn == TMF_CLEAR_ACA)
		|| (fn == TMF_CLEAR_TASK_SET)) {
		TRACE_ERROR("rx_task_mgmt_fn: task mgmt function %d not implemented\n",
			   fn);
		return NULL;
	}
	if ((fn == TMF_ABORT_TASK) && (value == NULL)) {
		TRACE_ERROR("rx_task_mgmt_fn: Cannot abort a NULL command\n");
		return NULL;
	}

	msg = malloc(sizeof(Target_Scsi_Message));
	if (!msg) {
		TRACE_ERROR("rx_task_mgmt_fn: no space for scsi message\n");
		return NULL;
	}

	msg->next = NULL;
	msg->prev = NULL;
	msg->device = dev;
	msg->value = value;
	msg->message = fn;

    pthread_mutex_lock(&target_data.cmd_queue_lock);
	if (!target_data.msgq_start) {
		target_data.msgq_start = target_data.msgq_end = msg;
	} else {
		target_data.msgq_end->next = msg;
		target_data.msgq_end = msg;
	}
    pthread_mutex_unlock(&target_data.cmd_queue_lock);

	/* wake up scsi_target_process_thread */
#if 0
	if (atomic_read(&target_data.target_sem.count) <= 0) {
		up(&target_data.target_sem);
	}
#endif

	return msg;
}

#if 0
/*
 * build_filp_table: builds up a table of open file descriptors.
 * INPUT: nothing
 * OUTPUT: 0 if all ok, < 0 if there was trouble
 */
static int
build_filp_table(void)
{
	char *tmp;
	int error;
	int targ, lun, max_files, max_luns;
	mm_segment_t old_fs;
	struct file *dev_file;
	struct target_map_item *this_item;

	max_files = MAX_FILE_TARGETS;
	if (max_files > MAX_TARGETS)
		max_files = MAX_TARGETS;
	max_luns = MAX_FILE_LUNS;
	if (max_luns > MAX_LUNS)
		max_luns = MAX_LUNS;
	for (targ = 0; targ < max_files; targ++) {
		for (lun = 0; lun < max_luns; lun++) {
			this_item = &target_map[targ][lun];
# if defined (GENERICIO)
			if (targ == 0)
				sprintf(this_item->file_name, "/dev/sg%d", lun);
			else
				sprintf(this_item->file_name, "dev/sg%d%d", targ, lun);
# endif
# if defined (FILEIO)
			sprintf(this_item->file_name, "/var/lib/unh-iscsi/dev/" "scsi_disk_file_%d_%d", targ, lun);
# endif
			printk("opening device %s\n", this_item->file_name);
			old_fs = get_fs();
			set_fs(get_ds());
			tmp = getname(this_item->file_name);
			set_fs(old_fs);
			error = PTR_ERR(tmp);
			if (IS_ERR(tmp)) {
				printk("build_filp_table: getname returned error %d\n", error);
				return -1;
			}

			/* open a file or a scsi_generic device */
# ifdef GENERICIO
			dev_file = filp_open(tmp, O_RDWR | O_NONBLOCK, 0600);
# endif

# ifdef FILEIO
			dev_file = filp_open(tmp, O_RDWR | O_NONBLOCK | O_CREAT, 0600);
# endif

			putname(tmp);
			if (IS_ERR(dev_file)) {
				error = PTR_ERR(dev_file);
				if (targ + lun == 0) {
					printk("build_filp_table: filp_open returned error %d\n",
							error);
					return -1;
				} else
					goto out_of_loop;
			} else {
				/* mark this device "in-use" and save info about it */
				this_item->the_file = dev_file;
				this_item->max_blocks = FILESIZE;
				this_item->bytes_per_block = BLOCKSIZE;
				this_item->in_use = 1;

				printk("opened file %s as id: %d lun: %d\n",
						this_item->file_name, targ, lun);
				target_count++;
			}
		}
	}

out_of_loop:

	return 0;
}

/*
 * close_filp_table: to close all the open file descriptors
 * INPUT: None
 * OUTPUT: None
 */
static void
close_filp_table(void)
{
	uint32_t targ, lun;
	struct target_map_item *this_item;

	if (!down_interruptible(&target_map_sem)) {
		for (targ = 0; targ < MAX_TARGETS; targ++) {
			for (lun = 0; lun < MAX_LUNS; lun++) {
				this_item = &target_map[targ][lun];
				if (this_item->the_file) {
					filp_close(this_item->the_file, NULL);
					this_item->the_file = NULL;
					this_item->in_use = 0;
				}
			}
		}
		up(&target_map_sem);
	}
}

#endif

/*
 * : allocates scatter-gather buffers for the received command
 * The assumption is that all front-ends use scatter-gather and so do
 * the back-ends ... this may change.
 * INPUT: Scsi_Request for which space has to be allocated, space needed
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
static int
get_space(Scsi_Request * req, int space /* in bytes */ )
{
	/* We assume that scatter gather is used universally */
	struct scatterlist *st_buffer;
	int buff_needed, i;
	int count;
    long pagesize = sysconf(_SC_PAGESIZE);

	/* we assume that all buffers are split by page size */

	/* get enough scatter gather entries */
	buff_needed = space / pagesize;
	if (space > (buff_needed * pagesize))
		buff_needed++;

	/*ramesh - added check for allocating memory */
	if (buff_needed == 0)
		buff_needed = 1;

	st_buffer = malloc(buff_needed * sizeof(struct scatterlist));

	if (!st_buffer) {
		TRACE_ERROR("get_space: no space for st_buffer\n");
		return -1;
	}
	memset(st_buffer, 0, buff_needed * sizeof(struct scatterlist));

	/* get necessary buffer space */
	for (i = 0, count = space; i < buff_needed; i++, count -= FAKED_PAGE_SIZE) {
		st_buffer[i].address = malloc(FAKED_PAGE_SIZE);

		if (!st_buffer[i].address) {
			TRACE_ERROR("get_space: no space for st_buffer[%d].address\n", i);
			return -1;
		}
		if (count > FAKED_PAGE_SIZE)
			st_buffer[i].length = FAKED_PAGE_SIZE;
		else
			st_buffer[i].length = count;


		TRACE(TRACE_VERBOSE, "get_space: st_buffer[%d] = %d", i, st_buffer[i].length);
	}

	req->sr_bufflen = space;
	req->sr_buffer = st_buffer;
	req->sr_sglist_len = buff_needed * sizeof(struct scatterlist);
	req->sr_use_sg = buff_needed;

	return 0;
}

/*
 * Returns size of space allocated for LUN list > 0 if all ok,
 * -1 on error after giving message
 */

static int
allocate_report_lun_space(Target_Scsi_Cmnd * cmnd)
{
	int i, luns, size;

	/* perform checks on Report LUNS - LATER */
	if (cmnd->req->sr_cmnd[2] != 0) {
		TRACE_ERROR("Select_Report in report_luns not zero\n");
	}

	/* set data direction */
	cmnd->req->sr_data_direction = SCSI_DATA_READ;

	/* get length */
	if (cmnd->target_id >= MAX_TARGETS) {
		TRACE_ERROR("target id %u >= MAX_TARGETS %u\n",
			    cmnd->target_id, MAX_TARGETS);
		return -1;
	}

	luns = 0;
    pthread_mutex_lock(&target_map_mutex);
    for (i = 0; i < MAX_LUNS; i++) {
        if (target_map[cmnd->target_id][i].in_use)
            luns++;
    }
    pthread_mutex_unlock(&target_map_mutex);

	if (luns == 0) {
		TRACE_ERROR("No luns in use for target id %u\n",
			   cmnd->target_id);
		return -1;
		}

	TRACE_ERROR("REPORT_LUNS: target id %u reporting %d luns\n",
                cmnd->target_id, luns);

	/* allocate space */
	size = luns * 8;
	if (get_space(cmnd->req, size + 8)) {
		TRACE_ERROR("get_space returned an error for %d\n", cmnd->id);
		return -1;
		}

	return size;
}

/*
 * get_allocation_length: This function looks at the received command
 * and calculates the size of the buffer to be allocated in order to
 * execute the command.
 * INPUT: pointer to command received
 * OUTPUT: buffer needing to be allocated or < 0 if there is an error
 */
static uint32_t __attribute__ ((no_instrument_function))
get_allocation_length(uint8_t *cmd)
{
	uint32_t err = 0;


	switch (cmd[0]) {
	case INQUIRY:
	case MODE_SENSE:
	case MODE_SELECT:
		{
			err = cmd[ALLOC_LEN_6];
			TRACE(TRACE_VERBOSE,
                  "get_allocation_length: INQUIRY/MODE SENSE/MODE SELECT length %d",
                  err);
			break;
		}

	case WRITE_10:
	case READ_10:
	case VERIFY:
		{
			err = (cmd[ALLOC_LEN_10] << BYTE) + cmd[ALLOC_LEN_10 + 1];
			err *= SCSI_BLOCKSIZE;
			TRACE(TRACE_VERBOSE, 
                  "get_allocation_length: READ_10/WRITE_10 length %d", err);
			break;
		}

		/* cdeng, August 24 2002, Report luns */
	case REPORT_LUNS:
		{
			/* Bjorn Thordarson, 10 May 2004 */
			/*****
			err = (cmd[6] << 24) + (cmd[7] << 16) + (cmd[8] << 8) + cmd[9];
			*****/
			err = 0;

			TRACE(TRACE_VERBOSE,
                  "get_allocation_length: REPORT_LUNS length %d - FIXME", err);
			break;
		}

	case READ_6:
	case WRITE_6:
		{
			err = cmd[4] * SCSI_BLOCKSIZE;
			if (err == 0)
				err = 256 * SCSI_BLOCKSIZE;
			TRACE(TRACE_VERBOSE,
                  "get_allocation_length: READ_6/WRITE_6 length %d", err);
			break;
		}
	default:
		{
			TRACE_ERROR("get_allocation_length: unknown command 0x%02x\n", cmd[0]);
			break;
		}
	}

	return err;
}

/*
 * get_inquiry_response: This function fills in the buffer to respond to
 * a received SCSI INQUIRY. This function is relevant when the emulator
 * is responding to commands itself and not transmitting it onto a SCSI
 * HBA attached to the system
 * INPUT: Scsi_Request ptr to the inquiry, length of expected response
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
static int
/* Bjorn Thordarson, 9-May-2004 */
/*****
get_inquiry_response(Scsi_Request * req, int len)
*****/
get_inquiry_response(Scsi_Request * req, int len, int type)
{
	uint8_t *inq;
	uint8_t *ptr, local_buffer[36];

	uint8_t *buffer = ((struct scatterlist *) req->sr_buffer)->address;

	/* SPC-2 says in section 7.3.2 on page 82:
	 *  "The standard INQUIRY data shall contain at least 36 bytes."
	 */
	if (len >= 36) {
		/* caller's buffer is big enough to hold all 36 bytes */
		ptr = buffer;
		memset(buffer, 0x00, len);
	} else {
		/* fill a local 36-byte buffer */
		ptr = local_buffer;
		memset(local_buffer, 0x00, 36);
	}
	/* Bjorn Thordarson, 9-May-2004 */
	/*****
	ptr[0] = TYPE_DISK;
	*****/
	ptr[0] = type;
	ptr[2] = 4;		/* Device complies to this standard - SPC-2 */
	ptr[3] = 2;		/* data in format specified in this standard */
	ptr[4] = 31;		/* n - 4 = 35 - 4 = 31 for full 36 byte data */
	ptr[6] = 0x80;		/* BQue = 1 basic task management supported */

	/* SPC-2 says in section 7.3.2 on page 86:
	 * "ASCII data fields shall contain only graphic codes (i.e., code
	 * values 20h through 7Eh). Left-aligned fields shall place any
	 * unused bytes at the end of the field (highest offset) and the
	 * unused bytes shall be filled with space characters (20h)."
	 */

	/* 8 byte ASCII Vendor Identification of the target - left aligned */
	memcpy(&ptr[8], "UNH-IOL ", 8);

	/* 16 byte ASCII Product Identification of the target - left aligned */
	memcpy(&ptr[16], "in-memory target", 16);

	/* 4 byte ASCII Product Revision Level of the target - left aligned */
	memcpy(&ptr[32], "1.2 ", 4);

	if (len < 36) {
		/* fill as much as possible of caller's too-small buffer */
		memcpy(buffer, ptr, len);
	}

	req->sr_result = DID_OK << 16;

	if (req->sr_allowed == 1) {
		inq = ((struct scatterlist *) req->sr_buffer)->address;
		inq[0] = 0x7f;
	}

	return 0;
}

/*
 * get_read_capacity_response: This function fills up the buffer to
 * respond to a received READ_CAPACITY. This function is relevant when
 * the emulator is responding to the commands.
 * INPUT: Scsi_Request pointer to the READ_CAPACITY
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
static int
/* Bjorn Thordarson, 9-May-2004 */
/*****
get_read_capacity_response(Scsi_Request * req)
*****/
get_read_capacity_response(Target_Scsi_Cmnd *cmnd)
{
	uint32_t blocksize = SCSI_BLOCKSIZE, nblocks;
	uint8_t *buffer;

/* RDR */
	nblocks = FILESIZE;

	buffer = ((struct scatterlist *)cmnd->req->sr_buffer)->address;

	/* Bjorn Thordarson, 9-May-2004 -- end -- */

	memset(buffer, 0x00, READ_CAP_LEN);

	/* last block on the disk is (nblocks-1) */
	buffer[0] = ((nblocks - 1) >> (BYTE * 3)) & 0xFF;
	buffer[1] = ((nblocks - 1) >> (BYTE * 2)) & 0xFF;
	buffer[2] = ((nblocks - 1) >> (BYTE)) & 0xFF;
	buffer[3] = (nblocks - 1) & 0xFF;

	buffer[4] = (blocksize >> (BYTE * 3)) & 0xFF;
	buffer[5] = (blocksize >> (BYTE * 2)) & 0xFF;
	buffer[6] = (blocksize >> BYTE) & 0xFF;
	buffer[7] = blocksize & 0xFF;

	/* Bjorn Thordarson, 9-May-2004 */
	/*****
	req->sr_result = DID_OK << 16;
	*****/
	cmnd->req->sr_result = DID_OK << 16;
	return 0;
}

/* RDR */

/* cdeng August 24 2002
 * get_mode_sense_response: This function fills up the buffer to
 * respond to a received MODE_SENSE. This function is relevant when
 * the emulator is responding to the commands.
 * INPUT: Scsi_Request pointer to the MODE_SENSE
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
static int
get_mode_sense_response(Scsi_Request * req, uint32_t len)
{
	uint8_t *buffer = ((struct scatterlist *) req->sr_buffer)->address;

	memset(buffer, 0x00, len);

	buffer[0] = 0x0b;			// no. of bytes that follow == 11

	buffer[3] = 0x08;			// block descriptor length

	buffer[10] = 0x02;			// density code and block length

	req->sr_result = DID_OK << 16;
	return 0;
}

/* cdeng August 24 2002
 * get_report_luns_response: This function fills up the buffer to
 * respond to a received REPORT_LUNS. This function is relevant when
 * the emulator is responding to the commands.
 * INPUT: Scsi_Request pointer to the REPORT_LUNS
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
static int
get_report_luns_response(Target_Scsi_Cmnd *cmnd, uint32_t len)
{
	int i;
	uint8_t *limit, *next_slot;

	/* Bjorn Thordarson, 9-May-2004 */
	/*****
	uint8_t *buffer = ((struct scatterlist *) req->sr_buffer)->address;
	*****/
	uint8_t *buffer = ((struct scatterlist *) cmnd->req->sr_buffer)->address;

	next_slot = buffer + 8;				/* first lun goes here */
	limit = next_slot + len;			/* address after end of entire list */
	memset(buffer, 0x00, len + 8);

	/* SAM-2, section 4.12.2 LUN 0 address
	 * "To address the LUN 0 of a SCSI device the peripheral device
	 * address method shall be used."
	 *
	 * What this means is that whenever the LUN is 0, the full 8 bytes of
	 * the LUN field in the iscsi PDU will also be 0. Because we have
	 * already zeroed out the entire buffer above,
	 * we don't need to do anything else here for lun 0.
	 */

	if (cmnd->target_id < MAX_TARGETS) 
    {
        pthread_mutex_lock(&target_map_mutex);
        for (i = 0; i < MAX_LUNS && next_slot < limit; i++) 
        {
            if (target_map[cmnd->target_id][i].in_use) {
                pack_lun(i, 0, next_slot);
                next_slot += 8;;
            }
        }
        pthread_mutex_unlock(&target_map_mutex);
    }

	/* lun list length */
	((uint32_t *)buffer)[0] = htonl(len);

# ifdef DEBUG_HANDLE_CMD
	/* dump out the complete REPORT LUNS response buffer */
	dump_buffer(buffer, len + 8);
# endif

	/* change status */
	cmnd->state = ST_DONE;
	cmnd->req->sr_result = DID_OK << 16;
	return 0;
}

/*
 * hand_to_front_end: the scsi command has been tackled by the mid-level
 * and is now ready to be handed off to the front-end either because it
 * is done or because data is needed to process this command.
 * This function is separated only because it makes
 * the main code a lot cleaner.
 * INPUT: Scsi_Cmnd that needs to be handed off
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
static int
hand_to_front_end(Target_Scsi_Cmnd * the_command)
{
	Scsi_Target_Device *curr_device;
# ifdef GENERICIO
	uint8_t *temp;
# endif

	/* get the device template corresponding to the device_id */
	for (curr_device = target_data.st_device_list; curr_device != NULL;
		 curr_device = curr_device->next) {
		if (curr_device->id == the_command->dev_id)
			break;
	}

	if (curr_device == NULL) {	/* huh - should not get here */
		/*
		 * That there is not a device with this id may mean
		 * one of two things - there has been a mistake or
		 * that the device is no longer there. For now, the
		 * former is more probable
		 */
		TRACE_ERROR("hand_to_front_end: no device with id %llu\n",
			   the_command->dev_id);
		return -1;
	}

	/*
	 * In the time that the command was processed, it could have
	 * been aborted/released. We need to check this. If so, then
	 * the command state is changed to ST_DEQUEUE and returned
	 */
	if (the_command->abort_code != CMND_OPEN) {
		the_command->state = ST_DEQUEUE;
		return 0;
	}

	if (the_command->state == ST_DONE) 
    {
        the_command->state = ST_HANDED;
        if (iscsi_xmit_response(the_command)) 
        {
            TRACE_ERROR("hand_to_front_end: error in xmit_response for %p "
                        "id %d\n", the_command, the_command->id);
            return -1;
        }
    } else if (the_command->state == ST_PENDING) {
        the_command->state = ST_XFERRED;
        if (iscsi_rdy_to_xfer(the_command)) {
            TRACE_ERROR("hand_to_front_end: error in rdy_to_xfer for %p "
                        "id %d\n", the_command, the_command->id);
            return -1;
        }
	} else {
		TRACE_ERROR("hand_to_front_end: command %p id: %d bad state %d\n",
                    the_command, the_command->id, the_command->state);
		return -1;
	}

	return 0;
}

/*
 * abort_notify: This function is used to notify to the front end driver
 * that a command has been successfully aborted
 * INPUT: Target_Scsi_Cmnd
 * OUTPUT: none
 */
static int
abort_notify(Target_Scsi_Message * msg)
{
	Target_Scsi_Cmnd *cmnd;
	Scsi_Target_Device *curr_device;

	if (msg && msg->value)
		cmnd = (Target_Scsi_Cmnd *) msg->value;
	else {
		TRACE_ERROR("abort_notify: null cmnd in the msg\n");
		return -1;
	}

	/* get the device template corresponding to the device_id */
	for (curr_device = target_data.st_device_list; curr_device != NULL;
		 curr_device = curr_device->next) {
		if (curr_device->id == cmnd->dev_id)
			break;
	}

	if (curr_device == NULL) {
		TRACE_ERROR("abort_notify: Could not find the device\n");
		return -1;
	}

    iscsi_task_mgt_fn_done(msg);

	return 0;
}

/*
 * aen_notify: This function is used to notify to the low-level driver
 * that task management function has been received and that they need to
 * contact all initiators logged in to them and let them know. NOTE: the
 * aen_notify will also call the device that sent the task management
 * function - it needs to inform other initiators (than the one that
 * originated the Task Management function) about the received Mgmt fn
 * INPUT: management function and lun
 * OUTPUT: None
 */
static void
aen_notify(int fn, uint64_t lun)
{
    UNUSED(fn);
    UNUSED(lun);
#if 0
	Scsi_Target_Device *dev;

	for (dev = target_data.st_device_list; dev != NULL; dev = dev->next) {
        iscsi_report_aen(fn, lun);
	}
#endif
}

/********************************************************************
 * THESE ARE FUNCTIONS WHICH ARE SPECIFIC TO MEMORY IO - I lump them*
 * 	together just to help me keep track of what is where	    *
 *******************************************************************/

/*
 * handle_cmd: This command is responsible for dealing with the received
 * command. I am slightly unclear about everything this function needs
 * to do. Right now, it receives new commands, and gets them to the
 * state where they are processing or pending.
 * INPUT: Target_Scsi_Cmnd to be handled
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
static int
handle_cmd(Target_Scsi_Cmnd * cmnd)
{
	int err = 0;
	int to_read;

	TRACE(TRACE_VERBOSE, "Entering MEMORYIO handle_cmd");

	switch (cmnd->req->sr_cmnd[0]) {
        case READ_CAPACITY:
		{
			TRACE(TRACE_VERBOSE, "READ_CAPACITY received");
			/* perform checks on READ_CAPACITY - LATER */

			/* set data direction */
			cmnd->req->sr_data_direction = SCSI_DATA_READ;

			/* allocate sg_list and get_free_pages */
			if (get_space(cmnd->req, READ_CAP_LEN)) 
            {
				TRACE_ERROR("handle_command: get_space returned an error for %d\n",
                            cmnd->id);
				err = -1;
				break;
			}

			/* get response */
			/* Bjorn Thordarson, 9-May-2004 */
			/*****
			if (get_read_capacity_response(cmnd->req)) {
			*****/
			if (get_read_capacity_response(cmnd)) {
                TRACE_ERROR
					("handle_command: get_read_capacity_response returned an error for %d\n",
					 cmnd->id);
				err = -1;
				break;
			}
            
			/* change status */
			cmnd->state = ST_DONE;
			cmnd->req->sr_result = DID_OK << 16;

			err = 0;
			break;
		}
        
        case INQUIRY:
        {
			TRACE(TRACE_VERBOSE, "INQUIRY received");
			/* perform checks on INQUIRY - LATER */

			/* set data direction */
			cmnd->req->sr_data_direction = SCSI_DATA_READ;
            
			/* get length */
			to_read = get_allocation_length(cmnd->req->sr_cmnd);
            
			if (to_read < 0) {
                TRACE_ERROR
					("handle_command: get_allocation length returned an error for %d\n",
					 cmnd->id);
				err = -1;
				break;
			}

			/* allocate space */
			if (get_space(cmnd->req, to_read)) {
				TRACE_ERROR("handle_command: get_space returned an error for %d\n",
					   cmnd->id);
				err = -1;
				break;
			}
            
			/* get response */
			/* Bjorn Thordarson, 9-May-2004 */
			/*****
                  if (get_inquiry_response(cmnd->req, to_read)) {
			*****/
			if (get_inquiry_response(cmnd->req, to_read, TYPE_DISK)) {
                TRACE_ERROR
					("handle_command: get_inquiry_response returned an error for %d\n",
					 cmnd->id);
				err = -1;
				break;
			}
            
			/* change status */
			cmnd->state = ST_DONE;
			cmnd->req->sr_result = DID_OK << 16;
            
			err = 0;
			break;
		}

        case TEST_UNIT_READY:
		{
			TRACE(TRACE_VERBOSE, "TEST UNIT READY received");

			/* perform checks on TEST UNIT READY */
			cmnd->req->sr_data_direction = SCSI_DATA_NONE;
			cmnd->req->sr_use_sg = 0;
			cmnd->req->sr_bufflen = 0;
			cmnd->req->sr_result = DID_OK << 16;
			cmnd->state = ST_DONE;
			err = 0;
			break;
		}
        
        case REPORT_LUNS:
		{
			if ((to_read = allocate_report_lun_space(cmnd)) < 0) {
				err = -1;
				break;
			}
            
			/* get response */
			if ((err = get_report_luns_response(cmnd, to_read))) {
                TRACE_ERROR
					("handle_command: get_report_luns_response returned an error for %d\n",
					 cmnd->id);
			}
			break;
		}
        
        case MODE_SENSE:
		{
			TRACE(TRACE_VERBOSE, "MODE_SENSE received");
			/* perform checks on MODE_SENSE - LATER */

			/* set data direction */
			cmnd->req->sr_data_direction = SCSI_DATA_READ;
            
			/* get length */
			to_read = get_allocation_length(cmnd->req->sr_cmnd);
            
			if (to_read < 0) {
                TRACE_ERROR
					("handle_command: get_allocation length returned an error for %d\n",
					 cmnd->id);
				err = -1;
				break;
			}
            
			/* allocate space */
			if (get_space(cmnd->req, to_read)) {
				TRACE_ERROR("handle_command: get_space returned an error for %d\n",
                            cmnd->id);
				err = -1;
				break;
			}
            
			/* get response */
			if (get_mode_sense_response(cmnd->req, to_read)) {
                TRACE_ERROR
					("handle_command: get_mode_sense_response returned an error for %d\n",
					 cmnd->id);
				err = -1;
				break;
			}
            
			/* change status */
			cmnd->state = ST_DONE;
			cmnd->req->sr_result = DID_OK << 16;
			err = 0;
			break;
		}
        
        case VERIFY:
		{
			TRACE(TRACE_VERBOSE, "VERIFY received");
			/* perform checks on TEST UNIT READY */
			cmnd->req->sr_data_direction = SCSI_DATA_NONE;
			cmnd->req->sr_use_sg = 0;
			cmnd->req->sr_bufflen = 0;
			cmnd->state = ST_DONE;
			cmnd->req->sr_result = DID_OK << 16;
			err = 0;
			break;
		}

        case READ_6:
        case READ_10:
		{
			if (cmnd->req->sr_cmnd[0] == READ_6)
				TRACE(TRACE_VERBOSE, "READ_6 received");
			else
				TRACE(TRACE_VERBOSE, "READ_10 received");
			/* perform checks for READ_10 */

			/* set data direction */
			cmnd->req->sr_data_direction = SCSI_DATA_READ;
			/* get length */
			to_read = get_allocation_length(cmnd->req->sr_cmnd);
            
			if (to_read < 0) {
                TRACE_ERROR
					("MEMORYIO handle_cmd: get_allocation_length returned (%d) an error for %d\n",
					 to_read, cmnd->id);
				err = -1;
				break;
			}
            
			if (get_space(cmnd->req, to_read)) {
				TRACE_ERROR("MEMORYIO handle_cmd: get_space returned an error for %d\n",
					   cmnd->id);
				err = -1;
				break;
			}
            
			/* this data can just be returned from memory */
			/* change_status */
			cmnd->req->sr_result = DID_OK << 16;
			cmnd->state = ST_DONE;
			err = 0;
			break;
		}
        
        case WRITE_6:
        case WRITE_10:
		{
			if (cmnd->req->sr_cmnd[0] == WRITE_6)
				TRACE(TRACE_VERBOSE, "WRITE_6 received");
			else
				TRACE(TRACE_VERBOSE, "WRITE_10 received");

			if (cmnd->state == ST_NEW_CMND) {
				/* perform checks on the received WRITE_10 */

				/* set data direction for the WRITE_10 */
				cmnd->req->sr_data_direction = SCSI_DATA_WRITE;
				/* get length */
				to_read = get_allocation_length(cmnd->req->sr_cmnd);

				if (to_read < 0) {
					TRACE_ERROR
						("MEMORYIO handle_cmd: get_allocation_length returned (%d) an error for %d\n",
						 to_read, cmnd->id);
					err = -1;
					break;
				}

				/* get space */
				if (get_space(cmnd->req, to_read)) {
					TRACE_ERROR("MEMORYIO handle_cmd: get_space returned error for %d\n",
                                cmnd->id);
					err = -1;
					break;
				}
				cmnd->state = ST_PENDING;
			} else if (cmnd->state == ST_TO_PROCESS) {
				/*
				 * in memory mode of processing we do not
				 * care about the data received at all
				 */
				cmnd->state = ST_DONE;
				cmnd->req->sr_result = DID_OK << 16;
			}
			err = 0;
			break;
		}

        default:
		{
			TRACE_ERROR("MEMORYIO handle_cmd: unknown command 0x%02x\n",
                   cmnd->req->sr_cmnd[0]);
			break;
		}
	}
	return err;
}



#if 0
/* dispatch_scsi_info is the central dispatcher
 * It is the interface between the proc-fs and the SCSI subsystem code
 */
static int
proc_scsi_target_read(char *buffer, char **start, off_t offset, int length,
					  int *eof, void *data)
{
	Scsi_Target_Device *tpnt = data;
	int n = 0;

	/*****
	printk("Entering proc_scsi_target_read\n");
	printk("offset: %d, length %d, tpntid: %d\n", (int) offset, length, 0);
	*****/

	if (tpnt->template->proc_info == NULL) {
/*		n = generic_proc_info(buffer, start, offset, length,
				      hpnt->hostt->info, hpnt); */
	} else {
		n = (tpnt->template->proc_info(buffer, start, offset, length, 0, 0));
	}
	*eof = (n < length);
	return n;
}
#endif

#define PROC_BLOCK_SIZE (3*1024)	/* 4K page size, but our output routines
									   * use some slack for overruns
									 */

#if 0
static int
proc_scsi_target_write(struct file *file, const char *buf,
					   unsigned long count, void *data)
{
	Scsi_Target_Device *tpnt = data;
	ssize_t ret = 0;
	char *page;
	char *start;

	if (count > PROC_BLOCK_SIZE)
		return -EOVERFLOW;

	if (!(page = (char *) __get_free_page(GFP_KERNEL)))
		return -ENOMEM;
	copy_from_user(page, buf, count);

	if (tpnt->template->proc_info == NULL)
		ret = -ENOSYS;
	else
		ret = tpnt->template->proc_info(page, &start, 0, count, tpnt->id, 1);
	free_page((ulong) page);
	return ret;
}
#endif


/* Define the functions that handle the proc interface */
#ifdef CONFIG_PROC_FS

#ifndef MEMORYIO
static int __attribute__ ((no_instrument_function))
print_device_info(char *buffer, uint32_t targ, uint32_t lun,
				  struct target_map_item *this_item)
{
	int len = 0;

#if defined(DISKIO)
	Scsi_Device *the_device;

	if ((the_device = this_item->the_device)) {
		len  = sprintf(buffer, "iscsi target %u: scsi device type %s, host %d, "
						"channel %d, targetid %d, lun %d\n", targ,
						printable_dev_type(the_device->type),
						the_device->host->host_no, the_device->channel,
						the_device->id, the_device->lun);
	} else {
		len = sprintf(buffer, "iscsi target %u: no device!\n", targ);
	}
#endif

#if defined(FILEIO) || defined(GENERICIO)
	if (this_item->the_file) {
		len = sprintf(buffer, "iscsi target %u lun %u: file %s, %u blocks, "
						"%u bytes/block\n",
						targ, lun, this_item->file_name, this_item->max_blocks,
						this_item->bytes_per_block);
	} else {
		len = sprintf(buffer, "iscsi target %u lun %u: no file!\n", targ, lun);
	}
#endif

	return len;
}
#endif

static int
scsi_target_proc_info(char *buffer, char **start, off_t offset, int length)
{
	int len = 0;
#ifndef MEMORYIO
	struct target_map_item *this_item;
#endif

#ifdef MEMORYIO
	len = sprintf(buffer, "unh_scsi_target using MEMORYIO\n");
#endif
#ifdef DISKIO
#ifdef TRUST_CDB
	len = sprintf(buffer, "unh_scsi_target using DISKIO with TRUST_CDB\n");
#else
	len = sprintf(buffer, "unh_scsi_target using DISKIO without TRUST_CDB\n");
#endif
#endif
#ifdef FILEIO
	len = sprintf(buffer, "unh_scsi_target using FILEIO\n");
#endif
#ifdef GENERICIO
	len = sprintf(buffer, "unh_scsi_target using GENERICIO\n");
#endif

	len += sprintf(buffer + len,"%d target devices currently configured\n",
					target_count);
#if defined(MEMORYIO)
	len += sprintf(buffer + len, "%d configured iscsi target numbers: 0..%d\n",
					MAX_TARGETS, MAX_TARGETS-1);
	len += sprintf(buffer + len, "each iscsi target has %d configured luns: "
					"0..%d\n", MAX_LUNS, MAX_LUNS-1);
#else
	if (!down_interruptible(&target_map_sem)) {

#if defined(DISKIO) && defined(TRUST_CDB)
		list_for_each_entry(this_item, &target_map_list, link) {
			if (this_item->in_use) {
				len += print_device_info(buffer + len, this_item->target_id, 0,
										 this_item);
			}
		}
#else
		uint32_t targ, lun;

		for (targ = 0; targ < MAX_TARGETS; targ++) {
			for (lun = 0; lun < MAX_LUNS; lun++) {
				this_item = &target_map[targ][lun];
				if (this_item->in_use) {
					len += print_device_info(buffer+len, targ, lun, this_item);
				}
			}
		}
		len += sprintf(buffer + len, "%d configurable iscsi target numbers: "
						"0..%d\n", MAX_TARGETS, MAX_TARGETS-1);
		len += sprintf(buffer + len, "each iscsi target has %d configurable "
						"luns: 0..%d\n", MAX_LUNS, MAX_LUNS-1);
#endif

		up(&target_map_sem);
	}
#endif
	return len;
}


/* RDR */
#if defined(FILEIO) || defined(GENERICIO)

/* Open a new device */
static int
open_device(char *device, struct file **dev_file)
{
        mm_segment_t old_fs;
        char *tmp;
        int error;

        printk("open device %s\n", device);
        old_fs = get_fs();
        set_fs(get_ds());
        tmp = getname(device);
        set_fs(old_fs);
        error = PTR_ERR(tmp);

        if (IS_ERR(tmp)) {
                printk("open_device: getname error %d\n", error);
                return -1;
        }

/* open a scsi_generic device */
# ifdef GENERICIO
        *dev_file = filp_open(tmp, O_RDWR | O_NONBLOCK | O_LARGEFILE, 0600);
# endif

# ifdef FILEIO
        *dev_file = filp_open(tmp, O_RDWR | O_NONBLOCK | O_CREAT | O_LARGEFILE, 0600);
# endif

        putname(tmp);
        if (IS_ERR(*dev_file)) {
                error = PTR_ERR(*dev_file);
                printk("open_device: filp_open error %d\n", error);
                return -1;
        }

        return 0;
}

/*
 * add this file to the matrix of open devices
 * Returns 0 if all ok, -1 on any error after giving message
 */
static int
add_file(uint32_t target_id, uint32_t lun, char *file_name, uint32_t max_blocks,
		 uint32_t block_size)
{
	int retval = -1;
	struct file *dev_file;
	struct target_map_item *this_item;

	if (target_id >= MAX_TARGETS) {
		printk("targetid=%u not in range [0..%d]\n", target_id, MAX_TARGETS-1);
	} else if (lun >= MAX_LUNS) {
		printk("lun=%u not in range [0..%d]\n", lun, MAX_LUNS-1);
	} else if (strlen(file_name) >= MAX_FILE_NAME) {
		printk("filename %s longer than %d characters\n",
				file_name, MAX_FILE_NAME-1);
	} else if (block_size < 512 || block_size > 65536) {
		printk("blocksize %u not in range [512..65536]\n", block_size);
	} else if (!down_interruptible(&target_map_sem)) {
		this_item = &target_map[target_id][lun];
		if (this_item->in_use) {
			printk("targetid %u lun %u already in use with filename %s\n",
					target_id, lun, this_item->file_name);
		} else if (open_device(file_name, &dev_file)) {
		   printk("add_file: trouble in open_device(%s)\n", file_name);
		} else {
			this_item->the_file = dev_file;
			this_item->max_blocks = max_blocks;
			this_item->bytes_per_block = block_size;
			strcpy(this_item->file_name, file_name);
			this_item->in_use = 1;

			printk("added file %s, targetid %u, lun %u\n",
					file_name, target_id, lun);
			retval = 0;
		}
		up(&target_map_sem);
	}
	return retval;
}

/*
 * delete this file from the matrix of open devices
 * Returns 0 if all ok, -1 on any error after giving message
 */
static int
del_file(uint32_t target_id, uint32_t lun)
{
	int retval = -1;
	struct target_map_item *this_item;

	if (target_id >= MAX_TARGETS) {
		printk("targetid %u not in range [0..%d]\n", target_id, MAX_TARGETS-1);
	} else if (lun >= MAX_LUNS) {
		printk("lun %u not in range [0..%d]\n", lun, MAX_LUNS-1);
	} else if (!down_interruptible(&target_map_sem)) {
		this_item = &target_map[target_id][lun];
		if (!this_item->in_use) {
			printk("targetid %u lun %u not in use\n", target_id, lun);
		} else {
			if (this_item->the_file) {
				filp_close(this_item->the_file, NULL);
				printk("closed file %s, targetid %u, lun %u\n",
						this_item->file_name, target_id, lun);
			}
			this_item->the_file = NULL;
			this_item->in_use = 0;
			retval = 0;
		}
		up(&target_map_sem);
	}
	return retval;
}


static int
proc_scsi_target_gen_write(struct file *file, const char *buffer,
						   unsigned long length, void *data)
{
	char buf[512];
	char file_name[64];
	uint32_t target_id, lun, max_blocks, block_size;

	if (length > 511)
			length = 511;
	copy_from_user(buf, buffer, length);
	buf[length] = 0;

	if (sscanf(buf, "addfile targetid=%u lun=%u filename=%63s "
			   "blocksinfile=%u blocksize=%u", &target_id, &lun,
			   file_name, &max_blocks, &block_size) == 5) {
		printk("addfile targetid=%u lun=%u filename=%s blocksinfile=%u "
			   "blocksize=%u\n", target_id, lun, file_name, max_blocks,
			   block_size);
		add_file(target_id, lun, file_name, max_blocks, block_size);
	} else if (sscanf(buf, "delfile targetid=%u lun=%u",
					  &target_id, &lun) == 2) {
		printk("delfile targetid=%u lun=%u\n", target_id, lun);
		del_file(target_id, lun);
	} else {
		printk("scsi_target: illegal input \"%s\"\n", buf);
	}

	return length;
}

#else

static int
proc_scsi_target_gen_write(struct file *file, const char *buf,
						   unsigned long length, void *data)
{
	printk("Function not implemented\n");
	return 0;
}

#endif

static void
build_proc_target_dir_entries(Scsi_Target_Device * the_device)
{
	struct proc_dir_entry *p;
	char name[10];

	/* create the proc directory entry for the device */
	the_device->template->proc_dir =
		proc_mkdir(the_device->template->name, proc_scsi_target);

	/* create the proc file entry for the device */
	sprintf(name, "%d", (int) the_device->id);
	p = create_proc_read_entry(name, S_IFREG | S_IRUGO | S_IWUSR,
							   the_device->template->proc_dir,
							   proc_scsi_target_read, (void *) the_device);
	if (!p)
		panic
			("Not enough memory to register SCSI target in /proc/scsi_target\n");
	p->write_proc = proc_scsi_target_write;

}

#endif

#ifdef MODULE
MODULE_AUTHOR("Bob Russell");
MODULE_DESCRIPTION("UNH");
MODULE_SUPPORTED_DEVICE("sd");
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
#endif

