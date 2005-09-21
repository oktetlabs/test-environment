/*	target/iscsi_target.c

	vi: set autoindent tabstop=4 shiftwidth=4 :

 	This is the iscsi target front-end which interfaces with the STML.
	The front-end has been written to the specifications of Draft 20 of the
	iSCSI spec.

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

	The name IOL and/or UNH may not be used to endorse or promote products
	derived from this software without specific prior written permission.
*/

#if !defined(MANGLE_INQUIRY_DATA)
#define MANGLE_INQUIRY_DATA
#endif

#include <stdlib.h>
#include <stdint.h>
#include "iscsi_target.h"
#include "target_error_rec.h"
#include <net/sock.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <net/if_inet6.h>
#include <net/ipv6.h>
#include <net/addrconf.h>

/* added by RDR when rcu_read_lock(), rcu_read_ulock() came in with 2.6.10
 * These macros define it the way it was in 2.4 and in earlier versions of 2.6
 */
#ifndef rcu_read_lock
#define rcu_read_lock()	read_lock(&inetdev_lock)
#endif
#ifndef rcu_read_unlock
#define rcu_read_unlock()	read_unlock(&inetdev_lock)
#endif

static void clean_bad_stuff(void);
static int iscsi_release_connection(struct iscsi_conn *conn);

static int handle_login(struct iscsi_conn *conn, uint8_t *buffer);
static int handle_logout(struct iscsi_conn *conn,
						 struct iscsi_session *session,
						 uint8_t *buffer);

static int iscsi_tx_login_reject(struct iscsi_conn *conn,
								 struct iscsi_init_login_cmnd *pdu,
								 uint8_t status_class,
								 uint8_t status_detail);

/* Daren Hayward, darenh@4bridgeworks.com */

struct iscsi_global *devdata;

/* 
 * initialize stuff in devdata.
 */
static int
init_target(struct iscsi_global *devdata)
{
	memset(devdata, 0, sizeof(struct iscsi_global));

	INIT_LIST_HEAD(&devdata->session_list);
	INIT_LIST_HEAD(&devdata->bad_session_list);

	init_MUTEX(&devdata->session_sem);
	init_MUTEX(&devdata->session_read_mutex);
	init_MUTEX_LOCKED(&devdata->server_sem);

	devdata->param_tbl = my_kmalloc(MAX_CONFIG_PARAMS
							* sizeof(struct parameter_type), "param_tbl");
	if (!(devdata->param_tbl)) {
		return -1;
	}

	/* Copy the default parameters */
	param_tbl_init(*devdata->param_tbl);

	/* chap and srp support - CHONG */
	devdata->auth_parameter.chap_local_ctx = CHAP_InitializeContext();
	devdata->auth_parameter.chap_peer_ctx = CHAP_InitializeContext();
	devdata->auth_parameter.srp_ctx = SRP_InitializeContext();

	return 0;
}

void
bring_down_portals( void )
{
	int i;
	struct portal_group *ptr;

	for (i = 0, ptr = iscsi_portal_groups; i < MAX_PORTAL; i++, ptr++) {
		if (ptr->in_use == 0)
			continue;
	
		if (devdata->server_thr[i]) {
			/*  Mike Christie mikenc@us.ibm.com */
			send_sig(ISCSI_SHUTDOWN_SIGNAL, devdata->server_thr[i], 1);
			down_interruptible(&devdata->server_sem);
		}
	
		if (devdata->server_socket[i]) {
			sock_release(devdata->server_socket[i]);
			devdata->server_socket[i] = NULL;
		}

		my_kfree((void **)&ptr->ip_address, "ip_address");
		if (ptr->in_use == 2)
		{
			my_kfree((void **)&ptr->ip_string, "ip_string");
			my_kfree((void **)&ptr->port_string, "port_string");
		}
		
		ptr->in_use = 0;
	}

	if (devdata->param_tbl) {
		param_tbl_uncpy(*devdata->param_tbl);
		my_kfree((void**)&devdata->param_tbl, "param_tbl");
	}

	/* chap and srp support - CHONG */
	CHAP_FinalizeContext(devdata->auth_parameter.chap_local_ctx);
	CHAP_FinalizeContext(devdata->auth_parameter.chap_peer_ctx);
	SRP_FinalizeContext(devdata->auth_parameter.srp_ctx);

	my_kfree((void **)&devdata, "devdata");

	/* check for any unfreed memory */
	my_kempty();
}

int stop_server_thread (char *ip_string, char *port_string)
{
	int i;
	
	for (i = 0; i < MAX_PORTAL; i++)
	{
		if (iscsi_portal_groups[i].in_use == 0)
			continue;
	
		if ((strcmp(iscsi_portal_groups[i].ip_string, ip_string) == 0)
			&& (strcmp(iscsi_portal_groups[i].port_string, port_string) == 0))
		{
			if (devdata->server_thr[i]) {
				/*  Mike Christie mikenc@us.ibm.com */
				send_sig(ISCSI_SHUTDOWN_SIGNAL, devdata->server_thr[i], 1);
				down_interruptible(&devdata->server_sem);
			}
	
			if (devdata->server_socket[i]) {
				sock_release(devdata->server_socket[i]);
				devdata->server_socket[i] = NULL;
			}

			my_kfree((void **)&iscsi_portal_groups[i].ip_address, "ip_address");
			if (iscsi_portal_groups[i].in_use == 2)
			{
				my_kfree((void **)&iscsi_portal_groups[i].ip_string, "ip_string");
				my_kfree((void **)&iscsi_portal_groups[i].port_string, "port_string");
			}

			iscsi_portal_groups[i].in_use = 0;
			return 0;
		}
	}

	TRACE_ERROR("%s Can't find match with ip %s, port %s\n", current->comm,
				ip_string, port_string);
	return -1;
}



int start_server_thread (char *ip_string, char *port_string, int tag)
{
	int i;
	struct portal_group *ptr;

	if (!devdata) {
		TRACE_ERROR("%s No device available\n", current->comm);
		goto out;
	}

	for (i = 0, ptr = iscsi_portal_groups; i < MAX_PORTAL; i++, ptr++)
	{
		if (!ptr->in_use)
			break;
	}

	if (i >= MAX_PORTAL)
	{
		TRACE_ERROR("%s No more portals permitted\n", current->comm);
		goto out;
	}

	/* mark this portal in use with dynamic ip strings */
	ptr->in_use = 2;
	
	ptr->ip_string = (char *)my_kmalloc(strlen(ip_string)+1, "ip_string");
	if (ptr->ip_string == NULL) {
		goto out0;
	}
	strcpy(ptr->ip_string, ip_string);

	ptr->port_string = (char *)my_kmalloc(strlen(port_string)+1,"port_string");
	if (ptr->port_string == NULL) {
		goto out1;
	}
	strcpy(ptr->port_string, port_string);

	ptr->tag = tag;
	
	if (bring_up_portal(ptr)) {
		goto out2;
	}

	return 0;

out2:
	my_kfree((void **)&ptr->port_string, "port_string");
out1:
	my_kfree((void **)&ptr->ip_string, "ip_string");
out0:
	ptr->in_use = 0;
out:
	return -1;
}



