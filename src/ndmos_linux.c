/*
 * Copyright (c) 1998,1999,2000
 *	Traakan, Inc., Los Altos, CA
 *	All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Project:  NDMJOB
 * Ident:    $Id: ndmos_linux.c,v 1.1.1.1 2003/10/14 19:12:41 ern Exp $
 *
 * Description:
 *	This contains the O/S (Operating System) specific
 *	portions of NDMJOBLIB for the FreeBSD platform.
 *
 *	This file is #include'd by ndmos.c when
 *	selected by #ifdef's of NDMOS_ID.
 *
 *	There are four major portions:
 *	1) Misc support routines: password check, local info, etc
 *	2) Non-blocking I/O support routines
 *	3) Tape interfacs ndmos_tape_xxx()
 *	4) OS Specific NDMP request dispatcher which intercepts
 *	   requests implemented here, such as SCSI operations
 *	   and system configuration queries.
 */




/*
 * #include "ndmagents.h" already done in ndmos.c
 */

/*
 * Additional #include's, not needed in ndmos_freebsd.h, yet needed here.
 */
#include <sys/utsname.h>



/*
 * These pre-processor symbols surround blocks of code below.
 * This let's you enable/disable these blocks. It also
 * helps illuminate portions as related.
 */
#undef I_HAVE_DISPATCH_REQUEST
#undef I_HAVE_TAPE_SUPPORT
#undef I_HAVE_SCSI_SUPPORT




/*
 * Misc Support Routines
 ****************************************************************
 */

/*
 * Determine whether the clear-text account name and password
 * are valid. Supports NDMPx_CONNECT_CLIENT_AUTH requests.
 */
int
ndmos_ok_name_password (struct ndm_session *sess, char *name, char *pass)
{
	if (strcmp (name, "ndmp") != 0)
		return 0;

	if (strcmp (pass, "ndmp") != 0)
		return 0;

	return 1;	/* OK */
}

/*
 * Get local info. Supports NDMPx_CONFIG_GET_HOST_INFO and
 * NDMP3_CONFIG_GET_SERVER_INFO.
 */
void
ndmos_get_local_info (struct ndm_session *sess)
{
	static struct utsname	unam;
	static char		idbuf[30];
	static char		revbuf[50];
	char			obuf[5];

	obuf[0] = (char)(NDMOS_ID >> 24);
	obuf[1] = (char)(NDMOS_ID >> 16);
	obuf[2] = (char)(NDMOS_ID >> 8);
	obuf[3] = (char)(NDMOS_ID >> 0);
	obuf[4] = 0;

	uname (&unam);
	sprintf (idbuf, "%lu", gethostid());

	sess->local_info.hostname = unam.nodename;
	sess->local_info.os_type = unam.sysname;
	sess->local_info.os_vers = unam.release;
	sess->local_info.hostid = idbuf;

	sess->local_info.vendor_name = NDMOS_CONST_VENDOR_NAME;
	sess->local_info.product_name = NDMOS_CONST_PRODUCT_NAME;

	sprintf (revbuf, "%s LIB:%d.%d/%s OS:%s (%s)",
		NDMOS_CONST_PRODUCT_REVISION,
		NDMJOBLIB_VERSION, NDMJOBLIB_RELEASE,
		NDMOS_CONST_NDMJOBLIB_REVISION,
		NDMOS_CONST_NDMOS_REVISION,
		obuf);

	sess->local_info.revision_number = revbuf;
}

#ifndef NDMOS_OPTION_NO_ROBOT_AGENT
/*
 * Refresh the SCSI state. This is called from various places
 * in preparation for examining the state.
 * This is mandatory even if you don't have SCSI support.
 */
#ifndef I_HAVE_SCSI
/* Keep it simple when no SCSI */
void
ndmos_scsi_sync_state (struct ndm_session *sess)
{
	sess->robot_acb.scsi_state.error = NDMP9_DEV_NOT_OPEN_ERR;
}

