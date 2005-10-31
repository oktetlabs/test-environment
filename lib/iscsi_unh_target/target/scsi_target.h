/*	target/scsi_target.h */

/*
	Copyright (C) 2001-2004 InterOperability Lab (IOL)
				University of New Hampshire (UNH)
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

	The name of IOL and/or UNH may not be used to endorse or promote
	products derived from this software without specific prior
	written permission.
*/

#ifndef _SCSI_TARGET_H
#define _SCSI_TARGET_H


#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdint.h>

#include "../common/linux-scsi.h"
#include "../common/scsi_request.h"
#include "../common/list.h"

/****/

typedef struct scsi_host_template Scsi_Host_Template;
typedef struct scsi_device Scsi_Device;
typedef struct scsi_cmnd Scsi_Cmnd;
typedef struct scsi_request Scsi_Request;
typedef struct scsi_pointer Scsi_Pointer;
/****/

#define	SCSI_BLOCKSIZE	512

#define	TWOBYTE		16
#define	BYTE		8
#define	FILESIZE	(4096 * 1024)	/* file size in blocks */
#define IO_SIGS		(sigmask(SIGKILL)|sigmask(SIGINT)|sigmask(SIGTERM)|sigmask(SIGIO))
#define	MAX_SENSE_DATA	16
#define SHUTDOWN_SIGS	(sigmask(SIGKILL)|sigmask(SIGINT)|sigmask(SIGTERM))
#define	TE_TRY		1
#define	TE_TIMEOUT	10*HZ
#define R_BIT		0x40
#define W_BIT		0x20

#define	MAX_LUNS	1

#define	MAX_TARGETS	16


/* undefined scsi opcode */
#define	REPORT_LUNS	0xa0

/* various possible states of commands - state in Target_Scsi_Cmnd */
#define	ST_NEW_CMND	1 /* command just arrived */
#define	ST_PROCESSING	2 /* sent off to process */
#define	ST_PENDING	3 /* waiting to get data */
#define	ST_TO_PROCESS	4 /* command ready to process */
#define	ST_DONE		5 /* response to command received */
#define	ST_DEQUEUE	6 /* front end done with command */
#define	ST_XFERRED	7 /* notified front end of buffers */
#define	ST_HANDED	8 /* command given to front end */
#define	ST_PROCESSED	9 /* SIGIO has been received */


/* values for abort code */
#define	CMND_OPEN	0 /* Normal state of command */
#define	CMND_ABORTED	1 /* ABORT received for this command */
#define	CMND_RELEASED	2 /* No response needed for this cmnd */


/* Values for management functions from
 * RFC 3720 Section 10.5.1 Function (field in TMF Request)
 *
 * N.B. These definitions are duplicated in common/iscsi_common.h
 * because they are needed in iSCSI but that common file cannot be
 * included in this file (scsi_target.h)
 */
#define	TMF_ABORT_TASK			1	/*1000*/
#define	TMF_ABORT_TASK_SET		2	/*1001*/
#define	TMF_CLEAR_ACA			3	/*1002*/
#define	TMF_CLEAR_TASK_SET		4	/*1003*/
#define	TMF_LUN_RESET			5	/*1004*/
#define	TMF_TARGET_WARM_RESET		6	/*1005*/
#define TMF_TARGET_COLD_RESET		7
#define TMF_TASK_REASSIGN		8


/* command response lengths */
#define	READ_CAP_LEN	8
#define	ALLOC_LEN_6	4
#define	ALLOC_LEN_10	7
#define	LBA_POSN_10	2

/* variable definitions */

struct GTE;
struct SC;
struct SM;
struct STD;
struct STT;


typedef struct SM {
	/* next: pointer to the next message */
	struct SM	*next;
	/* prev: pointer to the previous message */
	struct SM	*prev;
	/* message: Task Management function received */
	int		message;
	/* device: device that received the Task Management function */
	struct Scsi_Target_Device	*device;
	/* value: value relevant to the function, if any */
	void		*value;
} Target_Scsi_Message;