/*
 * iscsi_release: function to release the iSCSI device as required by
 * the Mid-Level
 */
int
iscsi_release(Scsi_Target_Device * device)
{
	struct iscsi_session *session;
	struct list_head *list_ptr, *list_temp;
	int err = 0;

	TRACE(TRACE_ENTER_LEAVE, "Enter iscsi_release\n");

	if ((struct iscsi_global *) device->dev_specific != devdata) {
		TRACE_ERROR("%s This is not an iscsi device\n", current->comm);
		err = -1;
		goto out;
	}

	/* destructive access to session lists */
	if (!down_interruptible(&devdata->session_sem)) {
		list_for_each_safe(list_ptr, list_temp, &devdata->session_list) {
			session = list_entry(list_ptr, struct iscsi_session, sess_link);
			iscsi_release_session(session);
		}
		list_for_each_safe(list_ptr, list_temp, &devdata->bad_session_list) {
			session = list_entry(list_ptr, struct iscsi_session, sess_link);
			TRACE(TRACE_DEBUG, "iscsi%d: release bad session %p, tsih %u\n",
			  	(int) devdata->device->id, session, session->tsih);
			iscsi_release_session(session);
		}
		up(&devdata->session_sem);
	}
	
	bring_down_portals();

out:
	TRACE(TRACE_ENTER_LEAVE, "Leave iscsi_release, err %d\n", err);

	return err;
}

/*
 * generate the next TTT in a session
 * must be called with session->cmnd_sem lock held
 */
static inline uint32_t __attribute__ ((no_instrument_function))
generate_next_ttt(struct iscsi_session* session)
{
	uint32_t retval;

	retval = session->cmnd_id++;
	if (session->cmnd_id == 0) {
		/* just assigned -1 to retval, which is reserved
		 * so do it again, since 0 is not reserved
		 */
		retval = session->cmnd_id++;
	}
	return retval;
}

/*
 * allocates all the structures necessary for a new connection and a
 * new session.  If later we find out that this connection belongs to
 * an existing session, the session structure alloced here will be
 * freed.  Its just easier to do everything here in one place, and it
 * makes it easier to recover from errors if everything is all set up
 * (i.e., there are no partial structures)
 * returns NULL if anything fails, in which case everything is freed
 *		   pointer to new iscsi_conn, which points to new iscsi_sess, if all ok
 */
static struct iscsi_conn *
build_conn_sess(struct socket *sock, struct portal_group *ptr)
{
	struct iscsi_conn *conn;
	struct iscsi_session *session;
	int addr_len;

	conn = (struct iscsi_conn *)my_kmalloc(sizeof(struct iscsi_conn),
										   "iscsi_conn");
	if (!conn) {
		goto out1;
	}

	TRACE(TRACE_DEBUG, "new conn %p for sock %p\n", conn, sock);

	memset(conn, 0, sizeof(struct iscsi_conn));

	INIT_LIST_HEAD(&conn->conn_link);

	INIT_LIST_HEAD(&conn->reject_list);
	init_MUTEX(&conn->reject_sem);

	conn->active = 1;
	conn->conn_id = ++devdata->conn_id;
	conn->conn_socket = sock;
	conn->dev = devdata;
	conn->max_send_length = 8192;
	conn->max_recv_length = 8192;
	conn->portal_group_tag = ptr->tag;
	conn->connection_flags = devdata->force;

	conn->nop_period = devdata->nop_period;
	init_timer(&conn->nop_timer);

	init_MUTEX_LOCKED(&conn->tx_sem);
	init_MUTEX_LOCKED(&conn->kill_rx_sem);
	init_MUTEX_LOCKED(&conn->kill_tx_sem);
	init_MUTEX(&conn->text_in_progress_sem);

	/* get ip addressing info about both ends of this connection */
	if ((conn->ip_address = my_kmalloc(sizeof(struct sockaddr), "ip address"))
								== NULL) {
		goto out2;
	}
	addr_len = ptr->ip_length;		/* value-result parameter */
	if (sock->ops->getname(sock, conn->ip_address, &addr_len, 1) < 0) {
		TRACE_ERROR("%s Could not get peer name for socket %p\n", current->comm,
					sock);
		goto out3;
	}

	if ((conn->local_ip_address = my_kmalloc(sizeof(struct sockaddr),
											 "local ip address")) == NULL) {
		goto out3;
	}
	addr_len = ptr->ip_length;		/* value-result parameter */
	if (sock->ops->getname(sock, conn->local_ip_address, &addr_len, 0) < 0) {
		TRACE_ERROR("%s Could not get local name for socket %p\n",
					current->comm, sock);
		goto out4;
	}

	session = (struct iscsi_session *)my_kmalloc(sizeof(struct iscsi_session),
												 "session");
	if (!session) {
			goto out4;
	}

	memset(session, 0, sizeof(struct iscsi_session));

	INIT_LIST_HEAD(&session->sess_link);
	INIT_LIST_HEAD(&session->conn_list);
	list_add_tail(&conn->conn_link, &session->conn_list);

	conn->session = session;
	session->nconn = 1;
	session->devdata = devdata;
	session->portal_group_tag = ptr->tag;
	session->version_max = ISCSI_MAX_VERSION;
	session->version_min = ISCSI_MIN_VERSION;

	if ((session->r2t_timer = (struct timer_list *)
				my_kmalloc(sizeof(struct timer_list), "r2t timer")) == NULL) {
				goto out5;
	}
	init_timer(session->r2t_timer);
	TRACE(TRACE_DEBUG, "Allocated R2T timer %p for session %p\n",
		  session->r2t_timer, session);

	if (!(session->session_params = my_kmalloc(MAX_CONFIG_PARAMS *
											   sizeof(struct parameter_type),
											   "session_params"))) {
		goto out6;
	}

	if (!(session->oper_param = my_kmalloc(MAX_CONFIG_PARAMS *
								sizeof(struct session_operational_parameters),
								"oper_param"))) {
		goto out7;
	}

	/* copy the parameters information from the global structure */
	param_tbl_cpy(*session->session_params, *devdata->param_tbl);
	session->r2t_period = devdata->r2t_period;

	/* Store SNACK flags as part of Session - SAI */
	session->targ_snack_flg = devdata->targ_snack_flg;

	init_MUTEX(&session->cmnd_sem);
	init_MUTEX_LOCKED(&session->retran_sem);
	init_MUTEX_LOCKED(&session->thr_kill_sem);

	/* all ok */
	goto out;

out7:
	my_kfree((void**)&session->session_params, "session_params");

out6:
	TRACE(TRACE_DEBUG, "Releasing R2T timer %p for session %p\n",
		  session->r2t_timer, session);
	my_kfree((void**)&session->r2t_timer, "r2t timer");

out5:
	my_kfree((void**)&session, "session");

out4:
	my_kfree((void**)&conn->local_ip_address, "local ip address");
	
out3:
	my_kfree((void**)&conn->ip_address, "ip address");

out2:
	my_kfree((void**)&conn, "iscsi_conn");
	
out1:
	sock_release(sock);

out:
	return conn;
}

