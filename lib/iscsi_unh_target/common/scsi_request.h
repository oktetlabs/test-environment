#ifndef _SCSI_SCSI_REQUEST_H
#define _SCSI_SCSI_REQUEST_H

#include <linux-scsi.h>

struct request;
struct scsi_cmnd;
struct scsi_device;
struct Scsi_Host;

#define MAX_COMMAND_SIZE 16


/*
 * This is essentially a slimmed down version of Scsi_Cmnd.  The point of
 * having this is that requests that are injected into the queue as result
 * of things like ioctls and character devices shouldn't be using a
 * Scsi_Cmnd until such a time that the command is actually at the head
 * of the queue and being sent to the driver.
 */
struct scsi_request {
	int     sr_magic;
	int     sr_result;	/* Status code from lower level driver */
	unsigned char sr_sense_buffer[SCSI_SENSE_BUFFERSIZE];		/* obtained by REQUEST SENSE
						 * when CHECK CONDITION is
						 * received on original command 
						 * (auto-sense) */
    uint32_t sr_sense_length;

	struct Scsi_Host *sr_host;
	struct scsi_device *sr_device;
	struct scsi_cmnd *sr_command;
	struct request *sr_request;	/* A copy of the command we are
				   working on */
	unsigned sr_bufflen;	/* Size of data buffer */
	void *sr_buffer;		/* Data buffer */
	int sr_allowed;
	enum scsi_data_direction sr_data_direction;
	unsigned char sr_cmd_len;
	unsigned char sr_cmnd[MAX_COMMAND_SIZE];
	void (*sr_done) (struct scsi_cmnd *);	/* Mid-level done function */
	int sr_timeout_per_command;
	unsigned short sr_use_sg;	/* Number of pieces of scatter-gather */
	unsigned short sr_sglist_len;	/* size of malloc'd scatter-gather list */
	unsigned sr_underflow;	/* Return error if less than
				   this amount is transferred */
 	void *upper_private_data;	/* reserved for owner (usually upper
 					   level driver) of this request */
};

struct scsi_mode_data {
	uint32_t    length;
	uint16_t    block_descriptor_length;
	uint8_t     medium_type;
	uint8_t     device_specific;
	uint8_t     header_length;
	uint8_t     longlba:1;
};

#endif /* _SCSI_SCSI_REQUEST_H */