typedef struct SC {
	/*
	 * This will take different values depending on what the present
	 * condition of the command is
	 */
	int		state;
	/* abort_code: is this command aborted, released, or open */
	int		abort_code;
	/* id: id used to refer to the command */
	int		id;
	/* dev_id: device id - front end id that received the command */
	uint64_t		dev_id;
	/* device: struct corresponding to device - may not be needed */
	struct Scsi_Target_Device	*device;
	/* target_id: scsi id that received this command */
	uint32_t		target_id;
	 /* lun: which lun was supposed to get this command */
	uint32_t		lun;
	/* cmd: array for command until req is allocated */
	unsigned char	cmd[MAX_COMMAND_SIZE];
	/* len: length of the command received */
	int		len;
	/*
	 * queue of Scsi commands
	 */
	/* link: thread to link this command onto the cmd_queue */
	struct list_head link;
	/* req: this is the SCSI request for the Scsi Command */
	Scsi_Request	*req;
#ifdef GENERICIO
	/* sg: this is what Scsi generic will use to do I/O */
	sg_io_hdr_t	*sg;
	/* fd: "file descriptor" required for this command */
	struct file	*fd;
	/* blk_size: number of bytes in one block */
	uint32_t		blk_size;
#endif
#ifdef FILEIO
	/* fd: "file descriptor" required for this command */
	struct file	*fd;
#endif

	/*
	 ramesh@global.com
	 added data length and flags fields. the values are
	 get assigned from the pdu that is received
	*/
	int datalen;
	int flags;

} Target_Scsi_Cmnd;


/*
 * Scsi_Target_Template: defines what functions the added front end will
 * have to provide in order to work with the Target mid-level. The
 * comments should make each field clearer. MUST HAVEs define functions that
 * I expect to be there in order to work. OPTIONAL says you have a choice.
 * Also, pay attention to the fact that a command is BLOCKING or NON-BLOCKING
 * Although I do not know the effect of this requirement on the code - it
 * most certainly is required for efficient functioning.
 */
typedef struct STT
{
	/* private: */

	/*
	 * Do not bother to touch any one of these -- the only thing
	 * that should have access to these is the mid-level - If you
	 * change stuff here, I assume you know what you are doing :-)
	 */

	struct  STT *next;	/* next one in the list */
	int	device_usage;	/* how many are using this Template */

	/* public: */
    /*
     * The pointer to the /proc/scsi_target directory entry
     */
    struct proc_dir_entry *proc_dir;

    /* proc-fs info function.
     * Can be used to export driver statistics and other infos to the world
     * outside the kernel ie. userspace and it also provides an interface
     * to feed the driver with information. Check eata_dma_proc.c for reference
     */
    int (*proc_info)(char *, char **, off_t, int, int, int);

	/*
	 * name of the template. Must be unique so as to help identify
	 * the template. I have restricted the name length to 16 chars
	 * which should be sufficient for most purposes. MUST HAVE
	 */
	const char name[TWOBYTE];
	/*
	 * detect function: This function should detect the devices that
	 * are present in the system. The function should return a value
	 * >= 0 to signify the number of front end devices within the
	 * system. A negative value should be returned whenever there is
	 * an error. MUST HAVE
	 */
	int (* detect) (struct STT*);
	/*
	 * release function: This function should free up the resources
	 * allocated to the device defined by the STD. The function should
	 * return 0 to indicate successful release or a negative value if
	 * there are some issues with the release. OPTIONAL
	 */
	int (* release)(struct STD*);
	/*
	 * xmit_response: This function is equivalent to the SCSI
	 * queuecommand. The front-end should transmit the response
	 * buffer and the status in the Scsi_Request struct.
	 * The expectation is that this executing this command is
	 * NON-BLOCKING.
	 *
	 * After the response is actually transmitted, the front-end
	 * should call the front_end_done function in the mid-level
	 * which will allow the mid-level to free up the command
	 * Return 0 for success and < 0 for trouble
	 * MUST HAVE
	 */
	int (* xmit_response)(struct SC*);
	/*
	 * rdy_to_xfer: This function informs the driver that data
	 * buffer corresponding to the said command have now been
	 * allocated and it is okay to receive data for this command.
	 * This function is necessary because a SCSI target does not
	 * have any control over the commands it receives. Most lower
	 * level protocols have a corresponding function which informs
	 * the initiator that buffers have been allocated e.g., XFER_
	 * RDY in Fibre Channel. After the data is actually received
	 * the low-level driver needs to call rx_data provided by the
	 * mid-level in order to continue processing this command.
	 * Return 0 for success and < 0 for trouble
	 * This command is expected to be NON-BLOCKING.
	 * MUST HAVE.
	 */
	int (* rdy_to_xfer)(struct SC*);
	/*
	 * task_mgmt_fn_done: This function informs the driver that a
	 * received task management function has been completed. This
	 * function is necessary because low-level protocols have some
	 * means of informing the initiator about the completion of a
	 * Task Management function. This function being called will
	 * signify that a Task Management function is completed as far
	 * as the mid-level is concerned. Any information that must be
	 * stored about the command is the responsibility of the low-
	 * level driver. The field SC is relevant only for ABORT tasks
	 * No return value expected.
	 * This function is expected to be NON-BLOCKING
	 * MUST HAVE if the front-end supports ABORTs
	 */
	void (* task_mgmt_fn_done)(struct SM*);
	/*
	 * report_aen: This function is used for Asynchronous Event
	 * Notification. Since the Mid-Level does not have a mechanism
	 * to know about initiators logged in with low-level driver, it
	 * is the responsibility of the driver to notify any/all
	 * initiators about the Asynchronous Event reported.
	 * 0 for success, and < 0 for trouble
	 * This command is expected to be NON-BLOCKING
	 * MUST HAVE if low-level protocol supports AEN
	 */
	void (* report_aen)(int /* MGMT FN */, uint64_t /* LUN */);
} Scsi_Target_Template;