/*
 * iscsi_server_thread: This thread is responsible for accepting
 * connections
 */
int
iscsi_server_thread(void *param)
{
	struct iscsi_conn *new_conn;
	struct socket *newsock, *sock;
	struct portal_group *ptr = param;
	long i;

	i = ptr - iscsi_portal_groups;
	
	lock_kernel();

/* Ming Zhang, mingz@ele.uri.edu */
#ifdef K26
	daemonize(
#else
	daemonize();
	snprintf(current->comm, sizeof(current->comm),
#endif
			 "iscsi_thr_%ld", i);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 4, 18)
	/*
	 * Arne Redlich, agr1@users.sourceforge.net:
	 * This prevents the server thread from becoming a zombie after it exits.
	 */
	reparent_to_init();
#endif

	siginitsetinv(&current->blocked, ISCSI_SHUTDOWN_SIGBITS);

	/* mark that this server thread is alive */
	devdata->server_thr[i] = current;

	unlock_kernel();

	printk("%s Starting pid %d\n", current->comm, current->pid);

	sock = devdata->server_socket[i];

	printk("%s Listening on %s:%s\n", current->comm,
											ptr->ip_string,
											ptr->port_string);

	/* notify our parent that this thread is up */
	up(&devdata->server_sem);

	/* loop forever accepting new connections from initiators */
	for ( ; ; ) {

	/* fix for 2.6.10 contributed by Charles Coffing, ccoffing@novell.com */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 9)
		newsock = sock_alloc();
		if (!newsock)
#else
		int ret;

		ret = sock_create_kern(sock->sk->sk_family, sock->type,
							   sock->sk->sk_protocol, &newsock);
		if (ret)
#endif
		{
			TRACE_ERROR("%s Could not allocate socket for accept\n",
						current->comm);
			goto iscsi_server_thread_out;
		}

		newsock->type = sock->type;
		newsock->ops = sock->ops;

		TRACE(TRACE_NET, "before accept: sock %p newsock %p\n", sock, newsock);

		if (sock->ops->accept(sock, newsock, 0) < 0) {
			goto out1;
		}

		TRACE(TRACE_NET, "after accept: sock %p newsock %p\n", sock, newsock);

		/* turn off the Nagle Algorithm on this socket */
		tcp_nagle_off(newsock);

		/* clear bad sessions and connections here */
		clean_bad_stuff();

		/* allocate and fill in connection details */
		if ((new_conn = build_conn_sess(newsock, ptr)) == NULL)
			goto iscsi_server_thread_out;

		TRACE(TRACE_ISCSI, "Connect socket %p on conn %p, cid %d, session %p\n",
			  newsock, new_conn, new_conn->conn_id, new_conn->session);
		/* 
		 * NOTE THAT WE CANNOT DO MUCH OF ANYTHING WITH THIS
		 * CONNECTION AS WE DO NOT KNOW WHAT SESSION IT BELONGS
		 * TO. THIS WILL HAVE TO BE A PART OF THE RX_THREAD
		 */

		/* start the tx thread */
		if (kernel_thread(iscsi_tx_thread, (void *)new_conn, 0) < 0) {
			TRACE_ERROR("%s unable to create tx_thread\n", current->comm);
			goto out1;
		}

		/* start the rx thread */
		if (kernel_thread(iscsi_rx_thread, (void *)new_conn, 0) < 0) {
			TRACE_ERROR("%s unable to create rx_thread\n", current->comm);
			if (new_conn->tx_thread) {
				/*  Mike Christie mikenc@us.ibm.com */
				send_sig(ISCSI_SHUTDOWN_SIGNAL, new_conn->tx_thread, 1);
			}
			goto out1;
		}
	}

out1:
	sock_release(newsock);

iscsi_server_thread_out:

	/* mark that this server thread is no longer alive */
	devdata->server_thr[i] = NULL;

	printk("%s closed %s:%s\n", current->comm,
							ptr->ip_string,
							ptr->port_string);

	/* exit this thread after notifying whoever might be waiting */
	up(&devdata->server_sem);
	printk("%s Exiting pid %d\n", current->comm, current->pid);
	return 0;
}

/*
 * for each session on the list,
 * release inactive connections and the session itself if no conns remain.
 * devdata->session_sem MUST be locked before this routine is called.
 */
static void __attribute__ ((no_instrument_function))
clean_session_list(struct list_head *list)
{
	struct iscsi_session *session;
	struct iscsi_conn *conn;
	struct list_head *list_ptr1, *list_temp1;
	struct list_head *list_ptr2, *list_temp2;

	list_for_each_safe(list_ptr1, list_temp1, list) {
		session = list_entry(list_ptr1, struct iscsi_session, sess_link);

		list_for_each_safe(list_ptr2, list_temp2, &session->conn_list) {
			conn = list_entry(list_ptr2, struct iscsi_conn, conn_link);

			if (conn->active == 0) {
				if (iscsi_release_connection(conn) < 0) {
					TRACE_ERROR("%s Error releasing connection\n",
								current->comm);
				}
			}
		}

		if (list_empty(&session->conn_list)) {
			iscsi_release_session(session);
		}
	}
}

static void __attribute__ ((no_instrument_function))
clean_bad_stuff(void)
{
	/* destructive access to session lists */
	if (!down_interruptible(&devdata->session_sem)) {
		clean_session_list(&devdata->bad_session_list);
		clean_session_list(&devdata->session_list);
		up(&devdata->session_sem);
	}
}

/*
 * iscsi_release_session: This function is responsible for closing out
 * a session and removing it from whatever list it is on.
 * host->session_sem MUST be locked before this routine is called.
 * INPUT: session to be released
 * OUTPUT: 0 if success, < 0 if there is trouble
 */