#else /* !I_HAVE_SCSI */

void
ndmos_scsi_sync_state (struct ndm_session *sess)
{
	sess->robot_acb.scsi_state.error = NDMP9_DEV_NOT_OPEN_ERR;
}

#endif /* !I_HAVE_SCSI */
#endif /* !NDMOS_OPTION_NO_ROBOT_AGENT */




/*
 * MD5 authentication support
 ****************************************************************
 * See ndml_md5.c
 */

int
ndmos_get_md5_challenge (struct ndm_session *sess)
{
	ndmmd5_generate_challenge (sess->md5_challenge);
	sess->md5_challenge_valid = 1;
	return 0;
}

int
ndmos_ok_name_md5_digest (struct ndm_session *sess,
  char *name, char digest[16])
{
	if (strcmp (name, "ndmp") != 0)
		return 0;

	if (!ndmmd5_ok_digest (sess->md5_challenge, "ndmp", digest))
		return 0;

	return 1;	/* OK */
}




/*
 * NON-BLOCKING I/O SUPPORT
 ****************************************************************
 * As support non-blocking I/O for NDMCHAN, condition different
 * types of file descriptors to not block.
 */

void
ndmos_condition_listen_socket (struct ndm_session *sess, int sock)
{
	int		flag;

	flag = 1;
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag);
}

void
ndmos_condition_control_socket (struct ndm_session *sess, int sock)
{
	/* nothing */
}

void
ndmos_condition_image_stream_socket (struct ndm_session *sess, int sock)
{
	fcntl (sock, F_SETFL, O_NONBLOCK);
	signal (SIGPIPE, SIG_IGN);
}

void
ndmos_condition_pipe_fd (struct ndm_session *sess, int fd)
{
	fcntl (fd, F_SETFL, O_NONBLOCK);
	signal (SIGPIPE, SIG_IGN);
}






#ifndef NDMOS_OPTION_NO_TAPE_AGENT
#ifndef NDMOS_OPTION_TAPE_SIMULATOR
/*
 * TAPE INTERFACE
 ****************************************************************
 * These interface to the O/S specific tape drivers and subsystem.
 * They must result in functionality equivalent to the reference
 * tape simulator. The NDMP TAPE model is demanding, and it is
 * often necessary to workaround the native device driver(s)
 * to achieve NDMP TAPE model conformance.
 *
 * It's easy to test this ndmos_tape_xxx() implementation
 * using the ndmjob(1) command in test-tape conformance mode.
 * The tape simulator passes this test. To test this implementation,
 * rebuild ndmjob, then use this command:
 *
 *	ndmjob -o test-tape -T. -f /dev/whatever
 *
 * These ndmos_tape_xxx() interfaces must maintain the tape state
 * (sess->tape_agent.tape_state). In particular, the position
 * information (file_num and blockno) must be accurate at all
 * times. A typical workaround is to maintain these here rather
 * than relying on the native device drivers. Another workaround
 * is to implement NDMP MTIO operations using repeated native MTIO
 * operations with count=1, then interpret the results and errors
 * to maintain accurate position and residual information.
 *
 * Workarounds in this implementation (please keep this updated):
 *
 */

#ifdef I_HAVE_TAPE_SUPPORT

void
ndmos_tape_sync_state (struct ndm_session *sess)
{
	the_tape_state.error = NDMP9_DEV_NOT_OPEN_ERR;
}

ndmp9_error
ndmos_tape_open (struct ndm_session *sess, char *name, int will_write)
{
	return NDMP9_NOT_SUPPORTED_ERR;
}

ndmp9_error
ndmos_tape_close (struct ndm_session *sess)
{
	return NDMP9_NOT_SUPPORTED_ERR;
}

ndmp9_error
ndmos_tape_write (struct ndm_session *sess, char *data,
  unsigned long count, unsigned long * done_count)
{
	return NDMP9_NOT_SUPPORTED_ERR;
}