/*
 * Scsi_Target_Device: This is an internal struct that is created to define
 * a device list in the struct GTE. Thus one STT could potentially
 * correspond to multiple devices. The only thing that the front end should
 * actually care about is the device id. This is defined by SAM as a 64 bit
 * entity. I should change this one pretty soon.
 */
typedef struct Scsi_Target_Device
{
	uint64_t		id;		/* device id */
	struct Scsi_Target_Device	*next; 		/* next one in the list */

	/* dev_specific: my idea behind keeping this field was that this
	 * should help the front end target driver store a pointer to a
	 * struct that is a super-set of the Scsi_Target_Device where it
	 * stores system specific values. This should help the front-end
	 * locate the front-end easily instead of doing a search every
	 * time. (kind of like hostdata in Scsi_Host). Allocation and
	 * deallocation of memory for it is the responsibility of the
	 * front-end. THIS IS THE ONLY FIELD THAT SHOULD BE MODIFIED BY
	 * THE FRONT-END DRIVER
	 */
	void		*dev_specific;

} Scsi_Target_Device;


/*
 * Target_Emulator: This is the "global" target struct. All information
 * within the system flows from this target struct. It also serves as a
 * repository for all that is global to the target emulator system
 */
typedef struct GTE
{
	/*
	 * command_id: This is a counter to decide what command_id gets
	 * assigned to a command that gets queued. 0 is not a valid command.
	 * Also, command_ids can wrap around. I also assume that if command
	 * _ids wrap-around, then all commands with that particular command
	 * _id have already been dealt with. There is a miniscule (?) chance
	 * that this may not be the case - but I do not deal with it
	 */
	int			command_id;
	/*
	 * signal_id: pointer to the signal_process_thread. Will be used
	 * to kill the thread when necessary
	 */
	struct task_struct	*signal_id;
	/*
	 * thread_id: this task struct will store the pointer to the SCSI
	 * Mid-level thread - useful to kill the thread when necessary
	 */
	/*
	 * st_device_list: pointer to the list of devices. I have not added
	 * any semaphores around this list simply because I do not expect
	 * multiple devices to register simultaneously (Am I correct ?)
	 */
	Scsi_Target_Device	*st_device_list;
	/*
	 * st_target_template: pointer to the target template. Each template
	 * in this list can have multiple devices
	 */
	Scsi_Target_Template	*st_target_template;
	/* this may change */
	/*
	 * cmd_queue_lock: this spinlock must be acquired whenever the
	 * cmd_queue is accessed.  The spinlock is essential because
	 * received CDBs can get queued up in the context of an
	 * interrupt handler so we have to a spinlock, not a semaphore.
	 */
	pthread_mutex_t		cmd_queue_lock;
	/*
	 * cmd_queue: doubly linked circular queue of commands
	 */
	struct list_head	cmd_queue;
	/*
	 * msgq_start: pointer to the start of the message queue
	 */
	Target_Scsi_Message	*msgq_start;
	/*
	 * msgq_end: pointer to the end of the message queue
	 */
	Target_Scsi_Message	*msgq_end;
	/*
	 * msg_lock: spinlock for the message
	 */
	pthread_mutex_t		msg_lock;
} Target_Emulator;

/* number of devices target currently has access to */
extern int target_count;

/* function prototypes */

/* these are entry points provided to the low-level driver */
int	register_target_template	(Scsi_Target_Template*);
int	deregister_target_template	(Scsi_Target_Template*);
int	scsi_target_done		(Target_Scsi_Cmnd*);
int	deregister_target_front_end	(Scsi_Target_Device*);
int	scsi_rx_data			(Target_Scsi_Cmnd*);
int	scsi_release			(Target_Scsi_Cmnd*);


Scsi_Target_Device *make_target_front_end(void);

Target_Scsi_Cmnd*	rx_cmnd		(Scsi_Target_Device*, uint64_t,
					uint64_t, unsigned char*, int, int, int,
					Target_Scsi_Cmnd**);
Target_Scsi_Message*	rx_task_mgmt_fn	(Scsi_Target_Device*,int,void*);

/** Default size of an iSCSI backing store in 512-blocks */
#define DEFAULT_STORAGE_SIZE   16384

extern int scsi_target_init(void);

#endif