int
iscsi_release_session(struct iscsi_session *session)
{
	struct iscsi_cmnd *cmnd;
	struct iscsi_conn *conn;
	struct list_head *list_ptr, *list_temp;

	if (!session) {
		TRACE_ERROR("%s Cannot release a NULL session\n", current->comm);
		return -1;
	}

	if (TRACE_TEST(TRACE_ISCSI)) {
		print_isid_tsih_message(session, "Release session with ");
	}

	TRACE(TRACE_DEBUG, "Releasing R2T timer %p for session %p\n",
		  session->r2t_timer, session);

	/* Delete r2t timer - SAI */
	if (session->r2t_timer) {
		TRACE(TRACE_DEBUG, "Deleting r2t timer %p\n", session->r2t_timer);
		del_timer_sync(session->r2t_timer);
		TRACE(TRACE_DEBUG, "Deleted r2t timer\n");
		my_kfree((void**)&session->r2t_timer, "r2t timer");
	}

	/* free commands */
	while ((cmnd = session->cmnd_list)) {
		session->cmnd_list = cmnd->next;

		if (cmnd->cmnd != NULL) {
			if (scsi_release(cmnd->cmnd) < 0) {
				TRACE_ERROR("%s Trouble releasing command, opcode 0x%02x, "
							"ITT %u, state 0x%x\n", current->comm,
							cmnd->opcode_byte, cmnd->init_task_tag,
							cmnd->state);
			}
		}
		/* free data_list if any, cdeng */
		free_data_list(cmnd);
		my_kfree((void**)&cmnd->ping_data, "data_buf");
		my_kfree((void**)&cmnd, "iscsi_cmnd");
	}

	/* free connections */
	list_for_each_safe(list_ptr, list_temp, &session->conn_list) {
		conn = list_entry(list_ptr, struct iscsi_conn, conn_link);
		TRACE(TRACE_ISCSI, "iscsi%d: releasing connection %d\n", (int)
			  session->devdata->device->id, conn->conn_id);

		if (iscsi_release_connection(conn) < 0) {
			TRACE_ERROR("%s Trouble releasing connection\n", current->comm);
		}
	}

	/* dequeue session if it is linked into some list */
	if (!list_empty(&session->sess_link)) {
		list_del(&session->sess_link);

		/* error recovery ver new 18_04 - SAI */
		if (session->retran_thread) {
			/*  Mike Christie mikenc@us.ibm.com */
			send_sig(ISCSI_SHUTDOWN_SIGNAL, session->retran_thread, 1);
			down_interruptible(&session->thr_kill_sem);
		}
	}

	/* free session structures */
	my_kfree((void**)&session->session_params, "session_params");

	my_kfree((void**)&session->oper_param, "oper_param");

	my_kfree((void**)&session, "session");

	return 0;
}

/*
 * called by both rx and tx threads, but only after the rx thread has been
 * started on a connection (so the connected message has been given).
 * if socket for this conn has not already been released, do it now and
 * print a message that says we disconnected.
 * Also NULL out the conn's socket so we don't try to do this again later.
 */
static void
iscsi_release_socket(struct iscsi_conn *conn)
{
	if (conn->conn_socket != NULL) {
		/* Release socket */
		TRACE(TRACE_ISCSI, "Release socket %p on conn %p, cid %d, session %p\n",
			  conn->conn_socket, conn, conn->conn_id, conn->session);

		sock_release(conn->conn_socket);
		conn->conn_socket = NULL;
	}
}

/*
 * iscsi_release_connection: releases all the stuff related to a conn.
 * Right now, it does not do anything for releasing commands specific
 * to a connection since commands are more specific to a session as
 * against a connection. It also dequeues the connection from the list
 */
static int
iscsi_release_connection(struct iscsi_conn *conn)
{
	if (!conn) {
		TRACE_ERROR("%s Cannot release NULL connection/n", current->comm);
		return -1;
	}

	/* dequeue any unsent reject messages (do not send them) */
	dequeue_reject(conn, 0);

	/* Kill the rx and tx thread */

	if (conn->rx_thread) {
		/*  Mike Christie mikenc@us.ibm.com */
		TRACE(TRACE_ISCSI, "Releasing iscsi_rx_%d\n", conn->conn_id);
		send_sig(ISCSI_SHUTDOWN_SIGNAL, conn->rx_thread, 1);
		down_interruptible(&conn->kill_rx_sem);
	}

	if (conn->tx_thread) {
		TRACE(TRACE_ISCSI, "Releasing iscsi_tx_%d\n", conn->conn_id);
		/*  Mike Christie mikenc@us.ibm.com */
		send_sig(ISCSI_SHUTDOWN_SIGNAL, conn->tx_thread, 1);
		down_interruptible(&conn->kill_tx_sem);
	}

	/* Release socket */
	iscsi_release_socket(conn);

	TRACE(TRACE_ISCSI_FULL, "Dequeue connection conn->cid %u\n", conn->conn_id);

	/* Dequeue connection */
	list_del(&conn->conn_link);
	conn->session->nconn--;

	/* Free connection */
	my_kfree((void **)&conn->local_ip_address, "local ip address");
	my_kfree((void **)&conn->ip_address, "ip address");
	del_timer_sync(&conn->nop_timer);
	my_kfree((void**)&conn, "conn");

	return 0;
}

/*
 * executed only by the rx thread.
 * Allocates new reject item, fills it in with header of rejected pdu,
 * and enqueues it for tx thread on reject_list of this connection.
 */
int
enqueue_reject(struct iscsi_conn *conn, uint8_t reason)
{
	int err = 0;
	struct reject_item	*new_item;

	TRACE(TRACE_ENTER_LEAVE, "Enter enqueue_reject, reason %u\n", reason);

	if ((new_item = (struct reject_item *)my_kmalloc(sizeof(struct reject_item),
													 "reject item")) == NULL) {
		err = -1;
		goto out;
	}
	memcpy(new_item->bad_header, conn->bad_hdr, ISCSI_HDR_LEN);
	new_item->reason = reason;
	if (down_interruptible(&conn->reject_sem)) {
		my_kfree((void **)&new_item, "reject item");
		err = -1;
		goto out;
	}
	list_add_tail(&new_item->reject_link, &conn->reject_list);
	up(&conn->reject_sem);

	if (atomic_read(&conn->tx_sem.count) <= 0) {
		up(&conn->tx_sem);
	}
out:
	TRACE(TRACE_ENTER_LEAVE, "Leave enqueue_reject, err %d\n", err);

	return err;
}

/* 
 * iscsi_rx_thread: This thread is responsible for receiving iSCSI PDUs
 * and messages from the Initiator and then figuring out what to do with
 * them
 */