ndmp9_error
ndmos_tape_read (struct ndm_session *sess, char *data,
  unsigned long count, unsigned long * done_count)
{
	return NDMP9_NOT_SUPPORTED_ERR;
}

ndmp9_error
ndmos_tape_mtio (struct ndm_session *sess, ndmp9_tape_mtio_op op,
  unsigned long count, unsigned long * done_count)
{
	return NDMP9_NOT_SUPPORTED_ERR;
}

#endif /* I_HAVE_TAPE_SUPPORT */
#endif /* !NDMOS_OPTION_TAPE_SIMULATOR */
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */




/*
 * ndmos_dispatch_request() -- O/S Specific Agent Dispatch Request (ADR)
 ****************************************************************
 * Some NDMP requests can only be handled as O/S specific portions,
 * and are implemented here.
 *
 * The more generic NDMP requests may be re-implemented here rather
 * than modifying the main body of code. Extensions to the NDMP protocol
 * may also be implemented here. Neither is ever a good idea beyond
 * experimentation. The structures in ndmagents.h provide for O/S
 * specific extensions. Such extensions are #define'd in the ndmos_freebsd.h.
 *
 * The return value from ndmos_dispatch_request() tells the main
 * dispatcher, ndma_dispatch_request(), whether or not the request
 * was intercepted. ndmos_dispatch_request() is called after basic
 * reply setup is done (message headers and buffers), but before
 * the request is interpretted.
 */

#ifndef I_HAVE_DISPATCH_REQUEST

/*
 * If we're not intercepting, keep it simple
 */
int
ndmos_dispatch_request (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
	return -1;	/* not intercepted */
}

#else /* !I_HAVE_DISPATCH_REQUEST */

extern struct ndm_dispatch_version_table ndmos_dispatch_version_table[];

int
ndmos_dispatch_request (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
	struct ndm_dispatch_request_table *drt;
	unsigned	protocol_version = ref_conn->protocol_version;
	unsigned	msg = xa->request.header.message;
	int		rc;

	drt = ndma_drt_lookup (ndmos_dispatch_version_table,
					protocol_version, msg);

	if (!drt) {
		return -1;	/* not intercepted */
	}

	/*
	 * Replicate the ndma_dispatch_request() permission checks
	 */
	if (!sess->conn_open
	 && !(drt->flags & NDM_DRT_FLAG_OK_NOT_CONNECTED)) {
		xa->reply.header.error = NDMP0_PERMISSION_ERR;
		return 0;
	}

	if (!sess->conn_authorized
	 && !(drt->flags & NDM_DRT_FLAG_OK_NOT_AUTHORIZED)) {
		xa->reply.header.error = NDMP9_NOT_AUTHORIZED_ERR;
		return 0;
	}

	rc = (*drt->dispatch_request)(sess, xa, ref_conn);

	if (rc < 0) {
		xa->reply.header.error = NDMP0_NOT_SUPPORTED_ERR;
	}

	return 0;
}




/*
 * NDMPx_CONFIG Interfaces
 ****************************************************************
 */



#ifndef NDMOS_OPTION_NO_NDMP3		/* Surrounds NDMPv3 CONFIG intfs */
/*
 * NDMP[23]_CONFIG_GET_FS_INFO
 *
 * This can only be implemented here as part of the O/S specific
 * module. There is absolutely no way to implement this generically,
 * nor is there merit to a generic "layer".
 */
int
ndmos_adr_config_get_fs_info (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_config_get_fs_info)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
    }
    return 0;
}




/*
 * NDMP[23]_CONFIG_GET_TAPE_INFO
 *
 * This can only be implemented here as part of the O/S specific
 * module. There is absolutely no way to implement this generically,
 * nor is there merit to a generic "layer".
 */
int
ndmos_adr_config_get_tape_info (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_config_get_tape_info)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
    }
    return 0;
}




/*
 * NDMP[23]_CONFIG_GET_SCSI_INFO
 *
 * This can only be implemented here as part of the O/S specific
 * module. There is absolutely no way to implement this generically,
 * nor is there merit to a generic "layer".
 */
int
ndmos_adr_config_get_scsi_info (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_config_get_scsi_info)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
    }
    return 0;
}
#endif /* !NDMOS_OPTION_NO_NDMP3 */	/* Surrounds NDMPv3 CONFIG intfs */




#ifndef NDMOS_OPTION_NO_ROBOT_AGENT	/* Surrounds all SCSI intfs */
#ifdef I_HAVE_SCSI_SUPPORT
/*
 * NDMPx_SCSI Interfaces
 ****************************************************************
 * These can only be implemented here as part of the O/S specific
 * module. There is absolutely no way to implement this generically,
 * nor is there merit to a generic "layer".
 */




/*
 * NDMP[23]_SCSI_OPEN
 */
int
ndmos_adr_scsi_open (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_scsi_open)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_scsi_open)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
    }
    return 0;
}




/*
 * NDMP[23]_SCSI_CLOSE
 */
int
ndmos_adr_scsi_close (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_scsi_close)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_scsi_close)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
    }
    return 0;
}




/*
 * NDMP[23]_SCSI_GET_STATE
 */
int
ndmos_adr_scsi_get_state (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_scsi_get_state)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_scsi_get_state)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
    }
    return 0;
}




/*
 * NDMP[23]_SCSI_SET_TARGET
 */
int
ndmos_adr_scsi_set_target (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_scsi_set_target)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_scsi_set_target)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
    }
    return 0;
}




/*
 * NDMP[23]_SCSI_RESET_DEVICE
 */
int
ndmos_adr_scsi_reset_device (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_scsi_reset_device)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_scsi_reset_device)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
    }
    return 0;
}




/*
 * NDMP[23]_SCSI_RESET_BUS
 */
int
ndmos_adr_scsi_reset_bus (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_scsi_reset_bus)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_scsi_reset_bus)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
    }
    return 0;
}




/*
 * NDMP[23]_SCSI_EXECUTE_CDB
 */
int
ndmos_adr_scsi_execute_cdb (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_scsi_execute_cdb)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_scsi_execute_cdb)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
    }
    return 0;
}
#endif /* I_HAVE_SCSI_SUPPORT */
#endif /* !NDMOS_OPTION_NO_ROBOT_AGENT */	/* Surrounds all SCSI intfs */




#ifndef NDMOS_OPTION_NO_TAPE_AGENT		/* Surrounds all TAPE intfs */
#ifdef I_HAVE_SCSI_SUPPORT
/*
 * NDMPx_TAPE Interfaces
 ****************************************************************
 */




/*
 * NDMP[23]_TAPE_EXECUTE_CDB
 *
 * This can only be implemented here as part of the O/S specific
 * module. There is absolutely no way to implement this generically,
 * nor is there merit to a generic "layer".
 */
int
ndmos_adr_tape_execute_cdb (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_tape_execute_cdb)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_tape_execute_cdb)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
    }
    return 0;
}
#endif /* I_HAVE_SCSI_SUPPORT */
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */	/* Surrounds all TAPE intfs */




/*
 * Dispatch Version Table and Dispatch Request Tables (DVT/DRT)
 ****************************************************************
 */

struct ndm_dispatch_request_table ndmos_dispatch_request_table_v0[] = {
   {0}
};