int
iscsi_rx_thread(void *param)
{
	struct iscsi_conn *conn = (struct iscsi_conn *) param;
	struct iovec iov;				/* for reading header and header digest */
	struct targ_error_rec err_rec;
	uint8_t buffer[ISCSI_HDR_LEN];		/* read 48-byte PDU header in here */
	uint32_t digest;					/* read 4-byte header digest in here */
	uint32_t hdr_crc;					/* compute PDU header digest in here */
	uint32_t opcode;					/* extract 6-bit PDU opcode into here */
	uint32_t local_itt;				/* convert 4-byte PDU ITT into here */
	int err;

	lock_kernel();

/* Ming Zhang, mingz@ele.uri.edu */
#ifdef K26
	daemonize(
#else
	daemonize();
	sprintf(current->comm,
#endif

			"iscsi_rx_%d", conn->conn_id);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 4, 18)
	/*
	 * Arne Redlich, agr1@users.sourceforge.net:
	 * This prevents the rx_thread from becoming a zombie after it exits.
	 */
	reparent_to_init();
#endif

	siginitsetinv(&current->blocked, ISCSI_SHUTDOWN_SIGBITS);

	/* mark that this rx thread is alive */
	conn->rx_thread = current;

	unlock_kernel();

	printk("%s Starting pid %d\n", current->comm, current->pid);

	while (1) {
		iov.iov_len = ISCSI_HDR_LEN;
		iov.iov_base = buffer;

		/* receive iSCSI header */
		err = iscsi_rx_data(conn, &iov, 1, ISCSI_HDR_LEN);
		if (err != ISCSI_HDR_LEN) {
			goto iscsi_rx_thread_out;
		}

		TRACE_BUFFER(TRACE_BUF, buffer, ISCSI_HDR_LEN,
					 "%s Got PDU header\n", current->comm);

		opcode = buffer[0] & ISCSI_OPCODE;

		local_itt = ntohl(((struct generic_pdu *)buffer)->init_task_tag);

		if (!(conn->session->tsih)) {
			/* not in full feature phase yet, only accept Login request pdus */
			if (opcode != ISCSI_INIT_LOGIN_CMND) {
				TRACE_ERROR("%s ITT %u has opcode 0x%02x, expected 0x%02x\n",
							current->comm, local_itt, opcode,
							ISCSI_INIT_LOGIN_CMND);
				/* MUST immediately terminate the connection */
				goto iscsi_rx_thread_out;
			}

			TRACE(TRACE_ISCSI, "%s Got login request, ITT %u\n",
				  current->comm, local_itt);

			if (handle_login(conn, buffer) < 0) {
				TRACE_ERROR("%s Trouble in handle_login\n", current->comm);
				goto iscsi_rx_thread_out;
			}
			continue;
		}

		/* keep a copy of this header in case we need it in a later reject */
		memcpy(conn->bad_hdr, buffer, ISCSI_HDR_LEN);

		/* in case there are any out-of-order commands that are now in-order */
		check_queued_cmnd(conn->session);

	} /* while(1) */

iscsi_rx_thread_out:
	conn->active = 0;
	conn->rx_thread = NULL;
	if (conn->tx_thread) {
		/* tx thread still going, shut it down */
		send_sig(ISCSI_SHUTDOWN_SIGNAL, conn->tx_thread, 1);
	} else {
		/* tx thread has shut down, if socket is still open, release it */
		iscsi_release_socket(conn);
	}
	up(&conn->kill_rx_sem);
	printk("%s Exiting pid %d\n", current->comm, current->pid);
	return 0;
}

/*
 * iscsi_rx_data: will receive a fixed amount of data.
 * INPUT: struct iscsi_conn (what connection), struct iovec, number of iovecs,
 * total amount of data in bytes
 * OUTPUT: > 0 total bytes read if everything is okay
 * 	       < 0 if trouble (-ECONNRESET means end-of-file)
 *		   = 0 for PAYLOAD_DIGERR
 */
int
iscsi_rx_data(struct iscsi_conn *conn, struct iovec *iov, int niov, int data)
{
	struct msghdr msg;
	int msg_iovlen;
	int rx_loop, total_rx;
	uint32_t checksum, data_crc;
	int i;
	struct iovec *iov_copy, *iov_ptr;
	mm_segment_t oldfs;

	total_rx = 0;

	TRACE(TRACE_ENTER_LEAVE, "Enter iscsi_rx_data, niov %d, data %d\n",
		  niov, data);

	if (!conn->conn_socket) {
		TRACE_ERROR("%s Transport endpoint is not connected\n", current->comm);
		total_rx = -ENOTCONN;		/* Transport endpoint is not connected */
		goto out;
	}

	iov_copy =
		(struct iovec *)my_kmalloc(niov * sizeof(struct iovec), "iovec");
	if (iov_copy == NULL) {
		total_rx = -ENOMEM;
		goto out;
	}

	while (total_rx < data) {
		/* get a clean copy of the original io vector to work with */
		memcpy(iov_copy, iov, niov * sizeof(struct iovec));
		msg_iovlen = niov;
		iov_ptr = iov_copy;

		if ((rx_loop = total_rx)) {
			/* have done only partial read, recalculate iov and niov -- cdeng */
			TRACE(TRACE_ISCSI,
				  "iscsi_rx_data: data %d, received so far %d, recompute iov\n",
				  data, total_rx);
			do {
				if (iov_ptr->iov_len <= rx_loop) {
					rx_loop -= iov_ptr->iov_len;
					iov_ptr++;
					msg_iovlen--;
				} else {
					iov_ptr->iov_base += rx_loop;
					iov_ptr->iov_len -= rx_loop;
					rx_loop = 0;
				}
			} while (rx_loop);
		}

		memset(&msg, 0, sizeof(struct msghdr));
		msg.msg_iovlen = msg_iovlen;
		msg.msg_iov = iov_ptr;

		oldfs = get_fs();
		set_fs(get_ds());
		rx_loop = sock_recvmsg(conn->conn_socket, &msg, (data - total_rx),
							   MSG_WAITALL);
		set_fs(oldfs);

		/* this receive from initiator broke the silence */
		clear_bit(SILENCE_BIT, &conn->control);

		if (rx_loop <= 0) {
			my_kfree((void **)&iov_copy, "iovec");
			total_rx = -ECONNRESET;		/* Connection reset by peer */
			goto out;
		}

		total_rx += rx_loop;
		TRACE(TRACE_DEBUG, "iscsi_rx_data: rx_loop %d total_rx %d\n", rx_loop,
			  total_rx);
	}

	my_kfree((void**)&iov_copy, "iovec");

	if (niov > 1 && conn->data_crc) {
		/* this really is for a data segment, and data digests are in effect */
		data_crc = 0;
		for (i = 0; i < niov - 1; i++)
			do_crc(iov[i].iov_base, iov[i].iov_len, &data_crc);
		checksum = *(uint32_t *) (iov[niov - 1].iov_base);
		if (checksum != data_crc) {
			TRACE_ERROR("%s Got data crc 0x%08x, expected 0x%08x\n",
						current->comm, ntohl(checksum), ntohl(data_crc));
			/* Data Digest Error Recovery - SAI */
			total_rx = PAYLOAD_DIGERR;
			goto out;
		} else {
			TRACE(TRACE_ISCSI_FULL, "Got data crc 0x%08x\n", ntohl(checksum));
		}
	}
out:
	TRACE(TRACE_ENTER_LEAVE, "Leave iscsi_rx_data, total_rx %d\n", total_rx);

	return total_rx;
}


/**************************************************************************/
/*                          handle income pdu                             */
/**************************************************************************/

/*
 * called only from rx thread.
 * handle_login: This function is responsible for interpreting the login
 * message and then making sure the necessary details get filled in, a
 * new session is opened if required by the login and the response is returned
 *
 * INPUT: Details of the connection, buffer containing the login
 * OUTPUT: 0 if everything is okay, < 0 if there is a problem
 * Note a 0 value returned may still mean that a connection is rejected
 */
static int
handle_login(struct iscsi_conn *conn, uint8_t *buffer)
{
	struct iscsi_init_login_cmnd *pdu = (struct iscsi_init_login_cmnd *) buffer;
	struct iscsi_session *session, *temp;
	struct iscsi_conn *temp_conn;
	uint32_t when_called;
	int found, retval = -1;
	struct auth_parameter_type auth_param;	/*chap and srp support - CHONG */
	struct parameter_type *p;
	struct parameter_type (*this_param_tbl)[MAX_CONFIG_PARAMS];
	struct parameter_type (*temp_params)[MAX_CONFIG_PARAMS];
	struct list_head *list_ptr;
	struct iscsi_global *host;

	if (TRACE_TEST(TRACE_ISCSI_FULL))
		print_init_login_cmnd(pdu);

	/* this will be non-NULL if the parameter table should be freed when done */
	temp_params = NULL;

	if ((session = conn->session) == NULL) {
		/* should never happen */
		goto out;
	}

	if ((host = session->devdata) == NULL) {
		/* should never happen */
		goto out;
	}

	/* always use clean copy of configured parameter table for negotiations */
	this_param_tbl = session->session_params;

	pdu->length = be32_to_cpu(pdu->length);
	pdu->tsih = be16_to_cpu(pdu->tsih);
	pdu->init_task_tag = be32_to_cpu(pdu->init_task_tag);
	pdu->cid = be16_to_cpu(pdu->cid);
	pdu->cmd_sn = be32_to_cpu(pdu->cmd_sn);
	pdu->exp_stat_sn = be32_to_cpu(pdu->exp_stat_sn);

	/* destructive access to session lists */
	if (down_interruptible(&host->session_sem))
		goto err_conn_out;

	if (pdu->tsih == 0) {
		/* a new session, the iscsi_session structure is already set up */
		conn->cid = pdu->cid;
		conn->stat_sn = pdu->exp_stat_sn;
		session->cmd_sn = pdu->cmd_sn;
		session->exp_cmd_sn = pdu->cmd_sn;
		session->max_cmd_sn = pdu->cmd_sn + QUEUE_DEPTH_ALLOWED;

		memcpy(session->isid, pdu->isid, 6);

		/* set up the operational parameters from the global structure */
		set_session_parameters(session->oper_param, *session->session_params);

		/* add new session to the end of the global session list
		 * with tsih = 0 to indicate login not finished
		 */
		list_add_tail(&session->sess_link, &host->session_list);

		/* set flags to allow keys for new session (leading) login */
		when_called = LEADING_ONLY | INITIAL_ONLY | ALL;

		TRACE(TRACE_ISCSI, "%s Start new session %p\n",
			  current->comm, session);
	} else {
		/* existing session, check through the session list to find it */
		found = 0;
		list_for_each(list_ptr, &conn->dev->session_list) {
			session = list_entry(list_ptr, struct iscsi_session, sess_link);
			if (session->tsih == pdu->tsih) {
				found = 1;
				break;
			}
		}

		if (!found) {
			TRACE_ERROR("%s No existing session with TSIH %u, "
						"terminate this connection\n", current->comm,
						pdu->tsih);
			goto err_conn_out;
		}

		if (conn->portal_group_tag != session->portal_group_tag) {
			TRACE_ERROR("%s Portal group tag %u for new connection does "
						"not match portal group tag %u of session\n",
						current->comm, conn->portal_group_tag,
						session->portal_group_tag);

			iscsi_tx_login_reject(conn, pdu, STAT_CLASS_INITIATOR,
								  STAT_DETAIL_NOT_INCLUDED);
			goto err_conn_out;
		}

		/* check isid */
		if (memcmp(pdu->isid, session->isid, 6) != 0) {
			TRACE_ERROR("%s The session has a different ISID, "
						"terminate the connection\n", current->comm);

			iscsi_tx_login_reject(conn, pdu, STAT_CLASS_INITIATOR,
								  STAT_DETAIL_ERR);
			goto err_conn_out;
		}

		conn->cid = pdu->cid;
		conn->stat_sn = pdu->exp_stat_sn;

		/* check cid, and if it already exists then release old connection */
		list_for_each(list_ptr, &session->conn_list) {
			temp_conn = list_entry(list_ptr, struct iscsi_conn, conn_link);
			if (temp_conn->cid == conn->cid) {
				TRACE(TRACE_ISCSI, "connection reinstatement with cid %u\n",
					  conn->cid);

				if (iscsi_release_connection(temp_conn) < 0) {
					TRACE_ERROR("%s Error releasing connection\n",
								current->comm);
				}
				break;
			}
		}

		TRACE(TRACE_ISCSI, "new connection cid %u attached to "
				"existing session tsih %u, IP 0x%08x\n",
			  conn->cid, pdu->tsih,
			  ((struct sockaddr_in *)conn->ip_address)->sin_addr.s_addr);

		/* add new connection to end of connection list for existing session */
		temp = conn->session;
		conn->session = session;
		list_del(&conn->conn_link);
		temp->nconn = 0;
		list_add_tail(&conn->conn_link, &session->conn_list);
		session->nconn++;

		/* use clean parameter table for negotiations, free it later */
		temp_params = this_param_tbl;
		temp->session_params = NULL;

		/* free up the no-longer-needed session structure */
		iscsi_release_session(temp);

		/* set back the leading-only keys if these keys were set to
		 * " KEY_TO_BE_NEGOTIATED" in the leading connection negotation.*/
		reset_parameter_table(*this_param_tbl);

		/* set flags to allow keys for new connection (only) login */
		when_called = INITIAL_ONLY | ALL;
	}

	/* by the time we get here, the connection is attached to the correct
	 * session and that session is threaded onto the devdata->session_list.
	 */

	up(&host->session_sem);

	/* chap and srp support - CHONG */
	auth_param.auth_flags = 0;
	auth_param.chap_local_ctx =
		CHAP_CloneContext(conn->dev->auth_parameter.chap_local_ctx);
	auth_param.chap_peer_ctx =
		CHAP_CloneContext(conn->dev->auth_parameter.chap_peer_ctx);
	auth_param.srp_ctx = SRP_CloneContext(conn->dev->auth_parameter.srp_ctx);

	if ((p = find_flag_parameter(TARGETPORTALGROUPTAG_FLAG,
								 *conn->session->session_params))) {
		/* force the TargetPortalGroupTag key to have as its value the
		   portal group tag for this session */
		p->int_value = conn->session->portal_group_tag;
	}

	if (parameter_negotiate(conn, *this_param_tbl, pdu, when_called, auth_param)
							< 0) {
		TRACE_ERROR("%s Parameter negotiation failed\n", current->comm);
		/* chap and srp support - CHONG */
		CHAP_FinalizeContext(auth_param.chap_local_ctx);
		CHAP_FinalizeContext(auth_param.chap_peer_ctx);
		SRP_FinalizeContext(auth_param.srp_ctx);
		goto out;
	}

	/* Bjorn Thordarson, 10-May-2004 -- start -- */
	if (TRACE_TEST(TRACE_ISCSI_FULL)) {
		int k;

		for (k = 0; k < MAX_CONFIG_PARAMS ; k++) {
			if ((*this_param_tbl)[k].parameter_name != NULL &&
				(*this_param_tbl)[k].str_value != NULL)
				printk("PARAM: %s = %s\n", (*this_param_tbl)[k].parameter_name,
					   (*this_param_tbl)[k].str_value);
		}
	}
	/* Bjorn Thordarson, 10-May-2004 -- end -- */

	/* chap and srp support - CHONG */
	CHAP_FinalizeContext(auth_param.chap_local_ctx);
	CHAP_FinalizeContext(auth_param.chap_peer_ctx);
	SRP_FinalizeContext(auth_param.srp_ctx);

	conn->stat_sn++;

	if (conn->stat_sn < 0) {
		TRACE_ERROR("%s Parameter negotiation with error stat_sn\n",
					current->comm);
		goto out;
	}

	/* set the operational parameters to the negotiated value */
	if (pdu->tsih == 0) {
		/* this is a new connection in a new session */
		set_session_parameters(session->oper_param, *session->session_params);

		/* Initialize R2T timer - ref18_09c SAI */
		if (session->oper_param->ErrorRecoveryLevel > 0
			&& session->r2t_period > 0) {
			/* error recovery ver ref18_04 */

			/* create a retransmit_thread for handling error recovery */
			if (kernel_thread(iscsi_retran_thread, (void *)session, 0) < 0) {
				TRACE_ERROR("%s Unable to create retran_thread\n",
							current->comm);
				session->r2t_period = 0;
			} else {
				restart_r2t_timer(session);
				TRACE(TRACE_ISCSI, "R2T timer %p started for session %p\n",
					  session->r2t_timer, session);
			}
		}
	}

	/* we are now in Full Feature Phase */
	conn->hdr_crc = (conn->connection_flags & USE_HEADERDIGEST) ? 1 : 0;
	conn->data_crc = (conn->connection_flags & USE_DATADIGEST) ? 1 : 0;

	/* all ok */
	retval = 0;

out:
	/* here when nothing further can or needs to be done
	 * to the session or connection structs
	 */
	if (temp_params) {
		param_tbl_uncpy(*temp_params);
		my_kfree((void**)&temp_params, "temp_params");
	}

	return retval;

err_conn_out:
	/* here on a fatal error detected before session is linked into devdata */
	iscsi_release_socket(conn);

	/* put this session onto end of "bad-session" list for cleanup later */
	session = conn->session;
	TRACE(TRACE_DEBUG, "add to list bad session %p, conn %p\n",
		  session, conn);

	list_add_tail(&session->sess_link, &host->bad_session_list);

	up(&host->session_sem);

	goto out;
}