#ifndef NDMOS_OPTION_NO_NDMP2
struct ndm_dispatch_request_table ndmos_dispatch_request_table_v2[] = {

#ifndef NDMOS_OPTION_NO_ROBOT_AGENT		/* Surrounds all SCSI intfs */
#ifdef I_HAVE_SCSI_SUPPORT
   { NDMP2_SCSI_OPEN,			0, ndmos_adr_scsi_open },
   { NDMP2_SCSI_CLOSE,			0, ndmos_adr_scsi_close },
   { NDMP2_SCSI_GET_STATE,		0, ndmos_adr_scsi_get_state },
   { NDMP2_SCSI_SET_TARGET,		0, ndmos_adr_scsi_set_target },
   { NDMP2_SCSI_RESET_DEVICE,		0, ndmos_adr_scsi_reset_device },
   { NDMP2_SCSI_RESET_BUS,		0, ndmos_adr_scsi_reset_bus },
   { NDMP2_SCSI_EXECUTE_CDB,		0, ndmos_adr_scsi_execute_cdb },
#endif /* I_HAVE_SCSI_SUPPORT */
#endif /* !NDMOS_OPTION_NO_ROBOT_AGENT */	/* Surrounds all SCSI intfs */

#ifndef NDMOS_OPTION_NO_TAPE_AGENT		/* Surrounds all TAPE intfs */
#ifdef I_HAVE_SCSI_SUPPORT
   { NDMP2_TAPE_EXECUTE_CDB,		0, ndmos_adr_tape_execute_cdb },
#endif /* I_HAVE_SCSI_SUPPORT */
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */	/* Surrounds all TAPE intfs */

   {0}
};
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
struct ndm_dispatch_request_table ndmos_dispatch_request_table_v3[] = {
   { NDMP3_CONFIG_GET_FS_INFO,		0, ndmos_adr_config_get_fs_info },
   { NDMP3_CONFIG_GET_TAPE_INFO,	0, ndmos_adr_config_get_tape_info },
   { NDMP3_CONFIG_GET_SCSI_INFO,	0, ndmos_adr_config_get_scsi_info },

#ifndef NDMOS_OPTION_NO_ROBOT_AGENT		/* Surrounds all SCSI intfs */
#ifdef I_HAVE_SCSI_SUPPORT
   { NDMP3_SCSI_OPEN,			0, ndmos_adr_scsi_open },
   { NDMP3_SCSI_CLOSE,			0, ndmos_adr_scsi_close },
   { NDMP3_SCSI_GET_STATE,		0, ndmos_adr_scsi_get_state },
   { NDMP3_SCSI_SET_TARGET,		0, ndmos_adr_scsi_set_target },
   { NDMP3_SCSI_RESET_DEVICE,		0, ndmos_adr_scsi_reset_device },
   { NDMP3_SCSI_RESET_BUS,		0, ndmos_adr_scsi_reset_bus },
   { NDMP3_SCSI_EXECUTE_CDB,		0, ndmos_adr_scsi_execute_cdb },
#endif /* I_HAVE_SCSI_SUPPORT */
#endif /* !NDMOS_OPTION_NO_ROBOT_AGENT */	/* Surrounds all SCSI intfs */

#ifndef NDMOS_OPTION_NO_TAPE_AGENT		/* Surrounds all TAPE intfs */
#ifdef I_HAVE_SCSI_SUPPORT
   { NDMP3_TAPE_EXECUTE_CDB,		0, ndmos_adr_tape_execute_cdb },
#endif /* I_HAVE_SCSI_SUPPORT */
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */	/* Surrounds all TAPE intfs */

   {0}
};
#endif /* !NDMOS_OPTION_NO_NDMP3 */


struct ndm_dispatch_version_table ndmos_dispatch_version_table[] = {
	{ 0,		ndmos_dispatch_request_table_v0 },
#ifndef NDMOS_OPTION_NO_NDMP2
	{ NDMP2VER,	ndmos_dispatch_request_table_v2 },
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	{ NDMP3VER,	ndmos_dispatch_request_table_v3 },
#endif /* !NDMOS_OPTION_NO_NDMP2 */
	{ -1 }
};
#endif /* !I_HAVE_DISPATCH_REQUEST */