/**************************************************************************/
/*                            handle response                             */
/**************************************************************************/


#define UNDERFLOW_FLAG	0x0001
#define OVERFLOW_FLAG	0x0002
#define SEND_SENSE_FLAG	0x0004
#define LAST_SEQ_FLAG	0x0010

static __u32
do_command_status(struct iscsi_cmnd *cmnd, Scsi_Request *req,
				  int *data_left, int *residual_count)
{
	int transfer = 0;
	int data_length_left;
	__u32 flags = 0;

	data_length_left = req->sr_bufflen;

	TRACE(TRACE_DEBUG, "Sense: %02x %02x %02x %02x %02x %02x %02x %02x\n",
		  req->sr_sense_buffer[0], req->sr_sense_buffer[1],
		  req->sr_sense_buffer[2], req->sr_sense_buffer[3],
		  req->sr_sense_buffer[4], req->sr_sense_buffer[5],
		  req->sr_sense_buffer[6], req->sr_sense_buffer[7]);

	if ((req->sr_sense_buffer[0] & 0x7e) == 0x70) {
		/* current or deferred error -- else, we don't send the sense buffer */
		flags |= SEND_SENSE_FLAG;

		if (req->sr_sense_buffer[0] & 0x80) {
			/* bytes 3 thru 6 contain valid data */
			transfer = (req->sr_sense_buffer[3] << 24) |
				(req->sr_sense_buffer[4] << 16) |
				(req->sr_sense_buffer[5] << 8) | req->sr_sense_buffer[6];
			TRACE(TRACE_DEBUG, "information in sense data: %d\n", transfer);
		}

		if ((req->sr_sense_buffer[2] & 0x20) != 0) {		/* ILI bit set */
				data_length_left -= transfer;
		}
	} else if (req->sr_command) {
		/* ensure correct DataSegmentLength and ResidualCount */
		TRACE(TRACE_DEBUG, "data_length_left %d, sr_command->resid %d\n",
				data_length_left, req->sr_command->resid);
		data_length_left -= req->sr_command->resid;
	}

	/* check overflow or underflow problem, cdeng 3/24/02 */
	if (data_length_left > cmnd->data_length) {
		*residual_count = data_length_left - cmnd->data_length;
		data_length_left = cmnd->data_length;
		flags |= OVERFLOW_FLAG;
	} else if (data_length_left < cmnd->data_length) {
		*residual_count = cmnd->data_length - data_length_left;
		flags |= UNDERFLOW_FLAG;
	}

	TRACE(TRACE_DEBUG,"data_length_left %d, residual_count %d, flags 0x%08x\n",
		  data_length_left, *residual_count, flags);

	*data_left = data_length_left;
	return flags;
}

/*
 * This is executed only by the rx thread, NOT the tx thread as name implies!!!
 * During a login, this is called by the rx thread to send a login reject pdu.
 * The status_class parameter MUST not be zero.
 * Therefore, conn->session will not be valid when called by rx thread.
 * relevant parts of the standard are:
 *
 * Draft 20 section 5.3.1 Login Phase Start
 *	"-Login Response with Login reject. This is an immediate rejection from
 *	the target that causes the connection to terminate and the session to
 *	terminate if this is the first (or only) connection of a new session.
 *	The T bit and the CSG and NSG fields are reserved."
 *
 * Draft 20 section 10.13.1 Version-max
 *	"All Login responses within the Login Phase MUST carry the same
 *	Version-max."
 *
 * Draft 20 section 10.13.2 Version-active
 *	"All Login responses within the Login Phase MUST carry the same
 *	Version-active."
 *
 * Draft 20 section 10.13.3 TSIH
 *	"With the exception of the Login Final-Response in a new session,
 *	this field should be set to the TSIH provided by the initiator in
 *	the Login Request."
 *
 * Draft 20 section 10.13.4 StatSN
 *	"This field is only valid if the Status-Class is 0."
 *
 * Draft 20 section 10.13.5 Status-Class and Status-Detail
 *	"0 - Success - indicates that the iSCSI target successfully received,
 *	"understood, and accepted the request. The numbering fields (StatSN,
 *	ExpCmdSN, MaxCmdSN) are only valid if Status-Class is 0."
 */
static int
iscsi_tx_login_reject(struct iscsi_conn *conn,
				   struct iscsi_init_login_cmnd *pdu,
				   __u8 status_class, __u8 status_detail)
{
	__u8 iscsi_hdr[ISCSI_HDR_LEN];
	struct iscsi_targ_login_rsp *hdr;

	/* most fields in the Login reject header are zero (reserved) */
	memset(iscsi_hdr, 0x0, ISCSI_HDR_LEN);
	hdr = (struct iscsi_targ_login_rsp *) iscsi_hdr;

	hdr->opcode = ISCSI_TARG_LOGIN_RSP;

	/* the T bit and CSG and NSG fields are reserved on a Login reject */

	hdr->version_max = ISCSI_MAX_VERSION;
	hdr->version_active = ISCSI_MIN_VERSION;

	/* there is no data attached to this login reject */

	memcpy(hdr->isid, pdu->isid, 6);
	hdr->tsih = cpu_to_be16(pdu->tsih);
	hdr->init_task_tag = cpu_to_be32(pdu->init_task_tag);

	/* The numbering fields (StatSN, ExpCmdSn, MaxCmdSN) are only valid if
	 * Status-Class is 0, which it is not on a Login reject */

	hdr->status_class = status_class;
	hdr->status_detail = status_detail;

	if (send_hdr_only(conn, iscsi_hdr) < 0) {
		return -1;
	}

	TRACE(TRACE_ISCSI, "%s login response sent\n", current->comm);

	if (TRACE_TEST(TRACE_ISCSI_FULL))
		print_targ_login_rsp(hdr);

	return 0;
}

/*
 * find_iovec_needed: this function returns the number of iovecs that
 * are needed to receive some data
 * INPUT: amount of data needed, pointer to scattergather list (from the
 * 	  point where the data is to start for this round, offset within
 * 	  the first scattergather element - in case it was not used up
 * 	  completely
 * OUTPUT: number of iovectors needed, else < 0 if there is trouble
 */
int __attribute__ ((no_instrument_function))
find_iovec_needed(__u32 data_len, struct scatterlist *st_list, __u32 offset)
{
	int i = 0;
	__u32 sglen;

	while (data_len) {
		sglen = get_sglen(st_list);
		if (data_len > (sglen - offset))
			data_len -= (sglen - offset);
		else
			data_len = 0;

		i++;
		st_list++;
		offset = 0;
	}

	return (i);
}

/*
 * fill_iovec: This function fills in a given iovec structure so as to
 * reflect a given scattergather list upto the length that is required
 * No checks are made on the length on the iovec given except that it
 * needs to be greater than the size needed. Also, the scatterlist is
 * also not checked. The integer offset upon return reflects the posn of
 * the offset in the last scatterlist that was used for this function
 * INPUT: struct iovec, no. of iovecs, scatterlist, ptr to offset within
 * 	  the first element, and the total length required
 * OUTPUT: how many st_list elements are used, < 0 if there is trouble
 */
int __attribute__ ((no_instrument_function))
fill_iovec(struct iovec *iov, int p, int niov,
		   struct scatterlist *st_list, int *offset, __u32 data)
{
	int count = 0;
	__u32 sglen;

	TRACE(TRACE_DEBUG, "offset: %d, data_len: %d\n", *offset, data);

	iov += p;
	while (data > 0 && p < niov) {
		iov->iov_base = (void *)(get_sgbuf(st_list) + (*offset));
		sglen = get_sglen(st_list);
		if ((sglen - *offset) > data) {
			/* more space left in this st list item than iovector can use */
			iov->iov_len = data;
			*offset += data;
		} else {
			/* iovector uses all the space in this st list item */
			iov->iov_len = sglen - *offset;
			*offset = 0;
			count++;
		}

		data -= iov->iov_len;

		TRACE(TRACE_DEBUG, "iov %p, p %d, iov_base %p, iov_len %d\n", iov,
			  p, iov->iov_base, iov->iov_len);

		p++;
		iov++;
		st_list++;
	}

	return count;
}

/* Daren Hayward, darenh@4bridgeworks.com */
#if defined(MANGLE_INQUIRY_DATA)

/*
 * offsets to first byte and limit byte of version descriptors
 * section of standard INQUIRY data layout
 */
#define VER_DESC_START	58
#define VER_DESC_LIMIT	74

/*      
 * Note the name "driver_template" is required for "scsi_module.c" to work! 
 *      Used by: "/usr/src/linux/drivers/scsi/scsi_module.c" 
 *      Wrap this in an ifdef because "scsi_module.c" says to! 
 */
#ifdef MODULE

MODULE_AUTHOR("UNH IOL ISCSI");
MODULE_DESCRIPTION("UNH-IOL iSCSI reference target");
MODULE_SUPPORTED_DEVICE("sd");
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif

/* initializor for static Scsi_Target_Template */

#define ISCSI {\
  name: "iscsi_target", \
  detect:iscsi_detect, \
  release:iscsi_release,\
  xmit_response:iscsi_xmit_response, \
  rdy_to_xfer:iscsi_rdy_to_xfer,\
  proc_info:iscsi_proc_info, \
  task_mgmt_fn_done:iscsi_task_mgt_fn_done \
};

Scsi_Target_Template my_template = ISCSI;

#include "scsi_target_module.c"

#endif
