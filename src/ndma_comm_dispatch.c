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
 * Ident:    $Id: ndma_comm_dispatch.c,v 1.1 2003/10/14 19:12:38 ern Exp $
 *
 * Description:
 *	Think of ndma_dispatch_request() as a parser (like yacc(1) input).
 *	This parses (audits) the sequence of requests. If the requests
 *	conform to the "grammar", semantic actions are taken.
 *
 *	This is, admittedly, a huge source file. The idea
 *	is to have all audits and associated errors here. This
 *	makes review, study, comparing to the specification, and
 *	discussion easier because we don't get balled up in
 *	the implementation of semantics. Further, with the hope
 *	of wide-scale deployment, revisions of this source file
 *	can readily be integrated into derivative works without
 *	disturbing other portions.
 */


#include "ndmagents.h"


extern struct ndm_dispatch_version_table ndma_dispatch_version_table[];

static int		connect_open_common (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn,
				int protocol_version);



int
ndma_dispatch_request (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
	struct ndm_dispatch_request_table *drt;
	unsigned	protocol_version = ref_conn->protocol_version;
	unsigned	msg = xa->request.header.message;
	int		rc;

	NDMOS_MACRO_ZEROFILL (&xa->reply);

	xa->reply.protocol_version = xa->request.protocol_version;
	xa->reply.flags |= NDMNMB_FLAG_NO_FREE;

	xa->reply.header.sequence = 0;		/* filled-in by xmit logic */
	xa->reply.header.time_stamp = 0;	/* filled-in by xmit logic */
	xa->reply.header.message_type = NDMP0_MESSAGE_REPLY;
	xa->reply.header.message = xa->request.header.message;
	xa->reply.header.reply_sequence = xa->request.header.sequence;
	xa->reply.header.error = NDMP0_NO_ERR;

	/* assume no error */
	ndmnmb_set_reply_error_raw (&xa->reply, NDMP0_NO_ERR);

	/* sanity check */
	if (xa->request.protocol_version != protocol_version) {
		xa->reply.header.error = NDMP0_UNDEFINED_ERR;
		return 0;
	}

	/*
	 * If the session is not open and the message
	 * is anything other than CONNECT_OPEN, the client
	 * has implicitly agreed to the protocol_version
	 * offered by NOTIFY_CONNECTED (ref ndmconn_accept()).
	 * Effectively perform CONNECT_OPEN for that
	 * protocol_version.
	 */
	if (!sess->conn_open && msg != NDMP0_CONNECT_OPEN) {
		connect_open_common (sess, xa, ref_conn,
					ref_conn->protocol_version);
	}

	rc = ndmos_dispatch_request (sess, xa, ref_conn);
	if (rc >= 0)
		return rc;	/* request intercepted */

	drt = ndma_drt_lookup (ndma_dispatch_version_table,
					protocol_version, msg);

	if (!drt) {
		xa->reply.header.error = NDMP0_NOT_SUPPORTED_ERR;
		return 0;
	}

	if (!sess->conn_open
	 && !(drt->flags & NDM_DRT_FLAG_OK_NOT_CONNECTED)) {
		xa->reply.header.error = NDMP0_PERMISSION_ERR;
		return 0;
	}

	if (!sess->conn_authorized
	 && !(drt->flags & NDM_DRT_FLAG_OK_NOT_AUTHORIZED)) {
		xa->reply.header.error = NDMP0_NOT_AUTHORIZED_ERR;
		return 0;
	}

	rc = (*drt->dispatch_request)(sess, xa, ref_conn);

	if (rc < 0) {
		xa->reply.header.error = NDMP0_NOT_SUPPORTED_ERR;
	}

	return 0;
}

int
ndma_dispatch_raise_error (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  ndmp9_error error, char *errstr)
{
	int			protocol_version = ref_conn->protocol_version;
	ndmp0_message		msg = xa->request.header.message;

	if (errstr) {
		ndmalogf (sess, 0, 2, "op=%s err=%s why=%s",
			ndmp_message_to_str (protocol_version, msg),
			ndmp9_error_to_str (error),
			errstr);
	}

	ndmnmb_set_reply_error (&xa->reply, error);

	return 1;
}




/*
 * Access paths to ndma_dispatch_request()
 ****************************************************************
 */

/* incomming requests on a ndmconn connection */
int
ndma_dispatch_conn (struct ndm_session *sess, struct ndmconn *conn)
{
	struct ndmp_xa_buf	xa;
	int			rc;

	NDMOS_MACRO_ZEROFILL (&xa);

	rc = ndmconn_recv_nmb (conn, &xa.request);
	if (rc) return rc;

	ndma_dispatch_request (sess, &xa, conn);

	if (! (xa.reply.flags & NDMNMB_FLAG_NO_SEND)) {
		rc = ndmconn_send_nmb (conn, &xa.reply);
		if (rc) return rc;
	}

	ndmnmb_free (&xa.reply);

	return 0;
}

void
ndma_dispatch_ctrl_unexpected (struct ndmconn *conn, struct ndmp_msg_buf *nmb)
{
	int			protocol_version = conn->protocol_version;
	struct ndm_session *	sess = conn->context;
	struct ndmp_xa_buf	xa;

	if (nmb->header.message_type != NDMP0_MESSAGE_REQUEST) {
		ndmalogf (sess, conn->chan.name, 1, "Unexpected message");
		ndmnmb_free (nmb);
		return;
	}

	NDMOS_MACRO_ZEROFILL (&xa);
	xa.request = *nmb;

	ndmalogf (sess, conn->chan.name, 4, "Async request %s",
			ndmp_message_to_str (protocol_version,
				xa.request.header.message));

	ndma_dispatch_request (sess, &xa, conn);

	if (! (xa.reply.flags & NDMNMB_FLAG_NO_SEND)) {
		ndmconn_send_nmb (conn, &xa.reply);
	}

	ndmnmb_free (&xa.reply);
}



int
ndma_call_no_tattle (struct ndmconn *conn, struct ndmp_xa_buf *xa)
{
	int		rc;

	if (conn->conn_type == NDMCONN_TYPE_RESIDENT) {
		struct ndm_session *sess = conn->context;

		conn->last_message = xa->request.header.message;
		conn->last_call_status = NDMCONN_CALL_STATUS_BOTCH;
		conn->last_header_error = -1;	/* invalid */
		conn->last_reply_error = -1;	/* invalid */

		xa->request.header.sequence = conn->next_sequence++;

		ndmconn_snoop_nmb (conn, &xa->request, "Send");

		rc = ndma_dispatch_request (sess, xa, conn);

		xa->reply.header.sequence = conn->next_sequence++;

		if (! (xa->reply.flags & NDMNMB_FLAG_NO_SEND))
			ndmconn_snoop_nmb (conn, &xa->reply, "Recv");
		if (rc) {
			/* keep it */
		} else if (xa->reply.header.error != NDMP0_NO_ERR) {
			rc = NDMCONN_CALL_STATUS_HDR_ERROR;
			conn->last_header_error = xa->reply.header.error;
		} else {
			conn->last_header_error = NDMP9_NO_ERR;
			conn->last_reply_error =
					ndmnmb_get_reply_error (&xa->reply);

			if (conn->last_reply_error == NDMP9_NO_ERR) {
				rc = NDMCONN_CALL_STATUS_OK;
			} else {
				rc = NDMCONN_CALL_STATUS_REPLY_ERROR;
			}
		}
	} else {
		rc = ndmconn_call (conn, xa);
	}

	return rc;
}

int
ndma_call (struct ndmconn *conn, struct ndmp_xa_buf *xa)
{
	int		rc;

	rc = ndma_call_no_tattle (conn, xa);

	if (rc) {
		ndma_tattle (conn, xa, rc);
	}
	return rc;
}

int
ndma_send_to_control (struct ndm_session *sess, struct ndmp_xa_buf *xa,
  struct ndmconn *from_conn)
{
	struct ndmconn *	conn = sess->plumb.control;
	int			rc;

	if (conn->conn_type == NDMCONN_TYPE_RESIDENT && from_conn) {
		/*
		 * Control and sending agent are
		 * resident. Substitute the sending
		 * agents "connection" so that logs
		 * look right and right protocol_version
		 * is used.
		 */
		conn = from_conn;
	}

	rc = ndma_call_no_tattle (conn, xa);

	if (rc) {
		ndma_tattle (conn, xa, rc);
	}
	return rc;
}

int
ndma_tattle (struct ndmconn *conn, struct ndmp_xa_buf *xa, int rc)
{
	struct ndm_session *sess = conn->context;
	int		protocol_version = conn->protocol_version;
	unsigned	msg = xa->request.header.message;
	char *		tag = conn->chan.name;
	char *		msgname = ndmp_message_to_str (protocol_version, msg);
	unsigned	err;

	switch (rc) {
	case 0:
		ndmalogf (sess, tag, 2, " ?OK %s", msgname);
		break;

	case 1:	/* no error in header, error in reply */
		err = ndmnmb_get_reply_error_raw (&xa->reply);
		ndmalogf (sess, tag, 2, " ERR %s  %s",
			msgname,
			ndmp_error_to_str (protocol_version, err));
		break;

	case -2: /* error in header, no reply body */
		err = xa->reply.header.error;
		ndmalogf (sess, tag, 2, " ERR-AGENT %s  %s",
			msgname,
			ndmp_error_to_str (protocol_version, err));
		break;

	default:
		ndmalogf (sess, tag, 2, " ERR-CONN %s  %s",
			msgname,
			ndmconn_get_err_msg (conn));
		break;
	}

	return 0;
}




/*
 * NDMPx_CONNECT Interfaces
 ****************************************************************
 */

static int	connect_client_auth_common234 (struct ndm_session *sess,
		   struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
		   ndmp9_auth_type auth_type, char *name, char *proof);




/*
 * NDMP[023]_CONNECT_OPEN
 */
int
ndmadr_connect_open (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    NDMS_WITH(ndmp0_connect_open)
	if (sess->conn_open) {
	    if (request->protocol_version != ref_conn->protocol_version) {
		NDMADR_RAISE_ILLEGAL_ARGS("too late to change version");
	    }
	} else {
	    switch (request->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	    case NDMP2VER:
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	    case NDMP3VER:
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	    case NDMP4VER:
#endif /* !NDMOS_OPTION_NO_NDMP4 */
		connect_open_common (sess, xa, ref_conn,
					request->protocol_version);
		break;

	    default:
		NDMADR_RAISE_ILLEGAL_ARGS("unsupport protocol version");
		break;
	    }
	}
    NDMS_ENDWITH

    return 0;
}

static int
connect_open_common (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  int protocol_version)
{
#ifndef NDMOS_OPTION_NO_DATA_AGENT
	sess->data_acb.protocol_version = protocol_version;
#endif /* !NDMOS_OPTION_NO_DATA_AGENT */
#ifndef NDMOS_OPTION_NO_TAPE_AGENT
	sess->tape_acb.protocol_version = protocol_version;
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */
#ifndef NDMOS_OPTION_NO_ROBOT_AGENT
	sess->robot_acb.protocol_version = protocol_version;
#endif /* !NDMOS_OPTION_NO_ROBOT_AGENT */

	ref_conn->protocol_version = protocol_version;
	sess->conn_open = 1;

	return 0;
}




/*
 * NDMP[23]_CONNECT_CLIENT_AUTH
 */
int
ndmadr_connect_client_auth (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    ndmp9_auth_type	auth_type;
    char *		name = 0;
    char *		proof = 0;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_connect_client_auth)
	switch (request->auth_data.auth_type) {
	default:	auth_type = -1; break;

	case NDMP2_AUTH_TEXT:
	    {
		ndmp2_auth_text *	p;

		p = &request->auth_data.ndmp2_auth_data_u.auth_text;
		name = p->auth_id;
		proof = p->auth_password;
		auth_type = NDMP9_AUTH_TEXT;
	    }
	    break;

	case NDMP2_AUTH_MD5:
	    {
		ndmp2_auth_md5 *	p;

		p = &request->auth_data.ndmp2_auth_data_u.auth_md5;
		name = p->auth_id;
		proof = p->auth_digest;
		auth_type = NDMP9_AUTH_MD5;
	    }
	    break;
	}
	return connect_client_auth_common234 (sess, xa, ref_conn,
						auth_type, name, proof);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_connect_client_auth)
	switch (request->auth_data.auth_type) {
	default:	auth_type = -1; break;

	case NDMP3_AUTH_TEXT:
	    {
		ndmp3_auth_text *	p;

		p = &request->auth_data.ndmp3_auth_data_u.auth_text;
		name = p->auth_id;
		proof = p->auth_password;
		auth_type = NDMP9_AUTH_TEXT;
	    }
	    break;

	case NDMP3_AUTH_MD5:
	    {
		ndmp3_auth_md5 *	p;

		p = &request->auth_data.ndmp3_auth_data_u.auth_md5;
		name = p->auth_id;
		proof = p->auth_digest;
		auth_type = NDMP9_AUTH_MD5;
	    }
	    break;
	}
	return connect_client_auth_common234 (sess, xa, ref_conn,
						auth_type, name, proof);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_connect_client_auth)
	switch (request->auth_data.auth_type) {
	default:	auth_type = -1; break;

	case NDMP4_AUTH_TEXT:
	    {
		ndmp4_auth_text *	p;

		p = &request->auth_data.ndmp4_auth_data_u.auth_text;
		name = p->auth_id;
		proof = p->auth_password;
		auth_type = NDMP9_AUTH_TEXT;
	    }
	    break;

	case NDMP4_AUTH_MD5:
	    {
		ndmp4_auth_md5 *	p;

		p = &request->auth_data.ndmp4_auth_data_u.auth_md5;
		name = p->auth_id;
		proof = p->auth_digest;
		auth_type = NDMP9_AUTH_MD5;
	    }
	    break;
	}
	return connect_client_auth_common234 (sess, xa, ref_conn,
						auth_type, name, proof);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

static int
connect_client_auth_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  ndmp9_auth_type auth_type, char *name, char *proof)
{
	switch (auth_type) {
	default:	NDMADR_RAISE_ILLEGAL_ARGS ("auth_type");

	case NDMP9_AUTH_TEXT:
		if (!ndmos_ok_name_password (sess, name, proof)) {
			NDMADR_RAISE(NDMP9_NOT_AUTHORIZED_ERR,
						"password not OK");
		}
		break;

	case NDMP9_AUTH_MD5:
		if (!sess->md5_challenge_valid) {
			NDMADR_RAISE(NDMP9_NOT_AUTHORIZED_ERR, "no challenge");
		}

		if (!ndmos_ok_name_md5_digest (sess, name, proof)) {
			NDMADR_RAISE(NDMP9_NOT_AUTHORIZED_ERR,
						"digest not OK");
		}
		break;
	}
	sess->conn_authorized = 1;

	return 0;
}




/*
 * NDMP[023]_CONNECT_CLOSE
 */
int
ndmadr_connect_close (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;	/* ??? */
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	/* TODO: shutdown everything */
	sess->connect_status = 0;
        break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
	/* TODO: shutdown everything */
	sess->connect_status = 0;
        break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
	/* TODO: shutdown everything */
	sess->connect_status = 0;
        break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[23]_CONNECT_SERVER_AUTH
 */
int
ndmadr_connect_server_auth (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_connect_server_auth)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_connect_server_auth)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_connect_server_auth)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMPx_CONFIG Interfaces
 ****************************************************************
 */




/*
 * NDMP[23]_CONFIG_GET_HOST_INFO
 */
int
ndmadr_config_get_host_info (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_config_get_host_info)
	static ndmp2_auth_type auth_type_val[] = {
				NDMP2_AUTH_TEXT, NDMP2_AUTH_MD5 };

	ndmos_get_local_info(sess);
	reply->hostname    = sess->local_info.hostname;
	reply->os_type     = sess->local_info.os_type;
	reply->os_vers     = sess->local_info.os_vers;
	reply->hostid      = sess->local_info.hostid;
	reply->auth_type.auth_type_len = 2;
	reply->auth_type.auth_type_val = auth_type_val;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_config_get_host_info)
	ndmos_get_local_info(sess);
	reply->hostname    = sess->local_info.hostname;
	reply->os_type     = sess->local_info.os_type;
	reply->os_vers     = sess->local_info.os_vers;
	reply->hostid      = sess->local_info.hostid;
	/* NDMPv3 moved auth_type to NDMP3_CONFIG_GET_SERVER_INFO */
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_config_get_host_info)
	ndmos_get_local_info(sess);
	reply->hostname    = sess->local_info.hostname;
	reply->os_type     = sess->local_info.os_type;
	reply->os_vers     = sess->local_info.os_vers;
	reply->hostid      = sess->local_info.hostid;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




#ifndef NDMOS_OPTION_NO_NDMP2
/*
 * NDMP2_CONFIG_GET_BUTYPE_ATTR
 */
int
ndmadr_config_get_butype_attr (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_data_butype *	bt;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_config_get_butype_attr)
	bt = ndmda_find_butype_by_name (request->name);
	if (!bt)
		NDMADR_RAISE_ILLEGAL_ARGS("'butype'");
	if ((*bt->config_get_attrs) (sess, &reply->attrs, NDMP2VER) != 0)
		NDMADR_RAISE_ILLEGAL_ARGS("config_get_attrs");
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
	/* not part of NDMPv3 */
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
	/* not part of NDMPv4 */
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}
#endif /* !NDMOS_OPTION_NO_NDMP2 */




/*
 * NDMP2_CONFIG_GET_MOVER_TYPE
 * NDMP[34]_CONFIG_GET_CONNECTION_TYPE
 */
int
ndmadr_config_get_connection_type (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_config_get_mover_type)
	static ndmp2_mover_addr_type methods_val[] = {
					 NDMP2_ADDR_LOCAL,NDMP2_ADDR_TCP };
	reply->methods.methods_len = 2;
	reply->methods.methods_val = methods_val;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_config_get_connection_type)
	static ndmp3_addr_type addr_types[] = {
					 NDMP3_ADDR_LOCAL,NDMP3_ADDR_TCP };
	reply->addr_types.addr_types_len = 2;
	reply->addr_types.addr_types_val = addr_types;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_config_get_connection_type)
	static ndmp4_addr_type addr_types[] = {
					 NDMP4_ADDR_LOCAL,NDMP4_ADDR_TCP };
	reply->addr_types.addr_types_len = 2;
	reply->addr_types.addr_types_val = addr_types;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_CONFIG_GET_AUTH_ATTR
 *
 * Credits to Rajiv of NetApp for helping with MD5 stuff.
 */
int
ndmadr_config_get_auth_attr (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_config_get_auth_attr)
	switch (request->auth_type) {
	default:
		NDMADR_RAISE_ILLEGAL_ARGS ("auth_type");

	case NDMP2_AUTH_NONE:
		break;

	case NDMP2_AUTH_TEXT:
		break;

	case NDMP2_AUTH_MD5:
		ndmos_get_md5_challenge (sess);
		NDMOS_API_BCOPY (sess->md5_challenge,
			reply->server_attr.ndmp2_auth_attr_u.challenge, 64);
		break;
	}
	reply->server_attr.auth_type = request->auth_type;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_config_get_auth_attr)
	switch (request->auth_type) {
	default:
		NDMADR_RAISE_ILLEGAL_ARGS ("auth_type");

	case NDMP3_AUTH_NONE:
		break;

	case NDMP3_AUTH_TEXT:
		break;

	case NDMP3_AUTH_MD5:
		ndmos_get_md5_challenge (sess);
		NDMOS_API_BCOPY (sess->md5_challenge,
			reply->server_attr.ndmp3_auth_attr_u.challenge,	64);
		break;
	}
	reply->server_attr.auth_type = request->auth_type;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_config_get_auth_attr)
	switch (request->auth_type) {
	default:
		NDMADR_RAISE_ILLEGAL_ARGS ("auth_type");

	case NDMP4_AUTH_NONE:
		break;

	case NDMP4_AUTH_TEXT:
		break;

	case NDMP4_AUTH_MD5:
		ndmos_get_md5_challenge (sess);
		NDMOS_API_BCOPY (sess->md5_challenge,
			reply->server_attr.ndmp4_auth_attr_u.challenge,	64);
		break;
	}
	reply->server_attr.auth_type = request->auth_type;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




#ifndef NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4	/* Surrounds NDMPv[34] CONFIG intfs */
/*
 * NDMP[34]_CONFIG_GET_BUTYPE_INFO
 */
int
ndmadr_config_get_butype_info (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	/* not part of NDMPv2 */
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_config_get_butype_info)
	static ndmp3_pval		default_env_tab[3];
	static ndmp3_butype_info	butype_info_tar;
	ndmp3_pval *			evp;

	evp = default_env_tab;
	evp->name = "TYPE";
	evp->value = "tar";
	evp++;

	butype_info_tar.butype_name = "tar";
	butype_info_tar.attrs =  NDMP3_BUTYPE_BACKUP_FILE_HISTORY
			       + NDMP3_BUTYPE_BACKUP_FILELIST
			       + NDMP3_BUTYPE_RECOVER_FILELIST
			       + NDMP3_BUTYPE_BACKUP_DIRECT
			       + NDMP3_BUTYPE_RECOVER_DIRECT
			       + NDMP3_BUTYPE_BACKUP_INCREMENTAL
			       + NDMP3_BUTYPE_RECOVER_INCREMENTAL;

	butype_info_tar.default_env.default_env_val = default_env_tab;
	butype_info_tar.default_env.default_env_len = evp - default_env_tab;

	reply->butype_info.butype_info_val = &butype_info_tar;
	reply->butype_info.butype_info_len = 1;

      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_config_get_butype_info)
	static ndmp4_pval		default_env_tab[4];
	static ndmp4_butype_info	butype_info_tar;
	ndmp4_pval *			evp;

	evp = default_env_tab;
	evp->name = "TYPE";
	evp->value = "tar";
	evp++;

	butype_info_tar.butype_name = "tar";
	butype_info_tar.attrs =  NDMP4_BUTYPE_BACKUP_FILELIST
			       + NDMP4_BUTYPE_RECOVER_FILELIST
			       + NDMP4_BUTYPE_BACKUP_DIRECT
			       + NDMP4_BUTYPE_RECOVER_DIRECT
			       + NDMP4_BUTYPE_BACKUP_INCREMENTAL
			       + NDMP4_BUTYPE_RECOVER_INCREMENTAL;

	butype_info_tar.default_env.default_env_val = default_env_tab;
	butype_info_tar.default_env.default_env_len = evp - default_env_tab;

	reply->butype_info.butype_info_val = &butype_info_tar;
	reply->butype_info.butype_info_len = 1;

      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[34]_CONFIG_GET_FS_INFO
 *
 * If this is implemented, it should already have been
 * intercepted by ndmos_dispatch_request().
 * There is absolutely no way to implement this generically,
 * nor is there merit to a generic "layer".
 * Still, just in case, it is implemented here.
 */
int
ndmadr_config_get_fs_info (struct ndm_session *sess,
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
	/* Just return an empty reply */
	reply->fs_info.fs_info_len = 0;
	reply->fs_info.fs_info_val = 0;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_config_get_fs_info)
	/* Just return an empty reply */
	reply->fs_info.fs_info_len = 0;
	reply->fs_info.fs_info_val = 0;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[34]_CONFIG_GET_TAPE_INFO
 *
 * If this is implemented, it should already have been
 * intercepted by ndmos_dispatch_request().
 * There is absolutely no way to implement this generically,
 * nor is there merit to a generic "layer".
 * Still, just in case, it is implemented here.
 */
int
ndmadr_config_get_tape_info (struct ndm_session *sess,
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
	/* Just return an empty reply */
	reply->tape_info.tape_info_len = 0;
	reply->tape_info.tape_info_val = 0;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_config_get_tape_info)
	/* Just return an empty reply */
	reply->tape_info.tape_info_len = 0;
	reply->tape_info.tape_info_val = 0;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[34]_CONFIG_GET_SCSI_INFO
 *
 * If this is implemented, it should already have been
 * intercepted by ndmos_dispatch_request().
 * There is absolutely no way to implement this generically,
 * nor is there merit to a generic "layer".
 * Still, just in case, it is implemented here.
 */
int
ndmadr_config_get_scsi_info (struct ndm_session *sess,
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
	/* Just return an empty reply */
	reply->scsi_info.scsi_info_len = 0;
	reply->scsi_info.scsi_info_val = 0;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_config_get_scsi_info)
	/* Just return an empty reply */
	reply->scsi_info.scsi_info_len = 0;
	reply->scsi_info.scsi_info_val = 0;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[34]_CONFIG_GET_SERVER_INFO
 */
int
ndmadr_config_get_server_info (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	/* not part of NDMPv2 */
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_config_get_server_info)
	static ndmp3_auth_type auth_type_val[] = {
				NDMP3_AUTH_TEXT, NDMP3_AUTH_MD5 };

	ndmos_get_local_info(sess);
	reply->vendor_name     = sess->local_info.vendor_name;
	reply->product_name    = sess->local_info.product_name;
	reply->revision_number = sess->local_info.revision_number;
	reply->auth_type.auth_type_len = 2;
	reply->auth_type.auth_type_val = auth_type_val;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_config_get_server_info)
	static ndmp4_auth_type auth_type_val[] = {
				NDMP4_AUTH_TEXT, NDMP4_AUTH_MD5 };

	ndmos_get_local_info(sess);
	reply->vendor_name     = sess->local_info.vendor_name;
	reply->product_name    = sess->local_info.product_name;
	reply->revision_number = sess->local_info.revision_number;
	reply->auth_type.auth_type_len = 2;
	reply->auth_type.auth_type_val = auth_type_val;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}
#endif /* !NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4  Surrounds NDMPv[34] CONFIG intfs */




#ifndef NDMOS_OPTION_NO_ROBOT_AGENT	/* Surrounds SCSI intfs */
/*
 * NDMPx_SCSI Interfaces
 ****************************************************************
 *
 * If these are implemented, they should already have been
 * intercepted by ndmos_dispatch_request(). There is absolutely
 * no way to implement this generically, nor is there merit to
 * a generic "layer".
 *
 * Still, just in case, they are implemented here.
 */

static ndmp9_error	scsi_open_ok (struct ndm_session *sess);
static ndmp9_error	scsi_op_ok (struct ndm_session *sess);




/*
 * NDMP[234]_SCSI_OPEN
 */
int
ndmadr_scsi_open (struct ndm_session *sess,
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

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_scsi_open)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_SCSI_CLOSE
 */
int
ndmadr_scsi_close (struct ndm_session *sess,
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

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_scsi_close)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_SCSI_GET_STATE
 */
int
ndmadr_scsi_get_state (struct ndm_session *sess,
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

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_scsi_get_state)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[23]_SCSI_SET_TARGET
 */
int
ndmadr_scsi_set_target (struct ndm_session *sess,
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

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_SCSI_RESET_DEVICE
 */
int
ndmadr_scsi_reset_device (struct ndm_session *sess,
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

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_scsi_reset_device)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[23]_SCSI_RESET_BUS
 */
int
ndmadr_scsi_reset_bus (struct ndm_session *sess,
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

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_SCSI_EXECUTE_CDB
 */
int
ndmadr_scsi_execute_cdb (struct ndm_session *sess,
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

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_scsi_execute_cdb)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMPx_SCSI helper routines
 */

static ndmp9_error
scsi_open_ok (struct ndm_session *sess)
{
	struct ndm_robot_agent *	ra = &sess->robot_acb;

	ndmos_scsi_sync_state(sess);
	if (ra->scsi_state.error != NDMP9_DEV_NOT_OPEN_ERR)
		return NDMP9_DEVICE_OPENED_ERR;

#ifndef NDMOS_OPTION_ALLOW_SCSI_AND_TAPE_BOTH_OPEN
#ifndef NDMOS_OPTION_NO_TAPE_AGENT
	ndmos_tape_sync_state(sess);
	if (sess->tape_acb.tape_state.error != NDMP9_DEV_NOT_OPEN_ERR)
		return NDMP9_DEVICE_OPENED_ERR;
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */
#endif /* NDMOS_OPTION_ALLOW_SCSI_AND_TAPE_BOTH_OPEN */

	return NDMP9_NO_ERR;
}

static ndmp9_error
scsi_op_ok (struct ndm_session *sess)
{
	struct ndm_robot_agent *	ra = &sess->robot_acb;

	ndmos_scsi_sync_state(sess);
	if (ra->scsi_state.error != NDMP9_NO_ERR)
		return NDMP9_DEV_NOT_OPEN_ERR;

	return NDMP9_NO_ERR;
}
#endif /* !NDMOS_OPTION_NO_ROBOT_AGENT */	/* Surrounds SCSI intfs */




#ifndef NDMOS_OPTION_NO_TAPE_AGENT		/* Surrounds TAPE intfs */
/*
 * NDMPx_TAPE Interfaces
 ****************************************************************
 */
static ndmp9_error	tape_open_ok (struct ndm_session *sess,
				int will_write);
static ndmp9_error	tape_op_ok (struct ndm_session *sess,
				int will_write);




/*
 * NDMP[234]_TAPE_OPEN
 */
int
ndmadr_tape_open (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    ndmp9_error			error;
    int				will_write;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_tape_open)
	switch (request->mode) {
	default:
		NDMADR_RAISE_ILLEGAL_ARGS("tape_mode");

	case NDMP2_TAPE_READ_MODE:
		will_write = 0;
		break;

	case NDMP2_TAPE_WRITE_MODE:
		will_write = 1;
		break;
	}

	error = tape_open_ok (sess, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_open_ok");

	error = ndmos_tape_open (sess, request->device.name, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "tape_open");
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_tape_open)
	switch (request->mode) {
	default:
		NDMADR_RAISE_ILLEGAL_ARGS("tape_mode");

	case NDMP3_TAPE_READ_MODE:
		will_write = 0;
		break;

	case NDMP3_TAPE_RDWR_MODE:
		will_write = 1;
		break;
	}

	error = tape_open_ok (sess, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_open_ok");

	error = ndmos_tape_open (sess, request->device, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "tape_open");
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_tape_open)
	switch (request->mode) {
	default:
		NDMADR_RAISE_ILLEGAL_ARGS("tape_mode");

	case NDMP4_TAPE_READ_MODE:
		will_write = 0;
		break;

	case NDMP4_TAPE_RDWR_MODE:
		will_write = 1;
		break;
	}

	error = tape_open_ok (sess, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_open_ok");

	error = ndmos_tape_open (sess, request->device, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "tape_open");
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_TAPE_CLOSE
 */
int
ndmadr_tape_close (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    ndmp9_error			error;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_tape_close)
	error = tape_op_ok (sess, 0);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_close (sess);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "tape_close");
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_tape_close)
	error = tape_op_ok (sess, 0);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_close (sess);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "tape_close");
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_tape_close)
	error = tape_op_ok (sess, 0);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_close (sess);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "tape_close");
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_TAPE_GET_STATE
 */
int
ndmadr_tape_get_state (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_tape_agent *	ta = &sess->tape_acb;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_tape_get_state)
	ndmos_tape_sync_state(sess);
	ndmp_9to2_tape_get_state_reply (&ta->tape_state, reply);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_tape_get_state)
	ndmos_tape_sync_state(sess);
	ndmp_9to3_tape_get_state_reply (&ta->tape_state, reply);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_tape_get_state)
	ndmos_tape_sync_state(sess);
	ndmp_9to4_tape_get_state_reply (&ta->tape_state, reply);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_TAPE_MTIO
 */
int
ndmadr_tape_mtio (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    ndmp9_error			error;
    ndmp9_tape_mtio_op		tape_op;
    int				will_write = 0;
    unsigned long		resid = 0;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_tape_mtio)

	switch (request->tape_op) {
	default:
		NDMADR_RAISE_ILLEGAL_ARGS("tape_op");

	case NDMP2_MTIO_EOF:
		will_write = 1;
		tape_op = NDMP9_MTIO_EOF;
		break;

	case NDMP2_MTIO_FSF:	tape_op = NDMP9_MTIO_FSF; break;
	case NDMP2_MTIO_BSF:	tape_op = NDMP9_MTIO_BSF; break;
	case NDMP2_MTIO_FSR:	tape_op = NDMP9_MTIO_FSR; break;
	case NDMP2_MTIO_BSR:	tape_op = NDMP9_MTIO_BSR; break;
	case NDMP2_MTIO_REW:	tape_op = NDMP9_MTIO_REW; break;
	case NDMP2_MTIO_OFF:	tape_op = NDMP9_MTIO_OFF; break;
	}

	error = tape_op_ok (sess, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_mtio (sess, tape_op, request->count, &resid);
	reply->error = error;
	reply->resid_count = resid;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_tape_mtio)

	switch (request->tape_op) {
	default:
		NDMADR_RAISE_ILLEGAL_ARGS("tape_op");

	case NDMP3_MTIO_EOF:
		will_write = 1;
		tape_op = NDMP9_MTIO_EOF;
		break;

	case NDMP3_MTIO_FSF:	tape_op = NDMP9_MTIO_FSF; break;
	case NDMP3_MTIO_BSF:	tape_op = NDMP9_MTIO_BSF; break;
	case NDMP3_MTIO_FSR:	tape_op = NDMP9_MTIO_FSR; break;
	case NDMP3_MTIO_BSR:	tape_op = NDMP9_MTIO_BSR; break;
	case NDMP3_MTIO_REW:	tape_op = NDMP9_MTIO_REW; break;
	case NDMP3_MTIO_OFF:	tape_op = NDMP9_MTIO_OFF; break;
	}

	error = tape_op_ok (sess, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_mtio (sess, tape_op, request->count, &resid);
	reply->error = error;
	reply->resid_count = resid;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_tape_mtio)

	switch (request->tape_op) {
	default:
		NDMADR_RAISE_ILLEGAL_ARGS("tape_op");

	case NDMP4_MTIO_EOF:
		will_write = 1;
		tape_op = NDMP9_MTIO_EOF;
		break;

	case NDMP4_MTIO_FSF:	tape_op = NDMP9_MTIO_FSF; break;
	case NDMP4_MTIO_BSF:	tape_op = NDMP9_MTIO_BSF; break;
	case NDMP4_MTIO_FSR:	tape_op = NDMP9_MTIO_FSR; break;
	case NDMP4_MTIO_BSR:	tape_op = NDMP9_MTIO_BSR; break;
	case NDMP4_MTIO_REW:	tape_op = NDMP9_MTIO_REW; break;
	case NDMP4_MTIO_OFF:	tape_op = NDMP9_MTIO_OFF; break;
	}

	error = tape_op_ok (sess, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_mtio (sess, tape_op, request->count, &resid);
	reply->error = error;
	reply->resid_count = resid;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_TAPE_WRITE
 */
int
ndmadr_tape_write (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    ndmp9_error			error;
    unsigned long		done_count = 0;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_tape_write)
	if (!NDMOS_MACRO_OK_TAPE_REC_LEN(request->data_out.data_out_len))
		NDMADR_RAISE_ILLEGAL_ARGS("!ok_tape_rec_len");

	error = tape_op_ok (sess, 1);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_write (sess, request->data_out.data_out_val,
				request->data_out.data_out_len,
				&done_count);
	reply->error = error;
	reply->count = done_count;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_tape_write)
	if (!NDMOS_MACRO_OK_TAPE_REC_LEN(request->data_out.data_out_len))
		NDMADR_RAISE_ILLEGAL_ARGS("!ok_tape_rec_len");

	error = tape_op_ok (sess, 1);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_write (sess, request->data_out.data_out_val,
				request->data_out.data_out_len,
				&done_count);
	reply->error = error;
	reply->count = done_count;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_tape_write)
	if (!NDMOS_MACRO_OK_TAPE_REC_LEN(request->data_out.data_out_len))
		NDMADR_RAISE_ILLEGAL_ARGS("!ok_tape_rec_len");

	error = tape_op_ok (sess, 1);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_write (sess, request->data_out.data_out_val,
				request->data_out.data_out_len,
				&done_count);
	reply->error = error;
	reply->count = done_count;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_TAPE_READ
 */
int
ndmadr_tape_read (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_tape_agent *	ta = &sess->tape_acb;
    ndmp9_error			error;
    unsigned long		done_count = 0;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_tape_read)
	if (!NDMOS_MACRO_OK_TAPE_REC_LEN(request->count))
		NDMADR_RAISE_ILLEGAL_ARGS("!ok_tape_rec_len");

	error = tape_op_ok (sess, 0);
	if (error != NDMP2_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_read (sess, ta->tape_buffer,
				request->count,
				&done_count);
	reply->error = error;
	reply->data_in.data_in_val = ta->tape_buffer;
	reply->data_in.data_in_len = done_count;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_tape_read)
	if (!NDMOS_MACRO_OK_TAPE_REC_LEN(request->count))
		NDMADR_RAISE_ILLEGAL_ARGS("!ok_tape_rec_len");

	error = tape_op_ok (sess, 0);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_read (sess, ta->tape_buffer,
				request->count,
				&done_count);
	reply->error = error;
	reply->data_in.data_in_val = ta->tape_buffer;
	reply->data_in.data_in_len = done_count;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_tape_read)
	if (!NDMOS_MACRO_OK_TAPE_REC_LEN(request->count))
		NDMADR_RAISE_ILLEGAL_ARGS("!ok_tape_rec_len");

	error = tape_op_ok (sess, 0);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!tape_op_ok");

	error = ndmos_tape_read (sess, ta->tape_buffer,
				request->count,
				&done_count);
	reply->error = error;
	reply->data_in.data_in_val = ta->tape_buffer;
	reply->data_in.data_in_len = done_count;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_TAPE_EXECUTE_CDB
 *
 * If this is implemented, it should already have been
 * intercepted by ndmos_dispatch_request().
 * There is absolutely no way to implement this generically,
 * nor is there merit to a generic "layer".
 * Still, just in case, it is implemented here.
 */
int
ndmadr_tape_execute_cdb (struct ndm_session *sess,
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

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_tape_execute_cdb)
	return NDMADR_UNIMPLEMENTED_MESSAGE;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMPx_TAPE helper routines
 */

static ndmp9_error
tape_open_ok (struct ndm_session *sess, int will_write)
{
	struct ndm_tape_agent *		ta = &sess->tape_acb;

	ndmos_tape_sync_state(sess);
	if (ta->tape_state.state != NDMP9_TAPE_STATE_IDLE)
		return NDMP9_DEVICE_OPENED_ERR;

#ifndef NDMOS_OPTION_ALLOW_SCSI_AND_TAPE_BOTH_OPEN
#ifndef NDMOS_OPTION_NO_ROBOT_AGENT
	ndmos_scsi_sync_state(sess);
	if (sess->robot_acb.scsi_state.error != NDMP9_DEV_NOT_OPEN_ERR)
		return NDMP9_DEVICE_OPENED_ERR;
#endif /* !NDMOS_OPTION_NO_ROBOT_AGENT */
#endif /* NDMOS_OPTION_ALLOW_SCSI_AND_TAPE_BOTH_OPEN */

	return NDMP9_NO_ERR;
}

/*
 * Tape operation is only OK if it is open and the MOVER
 * hasn't got a hold of it. We can't allow tape operations
 * to interfere with the MOVER.
 */

static ndmp9_error
tape_op_ok (struct ndm_session *sess, int will_write)
{
	struct ndm_tape_agent *		ta = &sess->tape_acb;

	ndmos_tape_sync_state(sess);
	switch (ta->tape_state.state) {
	case NDMP9_TAPE_STATE_IDLE:
		return NDMP9_DEV_NOT_OPEN_ERR;

	case NDMP9_TAPE_STATE_OPEN:
		if (will_write && !NDMTA_TAPE_IS_WRITABLE(ta))
			return NDMP9_PERMISSION_ERR;
		break;

	case NDMP9_TAPE_STATE_MOVER:
		return NDMP9_ILLEGAL_STATE_ERR;
	}

	return NDMP9_NO_ERR;
}
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */	/* Surrounds TAPE intfs */


#ifndef NDMOS_OPTION_NO_DATA_AGENT		/* Surrounds DATA intfs */
/*
 * NDMPx_DATA Interfaces
 ****************************************************************
 */

static int		data_can_connect (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn,
				ndmp9_addr *data_addr);

static int		data_can_start (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn,
				ndmp9_mover_mode mover_mode);

static int		data_can_connect_and_start (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn,
				ndmp9_addr *data_addr,
				ndmp9_mover_mode mover_mode);

static int		data_abort_common234 (struct ndm_session *sess,
			  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn);

static int		data_stop_common234 (struct ndm_session *sess,
			  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn);

static int		data_connect_common234 (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn,
				ndmp9_addr *data_addr);

#ifndef NDMOS_OPTION_NO_NDMP2
static ndmp9_error	data_copy_v2_environment (struct ndm_session *sess,
				ndmp2_pval *env2, unsigned n_env);
static ndmp9_error	data_copy_v2_nlist (struct ndm_session *sess,
				ndmp2_name *nlist2, unsigned n_nlist);
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
static ndmp9_error	data_copy_v3_environment (struct ndm_session *sess,
				ndmp3_pval *env3, unsigned n_env);
static ndmp9_error	data_copy_v3_nlist (struct ndm_session *sess,
				ndmp3_name *nlist3, unsigned n_nlist);
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
static ndmp9_error	data_copy_v4_environment (struct ndm_session *sess,
				ndmp4_pval *env4, unsigned n_env);
static ndmp9_error	data_copy_v4_nlist (struct ndm_session *sess,
				ndmp4_name *nlist4, unsigned n_nlist);
#endif /* !NDMOS_OPTION_NO_NDMP4 */
#ifndef NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4
static int		data_listen_common34 (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn,
				ndmp9_addr_type addr_type);
#endif /* !NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4 */




/*
 * NDMP[234]_DATA_GET_STATE
 */
int
ndmadr_data_get_state (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_data_agent *	da = &sess->data_acb;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_data_get_state)
	ndmp_9to2_data_get_state_reply (&da->data_state, reply);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_data_get_state)
	ndmp_9to3_data_get_state_reply (&da->data_state, reply);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_data_get_state)
	ndmp_9to4_data_get_state_reply (&da->data_state, reply);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_DATA_START_BACKUP
 */
int
ndmadr_data_start_backup (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_data_agent *	da = &sess->data_acb;
    int				rc;
    ndmp9_error			error;
    ndmp9_addr			mover;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_data_start_backup)
	if (strcmp (request->bu_type, "tar") != 0)
		NDMADR_RAISE_ILLEGAL_ARGS("!tar");

	ndmp_2to9_mover_addr (&request->mover, &mover);

	rc = data_can_connect_and_start (sess, xa, ref_conn,
				&mover, NDMP9_MOVER_MODE_READ);
	if (rc) return rc;	/* already tattled */

	error = data_copy_v2_environment (sess,
		request->env.env_val, request->env.env_len);
	if (error != NDMP9_NO_ERR) {
		NDMADR_RAISE(error, "copy-env");
	}

	rc = data_connect_common234 (sess, xa, ref_conn, &mover);
	if (rc) {
		ndmda_belay (sess);
		return rc;	/* already tattled */
	}

	da->data_state.data_connection_addr = mover;

	error = ndmda_data_start_backup (sess);
	if (error != NDMP9_NO_ERR) {
		/* TODO: undo everything */
		ndmda_belay (sess);
		NDMADR_RAISE(error, "start_backup");
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_data_start_backup)
	if (strcmp (request->bu_type, "tar") != 0)
		NDMADR_RAISE_ILLEGAL_ARGS("!tar");

	rc = data_can_start (sess, xa, ref_conn, NDMP9_MOVER_MODE_READ);
	if (rc) return rc;	/* already tattled */

	error = data_copy_v3_environment (sess,
		request->env.env_val, request->env.env_len);
	if (error != NDMP9_NO_ERR) {
		NDMADR_RAISE(error, "copy-env");
	}

	error = ndmda_data_start_backup (sess);
	if (error != NDMP9_NO_ERR) {
		/* TODO: undo everything */
		ndmda_belay (sess);
		NDMADR_RAISE(error, "start_backup");
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_data_start_backup)
	if (strcmp (request->bu_type, "tar") != 0)
		NDMADR_RAISE_ILLEGAL_ARGS("!tar");

	rc = data_can_start (sess, xa, ref_conn, NDMP9_MOVER_MODE_READ);
	if (rc) return rc;	/* already tattled */

	error = data_copy_v4_environment (sess,
		request->env.env_val, request->env.env_len);
	if (error != NDMP9_NO_ERR) {
		NDMADR_RAISE(error, "copy-env");
	}

	error = ndmda_data_start_backup (sess);
	if (error != NDMP9_NO_ERR) {
		/* TODO: undo everything */
		ndmda_belay (sess);
		NDMADR_RAISE(error, "start_backup");
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_DATA_START_RECOVER
 */
int
ndmadr_data_start_recover (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_data_agent *	da = &sess->data_acb;
    ndmp9_error			error;
    ndmp9_addr			mover;
    int				rc;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_data_start_recover)
	if (strcmp (request->bu_type, "tar") != 0)
		NDMADR_RAISE_ILLEGAL_ARGS("!tar");

	ndmp_2to9_mover_addr (&request->mover, &mover);

	rc = data_can_connect_and_start (sess, xa, ref_conn,
				&mover, NDMP9_MOVER_MODE_WRITE);
	if (rc) return rc;	/* already tattled */

	error = data_copy_v2_environment (sess,
		request->env.env_val, request->env.env_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-env");
	}

	error = data_copy_v2_nlist (sess,
		request->nlist.nlist_val, request->nlist.nlist_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-nlist");
	}

	rc = data_connect_common234 (sess, xa, ref_conn, &mover);
	if (rc) {
		ndmda_belay (sess);
		return rc;	/* already tattled */
	}

	da->data_state.data_connection_addr = mover;

	error = ndmda_data_start_recover (sess);
	if (error != NDMP9_NO_ERR) {
		/* TODO: undo everything */
		ndmda_belay (sess);
		NDMADR_RAISE(error, "start_recover");
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_data_start_recover)
	if (strcmp (request->bu_type, "tar") != 0)
		NDMADR_RAISE_ILLEGAL_ARGS("!tar");

	rc = data_can_start (sess, xa, ref_conn, NDMP9_MOVER_MODE_WRITE);
	if (rc) return rc;	/* already tattled */

	error = data_copy_v3_environment (sess,
		request->env.env_val, request->env.env_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-env");
	}

	error = data_copy_v3_nlist (sess,
		request->nlist.nlist_val, request->nlist.nlist_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-nlist");
	}

	error = ndmda_data_start_recover (sess);
	if (error != NDMP9_NO_ERR) {
		/* TODO: undo everything */
		ndmda_belay (sess);
		NDMADR_RAISE(error, "start_recover");
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_data_start_recover)
	if (strcmp (request->bu_type, "tar") != 0)
		NDMADR_RAISE_ILLEGAL_ARGS("!tar");

	rc = data_can_start (sess, xa, ref_conn, NDMP9_MOVER_MODE_WRITE);
	if (rc) return rc;	/* already tattled */

	error = data_copy_v4_environment (sess,
		request->env.env_val, request->env.env_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-env");
	}

	error = data_copy_v4_nlist (sess,
		request->nlist.nlist_val, request->nlist.nlist_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-nlist");
	}

	error = ndmda_data_start_recover (sess);
	if (error != NDMP9_NO_ERR) {
		/* TODO: undo everything */
		ndmda_belay (sess);
		NDMADR_RAISE(error, "start_recover");
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_DATA_ABORT
 */
int
ndmadr_data_abort (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_data_abort)
	return data_abort_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_data_abort)
	return data_abort_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_data_abort)
	return data_abort_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

static int
data_abort_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
	struct ndm_data_agent *	da = &sess->data_acb;

	if (da->data_state.state != NDMP9_DATA_STATE_ACTIVE)
		NDMADR_RAISE_ILLEGAL_STATE("data_state !ACTIVE");

	ndmda_data_abort (sess);

	return 0;
}




/*
 * NDMP[234]_DATA_GET_ENV
 */
int
ndmadr_data_get_env (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_data_agent *	da = &sess->data_acb;
    ndmp9_pval *		env = da->env_tab.env;
    unsigned			n_env = da->env_tab.n_env;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_data_get_env)
	ndmp2_pval *		envbuf;

	if (da->data_state.state == NDMP9_DATA_STATE_IDLE)
		NDMADR_RAISE_ILLEGAL_STATE("data_state IDLE");
	if (da->data_state.operation != NDMP9_DATA_OP_BACKUP)
		NDMADR_RAISE_ILLEGAL_STATE("data_op !BACKUP");

	ndmda_sync_environment (sess);

	envbuf = NDMOS_MACRO_NEWN (ndmp2_pval, n_env);
	if (!envbuf)
		NDMADR_RAISE(NDMP9_NO_MEM_ERR, "env-tab");

	ndmp_9to2_pval_vec (env, envbuf, n_env);
	reply->env.env_len = da->env_tab.n_env;
	reply->env.env_val = envbuf;

#if 0
	xa->reply.flags &= ~NDMNMB_FLAG_NO_FREE;  /* free env after xmit */
#endif
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_data_get_env)
	ndmp3_pval *		envbuf;

	if (da->data_state.state == NDMP9_DATA_STATE_IDLE)
		NDMADR_RAISE_ILLEGAL_STATE("data_state IDLE");
	if (da->data_state.operation != NDMP9_DATA_OP_BACKUP)
		NDMADR_RAISE_ILLEGAL_STATE("data_op !BACKUP");

	ndmda_sync_environment (sess);

	envbuf = NDMOS_MACRO_NEWN (ndmp3_pval, n_env);
	if (!envbuf)
		NDMADR_RAISE(NDMP9_NO_MEM_ERR, "env-tab");

	ndmp_9to3_pval_vec (env, envbuf, n_env);
	reply->env.env_len = da->env_tab.n_env;
	reply->env.env_val = envbuf;

#if 0
	xa->reply.flags &= ~NDMNMB_FLAG_NO_FREE;  /* free env after xmit */
#endif
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_data_get_env)
	ndmp4_pval *		envbuf;

	if (da->data_state.state == NDMP9_DATA_STATE_IDLE)
		NDMADR_RAISE_ILLEGAL_STATE("data_state IDLE");
	if (da->data_state.operation != NDMP9_DATA_OP_BACKUP)
		NDMADR_RAISE_ILLEGAL_STATE("data_op !BACKUP");

	ndmda_sync_environment (sess);

	envbuf = NDMOS_MACRO_NEWN (ndmp4_pval, n_env);
	if (!envbuf)
		NDMADR_RAISE(NDMP9_NO_MEM_ERR, "env-tab");

	ndmp_9to4_pval_vec (env, envbuf, n_env);
	reply->env.env_len = da->env_tab.n_env;
	reply->env.env_val = envbuf;

#if 0
	xa->reply.flags &= ~NDMNMB_FLAG_NO_FREE;  /* free env after xmit */
#endif
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_DATA_STOP
 */
int
ndmadr_data_stop (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_data_stop)
	return data_stop_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_data_stop)
	return data_stop_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_data_stop)
	return data_stop_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

static int
data_stop_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
	struct ndm_data_agent *	da = &sess->data_acb;

	if (da->data_state.state != NDMP9_DATA_STATE_HALTED)
		NDMADR_RAISE_ILLEGAL_STATE("data_state !HALTED");

	ndmda_data_stop (sess);

	return 0;
}


/*
 * For NDMPv2, called from ndmadr_data_start_{backup,recover,recover_filhist}()
 * For NDMPv[34], called from ndmadr_data_connect()
 */
static int
data_connect_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  ndmp9_addr *data_addr)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	int			rc;
	ndmp9_error		error;
	char			reason[100];

	rc = data_can_connect (sess, xa, ref_conn, data_addr);
	if (rc)
		return rc;

	/*
	 * Audits done, connect already
	 */
	error = ndmis_data_connect (sess, data_addr, reason);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, reason);

	da->data_state.data_connection_addr = *data_addr;
	/* alt: da->....data_connection_addr = sess->...peer_addr */

	error = ndmda_data_connect (sess);
	if (error != NDMP9_NO_ERR) {
		/* TODO: belay ndmis_data_connect() */
		NDMADR_RAISE(error, "!data_connect");
	}

	return 0;
}


#ifndef NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4	/* Surrounds NDMPv[34] DATA intfs */
/*
 * NDMP[34]_DATA_LISTEN
 */
int
ndmadr_data_listen (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_data_agent *	da = &sess->data_acb;
    int				rc;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	/* not part of NDMPv2 */
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_data_listen)
	ndmp9_addr_type		addr_type;

	/* Check args, map along the way */
	switch (request->addr_type) {
	default:		addr_type = -1; break;
	case NDMP3_ADDR_LOCAL:	addr_type = NDMP9_ADDR_LOCAL;	break;
	case NDMP3_ADDR_TCP:	addr_type = NDMP9_ADDR_TCP;	break;
	}

	rc = data_listen_common34 (sess, xa, ref_conn, addr_type);
	if (rc)
		return rc;	/* something went wrong */

	ndmp_9to3_addr (&da->data_state.data_connection_addr,
				&reply->data_connection_addr);
	/* reply->error already set to NDMPx_NO_ERROR */
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_data_listen)
	ndmp9_addr_type		addr_type;

	/* Check args, map along the way */
	switch (request->addr_type) {
	default:		addr_type = -1; break;
	case NDMP4_ADDR_LOCAL:	addr_type = NDMP9_ADDR_LOCAL;	break;
	case NDMP4_ADDR_TCP:	addr_type = NDMP9_ADDR_TCP;	break;
	}

	rc = data_listen_common34 (sess, xa, ref_conn, addr_type);
	if (rc)
		return rc;	/* something went wrong */

	ndmp_9to4_addr (&da->data_state.data_connection_addr,
				&reply->data_connection_addr);
	/* reply->error already set to NDMPx_NO_ERROR */
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

/* this same intf is expected in v4, so _common() now */
static int
data_listen_common34 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  ndmp9_addr_type addr_type)
{
	struct ndm_data_agent *	da = &sess->data_acb;
#ifndef NDMOS_OPTION_NO_TAPE_AGENT
	struct ndm_tape_agent *	ta = &sess->tape_acb;
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */
	ndmp9_error		error;
	char			reason[100];

	/* Check args */

	switch (addr_type) {
	default:		NDMADR_RAISE_ILLEGAL_ARGS("mover_addr_type");
	case NDMP9_ADDR_LOCAL:
#ifdef NDMOS_OPTION_NO_TAPE_AGENT
		NDMADR_RAISE_ILLEGAL_ARGS("data LOCAL w/o local TAPE agent");
#endif /* NDMOS_OPTION_NO_TAPE_AGENT */
		break;

	case NDMP9_ADDR_TCP:
		break;
	}

	/* Check states -- this should cover everything */
	if (da->data_state.state != NDMP9_DATA_STATE_IDLE)
		NDMADR_RAISE_ILLEGAL_STATE("data_state !IDLE");
#ifndef NDMOS_OPTION_NO_TAPE_AGENT
	if (ta->mover_state.state != NDMP9_MOVER_STATE_IDLE)
		NDMADR_RAISE_ILLEGAL_STATE("mover_state !IDLE");
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */

	/*
	 * Check image stream state -- should already be reflected
	 * in the mover and data states. This extra check gives
	 * us an extra measure of robustness and sanity
	 * check on the implementation.
	 */
	error = ndmis_audit_data_listen (sess, addr_type, reason);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, reason);

	error = ndmis_data_listen (sess, addr_type,
			&da->data_state.data_connection_addr,
			reason);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, reason);

	error = ndmda_data_listen(sess);
	if (error != NDMP9_NO_ERR) {
		/* TODO: belay ndmis_data_listen() */
		NDMADR_RAISE(error, "!data_listen");
	}

	return 0;
}


/*
 * NDMP[34]_DATA_CONNECT
 */
int
ndmadr_data_connect (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	/* not part of NDMPv2 */
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_data_connect)
	ndmp9_addr		data_addr;

	ndmp_3to9_addr (&request->addr, &data_addr);

	return data_connect_common234 (sess, xa, ref_conn, &data_addr);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_data_connect)
	ndmp9_addr		data_addr;

	ndmp_4to9_addr (&request->addr, &data_addr);

	return data_connect_common234 (sess, xa, ref_conn, &data_addr);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

#endif /* !NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4  Surrounds NDMPv[34] DATA intfs */



/*
 * NDMP[234]_DATA_START_RECOVER_FILEHIST
 * This is a Traakan extension, and under contemplation in NDMPv3.1
 */
int
ndmadr_data_start_recover_filehist (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_data_agent *	da = &sess->data_acb;
    ndmp9_error			error;
    ndmp9_addr			mover;
    int				rc;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_data_start_recover_filehist)
	if (strcmp (request->bu_type, "tar") != 0)
		NDMADR_RAISE_ILLEGAL_ARGS("!tar");

	ndmp_2to9_mover_addr (&request->mover, &mover);

	rc = data_can_connect_and_start (sess, xa, ref_conn,
				&mover, NDMP9_MOVER_MODE_WRITE);
	if (rc) return rc;	/* already tattled */

	error = data_copy_v2_environment (sess,
		request->env.env_val, request->env.env_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-env");
	}

	error = data_copy_v2_nlist (sess,
		request->nlist.nlist_val, request->nlist.nlist_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-nlist");
	}

	rc = data_connect_common234 (sess, xa, ref_conn, &mover);
	if (rc) {
		ndmda_belay (sess);
		return rc;	/* already tattled */
	}

	da->data_state.data_connection_addr = mover;

	error = ndmda_data_start_recover_fh (sess);
	if (error != NDMP2_NO_ERR) {
		/* TODO: undo everything */
		ndmda_belay (sess);
		NDMADR_RAISE(error, "start_recover_fh");
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_data_start_recover_filehist)
	if (strcmp (request->bu_type, "tar") != 0)
		NDMADR_RAISE_ILLEGAL_ARGS("!tar");

	rc = data_can_start (sess, xa, ref_conn, NDMP9_MOVER_MODE_WRITE);
	if (rc) return rc;	/* already tattled */

	error = data_copy_v3_environment (sess,
		request->env.env_val, request->env.env_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-env");
	}

	error = data_copy_v3_nlist (sess,
		request->nlist.nlist_val, request->nlist.nlist_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-nlist");
	}

	error = ndmda_data_start_recover_fh (sess);
	if (error != NDMP9_NO_ERR) {
		/* TODO: undo everything */
		ndmda_belay (sess);
		NDMADR_RAISE(error, "start_recover");
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_data_start_recover_filehist)
	if (strcmp (request->bu_type, "tar") != 0)
		NDMADR_RAISE_ILLEGAL_ARGS("!tar");

	rc = data_can_start (sess, xa, ref_conn, NDMP9_MOVER_MODE_WRITE);
	if (rc) return rc;	/* already tattled */

	error = data_copy_v4_environment (sess,
		request->env.env_val, request->env.env_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-env");
	}

	error = data_copy_v4_nlist (sess,
		request->nlist.nlist_val, request->nlist.nlist_len);
	if (error != NDMP9_NO_ERR) {
		ndmda_belay (sess);
		NDMADR_RAISE(error, "copy-nlist");
	}

	error = ndmda_data_start_recover_fh (sess);
	if (error != NDMP9_NO_ERR) {
		/* TODO: undo everything */
		ndmda_belay (sess);
		NDMADR_RAISE(error, "start_recover");
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMPx_DATA helper routines
 */

/*
 * Data can only start if the mover is ready.
 * Just mode and state checks.
 */

static int
data_can_connect (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  ndmp9_addr *data_addr)
{
	struct ndm_data_agent *	da = &sess->data_acb;
#ifndef NDMOS_OPTION_NO_TAPE_AGENT
	struct ndm_tape_agent *	ta = &sess->tape_acb;
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */
	ndmp9_error		error;
	char			reason[100];

	/* Check args */
	switch (data_addr->addr_type) {
	default:		NDMADR_RAISE_ILLEGAL_ARGS("addr_type");
	case NDMP9_ADDR_LOCAL:
#ifdef NDMOS_OPTION_NO_TAPE_AGENT
		NDMADR_RAISE_ILLEGAL_ARGS("mover LOCAL w/o local DATA agent");
#endif /* NDMOS_OPTION_NO_TAPE_AGENT */
		break;

	case NDMP9_ADDR_TCP:
		break;
	}

	/* Check states -- this should cover everything */
	if (da->data_state.state != NDMP9_DATA_STATE_IDLE)
		NDMADR_RAISE_ILLEGAL_STATE("data_state !IDLE");

#ifndef NDMOS_OPTION_NO_TAPE_AGENT
	if (data_addr->addr_type == NDMP9_ADDR_LOCAL) {
		ndmp9_mover_get_state_reply *ms = &ta->mover_state;

		if (ms->state != NDMP9_MOVER_STATE_LISTEN)
			NDMADR_RAISE_ILLEGAL_STATE("mover_state !LISTEN");

		if (ms->data_connection_addr.addr_type != NDMP9_ADDR_LOCAL)
			NDMADR_RAISE_ILLEGAL_STATE("mover_addr !LOCAL");

	} else {
		if (ta->mover_state.state != NDMP9_MOVER_STATE_IDLE)
			NDMADR_RAISE_ILLEGAL_STATE("mover_state !IDLE");
	}
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */

	/*
	 * Check image stream state -- should already be reflected
	 * in the mover and data states. This extra check gives
	 * us an extra measure of robustness and sanity
	 * check on the implementation.
	 */
	error = ndmis_audit_data_connect (sess, data_addr->addr_type, reason);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, reason);

	return 0;
}

static int
data_can_connect_and_start (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  ndmp9_addr *data_addr,
  ndmp9_mover_mode mover_mode)
{
	int			rc;

	/* Check args */
	switch (mover_mode) {
	default:		NDMADR_RAISE_ILLEGAL_ARGS("mover_mode");
	case NDMP9_MOVER_MODE_READ:	/* aka BACKUP */
	case NDMP9_MOVER_MODE_WRITE:	/* aka RECOVER */
		break;
	}

	rc = data_can_connect (sess, xa, ref_conn, data_addr);
	if (rc) return rc;

#ifndef NDMOS_OPTION_NO_TAPE_AGENT
	if (data_addr->addr_type == NDMP9_ADDR_LOCAL) {
		struct ndm_tape_agent *		ta = &sess->tape_acb;
		ndmp9_mover_get_state_reply *	ms = &ta->mover_state;

		if (ms->mode != mover_mode)
			NDMADR_RAISE_ILLEGAL_STATE("mover_mode mismatch");
	}
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */

	return 0;
}

static int
data_can_start (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  ndmp9_mover_mode mover_mode)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	ndmp9_data_get_state_reply *ds = &da->data_state;
#ifndef NDMOS_OPTION_NO_TAPE_AGENT
	struct ndm_tape_agent *	ta = &sess->tape_acb;
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */

	/* Check args */
	switch (mover_mode) {
	default:		NDMADR_RAISE_ILLEGAL_ARGS("mover_mode");
	case NDMP9_MOVER_MODE_READ:	/* aka BACKUP */
	case NDMP9_MOVER_MODE_WRITE:	/* aka RECOVER */
		break;
	}

	/* Check states -- this should cover everything */
	if (da->data_state.state != NDMP9_DATA_STATE_CONNECTED)
		NDMADR_RAISE_ILLEGAL_STATE("data_state !CONNECTED");

#ifndef NDMOS_OPTION_NO_TAPE_AGENT
	if (ds->data_connection_addr.addr_type == NDMP9_ADDR_LOCAL) {
		ndmp9_mover_get_state_reply *ms = &ta->mover_state;

		if (ms->state != NDMP9_MOVER_STATE_ACTIVE)
			NDMADR_RAISE_ILLEGAL_STATE("mover_state !ACTIVE");

		if (ms->data_connection_addr.addr_type != NDMP9_ADDR_LOCAL)
			NDMADR_RAISE_ILLEGAL_STATE("mover_addr !LOCAL");

		if (ms->mode != mover_mode)
			NDMADR_RAISE_ILLEGAL_STATE("mover_mode mismatch");
	} else {
		if (ta->mover_state.state != NDMP9_MOVER_STATE_IDLE)
			NDMADR_RAISE_ILLEGAL_STATE("mover_state !IDLE");
	}
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */

	return 0;
}


#ifndef NDMOS_OPTION_NO_NDMP2
static ndmp9_error
data_copy_v2_environment (struct ndm_session *sess,
  ndmp2_pval *env2, unsigned n_env)
{
	struct ndm_env_table	envtab;
	int			rc;

	NDMOS_MACRO_ZEROFILL (&envtab);

	if (n_env > NDM_MAX_ENV)
		return NDMP9_ILLEGAL_ARGS_ERR;

	ndmp_2to9_pval_vec (env2, envtab.env, n_env);
	envtab.n_env = n_env;

	rc = ndmda_copy_environment (sess, envtab.env, envtab.n_env);
	if (rc != 0)
		return NDMP9_NO_MEM_ERR;

	return NDMP9_NO_ERR;
}

static ndmp9_error
data_copy_v2_nlist (struct ndm_session *sess,
  ndmp2_name *nlist2, unsigned n_nlist)
{
	struct ndm_nlist_table	nlisttab;
	int			rc;

	NDMOS_MACRO_ZEROFILL (&nlisttab);

	if (n_nlist > NDM_MAX_NLIST)
		return NDMP9_ILLEGAL_ARGS_ERR;

	ndmp_2to9_name_vec (nlist2, nlisttab.nlist, n_nlist);
	nlisttab.n_nlist = n_nlist;

	rc = ndmda_copy_nlist (sess, nlisttab.nlist, nlisttab.n_nlist);
	if (rc != 0)
		return NDMP9_NO_MEM_ERR;

	return NDMP9_NO_ERR;
}

#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
static ndmp9_error
data_copy_v3_environment (struct ndm_session *sess,
  ndmp3_pval *env3, unsigned n_env)
{
	struct ndm_env_table	envtab;
	int			rc;

	NDMOS_MACRO_ZEROFILL (&envtab);

	if (n_env > NDM_MAX_ENV)
		return NDMP9_ILLEGAL_ARGS_ERR;

	ndmp_3to9_pval_vec (env3, envtab.env, n_env);
	envtab.n_env = n_env;

	rc = ndmda_copy_environment (sess, envtab.env, envtab.n_env);
	if (rc != 0)
		return NDMP9_NO_MEM_ERR;

	return NDMP9_NO_ERR;
}

static ndmp9_error
data_copy_v3_nlist (struct ndm_session *sess,
  ndmp3_name *nlist3, unsigned n_nlist)
{
	struct ndm_nlist_table	nlisttab;
	int			rc;

	NDMOS_MACRO_ZEROFILL (&nlisttab);

	if (n_nlist > NDM_MAX_NLIST)
		return NDMP9_ILLEGAL_ARGS_ERR;

	ndmp_3to9_name_vec (nlist3, nlisttab.nlist, n_nlist);
	nlisttab.n_nlist = n_nlist;

	rc = ndmda_copy_nlist (sess, nlisttab.nlist, nlisttab.n_nlist);
	if (rc != 0)
		return NDMP9_NO_MEM_ERR;

	return NDMP9_NO_ERR;
}

#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
static ndmp9_error
data_copy_v4_environment (struct ndm_session *sess,
  ndmp4_pval *env4, unsigned n_env)
{
	struct ndm_env_table	envtab;
	int			rc;

	NDMOS_MACRO_ZEROFILL (&envtab);

	if (n_env > NDM_MAX_ENV)
		return NDMP9_ILLEGAL_ARGS_ERR;

	ndmp_4to9_pval_vec (env4, envtab.env, n_env);
	envtab.n_env = n_env;

	rc = ndmda_copy_environment (sess, envtab.env, envtab.n_env);
	if (rc != 0)
		return NDMP9_NO_MEM_ERR;

	return NDMP9_NO_ERR;
}

static ndmp9_error
data_copy_v4_nlist (struct ndm_session *sess,
  ndmp4_name *nlist4, unsigned n_nlist)
{
	struct ndm_nlist_table	nlisttab;
	int			rc;

	NDMOS_MACRO_ZEROFILL (&nlisttab);

	if (n_nlist > NDM_MAX_NLIST)
		return NDMP9_ILLEGAL_ARGS_ERR;

	ndmp_4to9_name_vec (nlist4, nlisttab.nlist, n_nlist);
	nlisttab.n_nlist = n_nlist;

	rc = ndmda_copy_nlist (sess, nlisttab.nlist, nlisttab.n_nlist);
	if (rc != 0)
		return NDMP9_NO_MEM_ERR;

	return NDMP9_NO_ERR;
}

#endif /* !NDMOS_OPTION_NO_NDMP4 */


#endif /* !NDMOS_OPTION_NO_DATA_AGENT */	/* Surrounds DATA intfs */




#ifndef NDMOS_OPTION_NO_TAPE_AGENT		/* Surrounds MOVER intfs */
/*
 * NDMPx_MOVER Interfaces
 ****************************************************************
 */

static ndmp9_error	mover_can_proceed (struct ndm_session *sess,
				int will_write);

static int		mover_listen_common234 (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn,
				ndmp9_addr_type addr_type,
				ndmp9_mover_mode mover_mode);

static int		mover_continue_common234 (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn);

static int		mover_abort_common234 (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn);

static int		mover_stop_common234 (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn);

static int		mover_set_window_common234 (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn,
				ndmp9_u_quad offset,
				ndmp9_u_quad length);

static int		mover_read_common234 (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn,
				ndmp9_u_quad offset,
				ndmp9_u_quad length);

static int		mover_close_common234 (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn);

static int		mover_set_record_size_common234 (
			        struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn,
				u_long record_size);

#ifndef NDMOS_EFFECT_NO_NDMP3_NO_NDMP4
static int		mover_connect_common34 (struct ndm_session *sess,
				struct ndmp_xa_buf *xa,
				struct ndmconn *ref_conn,
				ndmp9_addr *addr,
				ndmp9_mover_mode mover_mode);
#endif /* !NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4 */





/*
 * NDMP[234]_MOVER_GET_STATE
 */
int
ndmadr_mover_get_state (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_tape_agent *	ta = &sess->tape_acb;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_mover_get_state)
	ndmta_mover_sync_state(sess);
	ndmp_9to2_mover_get_state_reply (&ta->mover_state, reply);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_mover_get_state)
	ndmta_mover_sync_state(sess);
	ndmp_9to3_mover_get_state_reply (&ta->mover_state, reply);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_mover_get_state)
	ndmta_mover_sync_state(sess);
	ndmp_9to4_mover_get_state_reply (&ta->mover_state, reply);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_MOVER_LISTEN
 */
int
ndmadr_mover_listen (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_tape_agent *	ta = &sess->tape_acb;
    ndmp9_addr_type		addr_type;
    ndmp9_mover_mode		mover_mode;
    int				rc;

    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_mover_listen)
	/* Check args, map along the way */
	switch (request->mode) {
	default:		mover_mode = -1; break;
	case NDMP2_MOVER_MODE_READ: mover_mode = NDMP9_MOVER_MODE_READ; break;
	case NDMP2_MOVER_MODE_WRITE:mover_mode = NDMP9_MOVER_MODE_WRITE;break;
	}
	switch (request->addr_type) {
	default:		addr_type = -1; break;
	case NDMP2_ADDR_LOCAL:	addr_type = NDMP9_ADDR_LOCAL;	break;
	case NDMP2_ADDR_TCP:	addr_type = NDMP9_ADDR_TCP;	break;
	}

	rc = mover_listen_common234(sess, xa, ref_conn, addr_type, mover_mode);
	if (rc)
		return rc;	/* something went wrong */

	ndmp_9to2_mover_addr (&ta->mover_state.data_connection_addr,
				&reply->mover);
	/* reply->error already set to NDMPx_NO_ERROR */
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_mover_listen)
	/* Check args, map along the way */
	switch (request->mode) {
	default:		mover_mode = -1; break;
	case NDMP3_MOVER_MODE_READ: mover_mode = NDMP9_MOVER_MODE_READ; break;
	case NDMP3_MOVER_MODE_WRITE:mover_mode = NDMP9_MOVER_MODE_WRITE;break;
	}
	switch (request->addr_type) {
	default:		addr_type = -1; break;
	case NDMP3_ADDR_LOCAL:	addr_type = NDMP9_ADDR_LOCAL;	break;
	case NDMP3_ADDR_TCP:	addr_type = NDMP9_ADDR_TCP;	break;
	}

	rc = mover_listen_common234(sess, xa, ref_conn, addr_type, mover_mode);
	if (rc)
		return rc;	/* something went wrong */

	ndmp_9to3_addr (&ta->mover_state.data_connection_addr,
				&reply->data_connection_addr);
	/* reply->error already set to NDMPx_NO_ERROR */
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_mover_listen)
	/* Check args, map along the way */
	switch (request->mode) {
	default:		mover_mode = -1; break;
	case NDMP4_MOVER_MODE_READ: mover_mode = NDMP9_MOVER_MODE_READ; break;
	case NDMP4_MOVER_MODE_WRITE:mover_mode = NDMP9_MOVER_MODE_WRITE;break;
	}
	switch (request->addr_type) {
	default:		addr_type = -1; break;
	case NDMP4_ADDR_LOCAL:	addr_type = NDMP9_ADDR_LOCAL;	break;
	case NDMP4_ADDR_TCP:	addr_type = NDMP9_ADDR_TCP;	break;
	}

	rc = mover_listen_common234(sess, xa, ref_conn, addr_type, mover_mode);
	if (rc)
		return rc;	/* something went wrong */

	ndmp_9to4_addr (&ta->mover_state.data_connection_addr,
				&reply->data_connection_addr);
	/* reply->error already set to NDMPx_NO_ERROR */
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

static int
mover_listen_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  ndmp9_addr_type addr_type, ndmp9_mover_mode mover_mode)
{
#ifndef NDMOS_OPTION_NO_DATA_AGENT
	struct ndm_data_agent *	da = &sess->data_acb;
#endif /* !NDMOS_OPTION_NO_DATA_AGENT */
	struct ndm_tape_agent *	ta = &sess->tape_acb;
	ndmp9_error		error;
	int			will_write;
	char			reason[100];

	ndmalogf (sess, 0, 6, "mover_listen_common() addr_type=%s mode=%s",
			ndmp9_addr_type_to_str (addr_type),
			ndmp9_mover_mode_to_str (mover_mode));

	/* Check args */
	switch (mover_mode) {
	default:		NDMADR_RAISE_ILLEGAL_ARGS("mover_mode");
	case NDMP9_MOVER_MODE_READ:
		will_write = 1;
		break;

	case NDMP9_MOVER_MODE_WRITE:
		will_write = 0;
		break;
	}

	switch (addr_type) {
	default:		NDMADR_RAISE_ILLEGAL_ARGS("mover_addr_type");
	case NDMP9_ADDR_LOCAL:
#ifdef NDMOS_OPTION_NO_DATA_AGENT
		NDMADR_RAISE_ILLEGAL_ARGS("mover LOCAL w/o local DATA agent");
#endif /* NDMOS_OPTION_NO_DATA_AGENT */
		break;

	case NDMP9_ADDR_TCP:
		break;
	}

	/* Check states -- this should cover everything */
	if (ta->mover_state.state != NDMP9_MOVER_STATE_IDLE)
		NDMADR_RAISE_ILLEGAL_STATE("mover_state !IDLE");
#ifndef NDMOS_OPTION_NO_DATA_AGENT
	if (da->data_state.state != NDMP9_DATA_STATE_IDLE)
		NDMADR_RAISE_ILLEGAL_STATE("data_state !IDLE");
#endif /* !NDMOS_OPTION_NO_DATA_AGENT */

	/* Check that the tape is ready to go */
	error = mover_can_proceed (sess, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!mover_can_proceed");

	/*
	 * Check image stream state -- should already be reflected
	 * in the mover and data states. This extra check gives
	 * us an extra measure of robustness and sanity
	 * check on the implementation.
	 */
	error = ndmis_audit_tape_listen (sess, addr_type, reason);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, reason);

	error = ndmis_tape_listen (sess, addr_type,
			&ta->mover_state.data_connection_addr,
			reason);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, reason);

	error = ndmta_mover_listen(sess, mover_mode);
	if (error != NDMP9_NO_ERR) {
		/* TODO: belay ndmis_tape_listen() */
		NDMADR_RAISE(error, "!mover_listen");
	}

	return 0;
}




/*
 * NDMP[234]_MOVER_CONTINUE
 */
int
ndmadr_mover_continue (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_mover_continue)
	return mover_continue_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_mover_continue)
	return mover_continue_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_mover_continue)
	return mover_continue_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

static int
mover_continue_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
	struct ndm_tape_agent *	ta = &sess->tape_acb;
	ndmp9_error		error;
	int			will_write;

	if (ta->mover_state.state != NDMP9_MOVER_STATE_PAUSED)
		NDMADR_RAISE_ILLEGAL_STATE("mover_state !PAUSED");

	will_write = ta->mover_state.mode == NDMP9_MOVER_MODE_READ;

	error = mover_can_proceed (sess, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!mover_can_proceed");

	ndmta_mover_continue (sess);

	return 0;
}




/*
 * NDMP[234]_MOVER_ABORT
 */
int
ndmadr_mover_abort (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_mover_abort)
	return mover_abort_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_mover_abort)
	return mover_abort_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_mover_abort)
	return mover_abort_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

static int
mover_abort_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
	struct ndm_tape_agent *	ta = &sess->tape_acb;

	if (ta->mover_state.state != NDMP9_MOVER_STATE_LISTEN
	 && ta->mover_state.state != NDMP9_MOVER_STATE_ACTIVE
	 && ta->mover_state.state != NDMP9_MOVER_STATE_PAUSED)
		NDMADR_RAISE_ILLEGAL_STATE("mover_state");

	ndmta_mover_abort (sess);

	return 0;
}




/*
 * NDMP[234]_MOVER_STOP
 */
int
ndmadr_mover_stop (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_mover_stop)
	return mover_stop_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_mover_stop)
	return mover_stop_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_mover_stop)
	return mover_stop_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

static int
mover_stop_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
	struct ndm_tape_agent *	ta = &sess->tape_acb;

	if (ta->mover_state.state != NDMP9_MOVER_STATE_HALTED)
		NDMADR_RAISE_ILLEGAL_STATE("mover_state !HALTED");

	ndmta_mover_stop (sess);

	return 0;
}




/*
 * NDMP[234]_MOVER_SET_WINDOW
 */
int
ndmadr_mover_set_window (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_mover_set_window)
	return mover_set_window_common234 (sess, xa, ref_conn,
			request->offset, request->length);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_mover_set_window)
	return mover_set_window_common234 (sess, xa, ref_conn,
			request->offset, request->length);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_mover_set_window)
	return mover_set_window_common234 (sess, xa, ref_conn,
			request->offset, request->length);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

static int
mover_set_window_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  ndmp9_u_quad offset, ndmp9_u_quad length)
{
	struct ndm_tape_agent *	ta = &sess->tape_acb;
	struct ndmp9_mover_get_state_reply *ms = &ta->mover_state;
	unsigned long long	max_len;
	unsigned long long	end_win;

	ndmta_mover_sync_state (sess);

	if (ms->state != NDMP9_MOVER_STATE_IDLE
	 && ms->state != NDMP9_MOVER_STATE_PAUSED)
		NDMADR_RAISE_ILLEGAL_STATE("mover_state !IDLE/PAUSED");

	if (offset % ms->record_size != 0)
		NDMADR_RAISE_ILLEGAL_ARGS("off !record_size");

	if (length != NDMP_LENGTH_INFINITY) {
		if (length % ms->record_size != 0)
			NDMADR_RAISE_ILLEGAL_ARGS("len !record_size");
#if 0
		/* Too pedantic. Sometimes needed (like for testing) */
		if (length == 0)
			NDMADR_RAISE_ILLEGAL_ARGS("length 0");
#endif

		max_len = NDMP_LENGTH_INFINITY - offset;
		max_len -= max_len % ms->record_size;
		if (length > max_len)  /* learn math fella */
			NDMADR_RAISE_ILLEGAL_ARGS("length too long");
		end_win = offset + length;
	} else {
		end_win = NDMP_LENGTH_INFINITY;
	}
	ms->window_offset = offset;
	ms->window_length = length;
	ta->mover_window_end = end_win;

	return 0;
}




/*
 * NDMP[234]_MOVER_READ
 */
int
ndmadr_mover_read (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_mover_read)
	return mover_read_common234 (sess, xa, ref_conn,
			request->offset, request->length);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_mover_read)
	return mover_read_common234 (sess, xa, ref_conn,
			request->offset, request->length);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_mover_read)
	return mover_read_common234 (sess, xa, ref_conn,
			request->offset, request->length);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

static int
mover_read_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  ndmp9_u_quad offset, ndmp9_u_quad length)
{
	struct ndm_tape_agent *	ta = &sess->tape_acb;
	struct ndmp9_mover_get_state_reply *ms = &ta->mover_state;

	ndmta_mover_sync_state (sess);

	if (ms->state != NDMP9_MOVER_STATE_ACTIVE)
		NDMADR_RAISE_ILLEGAL_STATE("mover_state !ACTIVE");

	if (ms->bytes_left_to_read > 0)
		NDMADR_RAISE_ILLEGAL_STATE("byte_left_to_read");

	if (ms->data_connection_addr.addr_type != NDMP9_ADDR_TCP)
		NDMADR_RAISE_ILLEGAL_STATE("mover_addr !TCP");

	if (ms->mode != NDMP9_MOVER_MODE_WRITE)
		NDMADR_RAISE_ILLEGAL_STATE("mover_mode !WRITE");

	ndmta_mover_read (sess, offset, length);

	return 0;
}




/*
 * NDMP[234]_MOVER_CLOSE
 */
int
ndmadr_mover_close (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_VOID_REQUEST(ndmp2_mover_close)
	return mover_close_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_VOID_REQUEST(ndmp3_mover_close)
	return mover_close_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_VOID_REQUEST(ndmp4_mover_close)
	return mover_close_common234 (sess, xa, ref_conn);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

static int
mover_close_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
	struct ndm_tape_agent *	ta = &sess->tape_acb;

	if (ta->mover_state.state == NDMP9_MOVER_STATE_IDLE)
		NDMADR_RAISE_ILLEGAL_STATE("mover_state !IDLE");

	ndmta_mover_close (sess);

	return 0;
}




/*
 * NDMP[234]_MOVER_SET_RECORD_SIZE
 */
int
ndmadr_mover_set_record_size (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH(ndmp2_mover_set_record_size)
	return mover_set_record_size_common234 (sess, xa, ref_conn,
			request->len);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_mover_set_record_size)
	return mover_set_record_size_common234 (sess, xa, ref_conn,
			request->len);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_mover_set_record_size)
	return mover_set_record_size_common234 (sess, xa, ref_conn,
			request->len);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

static int
mover_set_record_size_common234 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn, u_long record_size)
{
	struct ndm_tape_agent *	ta = &sess->tape_acb;
	struct ndmp9_mover_get_state_reply *ms = &ta->mover_state;

	ndmta_mover_sync_state (sess);

	if (ms->state != NDMP9_MOVER_STATE_IDLE
	 && ms->state != NDMP9_MOVER_STATE_PAUSED)
		NDMADR_RAISE_ILLEGAL_STATE("mover_state !IDLE/PAUSED");

	if (!NDMOS_MACRO_OK_TAPE_REC_LEN(record_size))
		NDMADR_RAISE_ILLEGAL_ARGS("!ok_tape_rec_len");

	ta->mover_state.record_size = record_size;

	return 0;
}



#ifndef NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4	/* Surrounds NDMPv[34] MOVER intfs */
/*
 * NDMP[34]_MOVER_CONNECT
 */
int
ndmadr_mover_connect (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	/* not part of NDMPv2 */
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH(ndmp3_mover_connect)
	ndmp9_mover_mode	mover_mode;
	ndmp9_addr		data_addr;

	switch (request->mode) {
	default:		mover_mode = -1; break;
	case NDMP3_MOVER_MODE_READ: mover_mode = NDMP9_MOVER_MODE_READ; break;
	case NDMP3_MOVER_MODE_WRITE:mover_mode = NDMP9_MOVER_MODE_WRITE;break;
	}

	ndmp_3to9_addr (&request->addr, &data_addr);

	return mover_connect_common34 (sess, xa, ref_conn,
				&data_addr, mover_mode);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH(ndmp4_mover_connect)
	ndmp9_mover_mode	mover_mode;
	ndmp9_addr		data_addr;

	switch (request->mode) {
	default:		mover_mode = -1; break;
	case NDMP4_MOVER_MODE_READ: mover_mode = NDMP9_MOVER_MODE_READ; break;
	case NDMP4_MOVER_MODE_WRITE:mover_mode = NDMP9_MOVER_MODE_WRITE;break;
	}

	ndmp_4to9_addr (&request->addr, &data_addr);

	return mover_connect_common34 (sess, xa, ref_conn,
				&data_addr, mover_mode);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}

/* this same intf is expected in v4, so _common() now */
static int
mover_connect_common34 (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn,
  ndmp9_addr *data_addr, ndmp9_mover_mode mover_mode)
{
#ifndef NDMOS_OPTION_NO_DATA_AGENT
	struct ndm_data_agent *	da = &sess->data_acb;
#endif /* !NDMOS_OPTION_NO_DATA_AGENT */
	struct ndm_tape_agent *	ta = &sess->tape_acb;
	ndmp9_error		error;
	int			will_write;
	char			reason[100];

	/* Check args */
	switch (mover_mode) {
	default:		NDMADR_RAISE_ILLEGAL_ARGS("mover_mode");
	case NDMP9_MOVER_MODE_READ:
		will_write = 1;
		break;

	case NDMP9_MOVER_MODE_WRITE:
		will_write = 0;
		break;
	}

	switch (data_addr->addr_type) {
	default:		NDMADR_RAISE_ILLEGAL_ARGS("mover_addr_type");
	case NDMP9_ADDR_LOCAL:
#ifdef NDMOS_OPTION_NO_DATA_AGENT
		NDMADR_RAISE_ILLEGAL_ARGS("mover LOCAL w/o local DATA agent");
#endif /* NDMOS_OPTION_NO_DATA_AGENT */
		break;

	case NDMP9_ADDR_TCP:
		break;
	}

	/* Check states -- this should cover everything */
	if (ta->mover_state.state != NDMP9_MOVER_STATE_IDLE)
		NDMADR_RAISE_ILLEGAL_STATE("mover_state !IDLE");
#ifndef NDMOS_OPTION_NO_DATA_AGENT
	if (data_addr->addr_type == NDMP9_ADDR_LOCAL) {
		ndmp9_data_get_state_reply *ds = &da->data_state;

		if (ds->state != NDMP9_DATA_STATE_LISTEN)
			NDMADR_RAISE_ILLEGAL_STATE("data_state !LISTEN");

		if (ds->data_connection_addr.addr_type != NDMP9_ADDR_LOCAL)
			NDMADR_RAISE_ILLEGAL_STATE("data_addr !LOCAL");
	} else {
		if (da->data_state.state != NDMP9_DATA_STATE_IDLE)
			NDMADR_RAISE_ILLEGAL_STATE("data_state !IDLE");
	}
#endif /* !NDMOS_OPTION_NO_DATA_AGENT */

	/* Check that the tape is ready to go */
	error = mover_can_proceed (sess, will_write);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, "!mover_can_proceed");

	/*
	 * Check image stream state -- should already be reflected
	 * in the mover and data states. This extra check gives
	 * us an extra measure of robustness and sanity
	 * check on the implementation.
	 */
	error = ndmis_audit_tape_connect (sess, data_addr->addr_type, reason);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, reason);

	error = ndmis_tape_connect (sess, data_addr, reason);
	if (error != NDMP9_NO_ERR) NDMADR_RAISE(error, reason);

	ta->mover_state.data_connection_addr = *data_addr;
	/* alt: ta->....data_connection_addr = sess->...peer_addr */

	error = ndmta_mover_connect (sess, mover_mode);
	if (error != NDMP9_NO_ERR) {
		/* TODO: belay ndmis_tape_connect() */
		NDMADR_RAISE(error, "!mover_connect");
	}

	return 0;
}
#endif /* !NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4  Surrounds NDMPv[34] MOVER intfs */




/*
 * NDMPx_MOVER helper routines
 */

/*
 * MOVER can only proceed from IDLE->LISTEN or PAUSED->ACTIVE
 * if the tape drive is ready.
 */

static ndmp9_error
mover_can_proceed (struct ndm_session *sess, int will_write)
{
	struct ndm_tape_agent *		ta = &sess->tape_acb;

	ndmos_tape_sync_state(sess);
	if (ta->tape_state.state != NDMP9_TAPE_STATE_OPEN)
		return NDMP9_DEV_NOT_OPEN_ERR;

	if (will_write && !NDMTA_TAPE_IS_WRITABLE(ta))
		return NDMP9_PERMISSION_ERR;

	return NDMP9_NO_ERR;
}

#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */	/* Surrounds MOVER intfs */




#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds NOTIFY intfs */
/*
 * NDMPx_NOTIFY Interfaces
 ****************************************************************
 */




/*
 * NDMP[234]_NOTIFY_DATA_HALTED
 */
int
ndmadr_notify_data_halted (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_control_agent *	ca = &sess->control_acb;

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_NO_REPLY(ndmp2_notify_data_halted)
	ca->pending_notify_data_halted++;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_NO_REPLY(ndmp3_notify_data_halted)
	ca->pending_notify_data_halted++;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_POST(ndmp4_notify_data_halted)
	ca->pending_notify_data_halted++;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_NOTIFY_CONNECTED
 */
int
ndmadr_notify_connected (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_NO_REPLY(ndmp2_notify_connected)
	/* Just ignore? */
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_NO_REPLY(ndmp3_notify_connected)
	/* Just ignore? */
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_POST(ndmp4_notify_connection_status)
	/* Just ignore? */
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_NOTIFY_MOVER_HALTED
 */
int
ndmadr_notify_mover_halted (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_control_agent *	ca = &sess->control_acb;

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_NO_REPLY(ndmp2_notify_mover_halted)
	ca->pending_notify_mover_halted++;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_NO_REPLY(ndmp3_notify_mover_halted)
	ca->pending_notify_mover_halted++;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_POST(ndmp4_notify_mover_halted)
	ca->pending_notify_mover_halted++;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_NOTIFY_MOVER_PAUSED
 */
int
ndmadr_notify_mover_paused (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_control_agent *	ca = &sess->control_acb;

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_NO_REPLY(ndmp2_notify_mover_paused)
	ca->pending_notify_mover_paused++;
	ca->last_notify_mover_paused.reason = request->reason;
	ca->last_notify_mover_paused.seek_position = request->seek_position;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_NO_REPLY(ndmp3_notify_mover_paused)
	ca->pending_notify_mover_paused++;
	ca->last_notify_mover_paused.reason = request->reason;
	ca->last_notify_mover_paused.seek_position = request->seek_position;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_POST(ndmp4_notify_mover_paused)
	ca->pending_notify_mover_paused++;
	ca->last_notify_mover_paused.reason = request->reason;
	ca->last_notify_mover_paused.seek_position = request->seek_position;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[234]_NOTIFY_DATA_READ
 */
int
ndmadr_notify_data_read (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_control_agent *	ca = &sess->control_acb;

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_NO_REPLY(ndmp2_notify_data_read)
	ca->pending_notify_data_read++;
	ca->last_notify_data_read.offset = request->offset;
	ca->last_notify_data_read.length = request->length;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_NO_REPLY(ndmp3_notify_data_read)
	ca->pending_notify_data_read++;
	ca->last_notify_data_read.offset = request->offset;
	ca->last_notify_data_read.length = request->length;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_POST(ndmp4_notify_data_read)
	ca->pending_notify_data_read++;
	ca->last_notify_data_read.offset = request->offset;
	ca->last_notify_data_read.length = request->length;
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds NOTIFY intfs */




#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds LOG intfs */
/*
 * NDMPx_LOG Interfaces
 ****************************************************************
 */




#ifndef NDMOS_OPTION_NO_NDMP2		/* Surrounds NDMPv2 LOG intfs */
/*
 * NDMP2_LOG_LOG
 */
int
ndmadr_log_log (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    char			prefix[32];

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_NO_REPLY(ndmp2_log_log)
	sprintf (prefix, "%cL", ref_conn->chan.name[1]);
	ndmalogf (sess, prefix, 1, "%s", request->entry);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}



/*
 * NDMP2_LOG_DEBUG
 */
int
ndmadr_log_debug (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    char			prefix[32];

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_NO_REPLY(ndmp2_log_debug)
	sprintf (prefix, "%cLD%s",
		ref_conn->chan.name[1],
		ndma_log_dbg_tag(request->level));
	ndmalogf (sess, prefix, 2, "%s", request->message);
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}
#endif /* !NDMOS_OPTION_NO_NDMP2 */	/* Surrounds NDMPv2 LOG intfs */



/*
 * NDMP[234]_LOG_FILE
 */
int
ndmadr_log_file (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    char			prefix[32];

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_NO_REPLY(ndmp2_log_file)
	sprintf (prefix, "%cLF", ref_conn->chan.name[1]);
	if (request->error == NDMP2_NO_ERR)
		ndmalogf (sess, prefix, 1, "%s", request->name);
	else
		ndmalogf (sess, prefix, 0, "%s (%s)",
			request->name, ndmp2_error_to_str(request->error));
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_NO_REPLY(ndmp3_log_file)
	sprintf (prefix, "%cLF", ref_conn->chan.name[1]);
	if (request->error == NDMP3_NO_ERR)
		ndmalogf (sess, prefix, 1, "%s", request->name);
	else
		ndmalogf (sess, prefix, 0, "%s (%s)",
			request->name, ndmp3_error_to_str(request->error));
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_POST(ndmp4_log_file)
	sprintf (prefix, "%cLF", ref_conn->chan.name[1]);
#if 0
	if (request->error == NDMP4_NO_ERR)
		ndmalogf (sess, prefix, 1, "%s", request->name);
	else
		ndmalogf (sess, prefix, 0, "%s (%s)",
			request->name, ndmp4_error_to_str(request->error));
#endif
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




#ifndef NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4	/* Surrounds NDMPv[34] LOG intfs */

/*
 * NDMP[34]_LOG_MESSAGE
 */
int
ndmadr_log_message (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    char			prefix[32];

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_NO_REPLY(ndmp3_log_message)
	sprintf (prefix, "%cLM%s",
		ref_conn->chan.name[1],
		ndma_log_message_tag_v3(request->log_type));

	ndmalogf (sess, prefix, 0, "%s", request->entry); /* omit message_id */
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_POST(ndmp4_log_message)
	sprintf (prefix, "%cLM%s",
		ref_conn->chan.name[1],
		ndma_log_message_tag_v4(request->log_type));

	ndmalogf (sess, prefix, 0, "%s", request->entry); /* omit message_id */
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}
#endif /* !NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4  Surrounds NDMPv[34] LOG intfs */
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds LOG intfs */




#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds FH intfs */
/*
 * NDMPx_FH Interfaces
 ****************************************************************
 */




#ifndef NDMOS_OPTION_NO_NDMP2		/* Surrounds NDMPv2 FH intfs */
/*
 * NDMP2_FH_ADD_UNIX_PATH
 */
int
ndmadr_fh_add_unix_path (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_control_agent *	ca = &sess->control_acb;
    struct ndmlog *		ixlog = &ca->job.index_log;
    int				i;
    char			prefix[32];
    char			statbuf[100], namebuf[NDMOS_CONST_PATH_MAX];

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_NO_REPLY(ndmp2_fh_add_unix_path)
	ndmp2_fh_unix_path *	pp;

	sprintf (prefix, "%cHf", ref_conn->chan.name[1]);
	for (i = 0; i < request->paths.paths_len; i++) {
		ndmp9_file_stat		filestat;

		pp = &request->paths.paths_val[i];

		ndmp_2to9_unix_file_stat (&pp->fstat, &filestat,
						NDMP_INVALID_U_QUAD);
		ndma_file_stat_to_str (&filestat, statbuf);
		ndmcstr_from_str (pp->name, namebuf, sizeof namebuf);

		ndmlogf (ixlog, prefix, 0, "%s UNIX %s", namebuf, statbuf);
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP2_FH_ADD_UNIX_DIR
 */
int
ndmadr_fh_add_unix_dir (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_control_agent *	ca = &sess->control_acb;
    struct ndmlog *		ixlog = &ca->job.index_log;
    int				i;
    char			prefix[32];
    char			namebuf[NDMOS_CONST_PATH_MAX];

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_NO_REPLY(ndmp2_fh_add_unix_dir)
	ndmp2_fh_unix_dir *	dp;

	sprintf (prefix, "%cHd", ref_conn->chan.name[1]);
	for (i = 0; i < request->dirs.dirs_len; i++) {
		dp = &request->dirs.dirs_val[i];

		ndmcstr_from_str (dp->name, namebuf, sizeof namebuf);

		ndmlogf (ixlog, prefix, 0, "%u %s UNIX %u",
				dp->parent, namebuf, dp->node);
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP2_FH_ADD_UNIX_NODE
 */
int
ndmadr_fh_add_unix_node (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_control_agent *	ca = &sess->control_acb;
    struct ndmlog *		ixlog = &ca->job.index_log;
    int				i;
    char			prefix[32], statbuf[100];

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
      NDMS_WITH_NO_REPLY(ndmp2_fh_add_unix_node)
	ndmp2_fh_unix_node *	np;

	sprintf (prefix, "%cHn", ref_conn->chan.name[1]);
	for (i = 0; i < request->nodes.nodes_len; i++) {
		ndmp9_file_stat		filestat;

		np = &request->nodes.nodes_val[i];

		ndmp_2to9_unix_file_stat (&np->fstat, &filestat,
						NDMP_INVALID_U_QUAD);
		ndma_file_stat_to_str (&filestat, statbuf);

		ndmlogf (ixlog, prefix, 0, "%u UNIX %s", np->node, statbuf);
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}
#endif /* !NDMOS_OPTION_NO_NDMP2 */	/* Surrounds NDMPv2 FH intfs */




#ifndef NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4	/* Surrounds NDMPv[34] FH intfs */
/*
 * NDMP[34]_FH_ADD_FILE
 */
int
ndmadr_fh_add_file (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_control_agent *	ca = &sess->control_acb;
    struct ndmlog *		ixlog = &ca->job.index_log;
    char			prefix[32];
    char			statbuf[100];
    char			namebuf[NDMOS_CONST_PATH_MAX];
    char			dos_buf[NDMOS_CONST_PATH_MAX];

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_NO_REPLY(ndmp3_fh_add_file)
	int			i, j, k;
	ndmp3_file *		file;
	ndmp3_file_name *	f_name;
	ndmp3_file_stat *	f_stat;

	sprintf (prefix, "%cHf", ref_conn->chan.name[1]);
	/*
	 * equijoin between names[] and stats[] based on fs_type
	 */
	for (i = 0; i < request->files.files_len; i++) {
	  file = &request->files.files_val[i];
	  for (j = 0; j < file->names.names_len; j++) {
	    f_name = &file->names.names_val[j];
	    for (k = 0; k < file->stats.stats_len; k++) {
		ndmp9_file_stat		filestat;
		char *			raw_name;

		f_stat = &file->stats.stats_val[k];

		if (f_name->fs_type != f_stat->fs_type)
			continue;

		ndmp_3to9_file_stat (f_stat, &filestat,
						file->node, file->fh_info);
		/* begin blechy */
		filestat.fh_info.valid = NDMP9_VALIDITY_VALID;
		filestat.fh_info.value = file->fh_info;
		if (filestat.fh_info.value == NDMP_FH_INVALID_FH_INFO)
			filestat.fh_info.valid = NDMP9_VALIDITY_INVALID;
		/* end blechy */
		ndma_file_stat_to_str (&filestat, statbuf);

		switch (f_name->fs_type) {
		default:
			raw_name = f_name->ndmp3_file_name_u.other_name;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);
			ndmlogf (ixlog, prefix, 0, "%s FST%d %s",
				namebuf, f_name->fs_type, statbuf);
			break;

		case NDMP3_FS_UNIX:
			raw_name = f_name->ndmp3_file_name_u.unix_name;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);
			ndmlogf (ixlog, prefix, 0, "%s UNIX %s",
				namebuf, statbuf);
			break;

		case NDMP3_FS_NT:
			raw_name = f_name->ndmp3_file_name_u.nt_name.nt_path;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);

			raw_name = f_name->ndmp3_file_name_u.nt_name.dos_path;
			ndmcstr_from_str (raw_name, dos_buf, sizeof dos_buf);

			ndmlogf (ixlog, prefix, 0, "%s NT %s %s",
				namebuf, statbuf, dos_buf);
			break;
		}
	    }
	  }
	  /* TODO: pick up straglers which didn't equijoin */
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_POST(ndmp4_fh_add_file)
	int			i, j, k;
	ndmp4_file *		file;
	ndmp4_file_name *	f_name;
	ndmp4_file_stat *	f_stat;

	sprintf (prefix, "%cHf", ref_conn->chan.name[1]);
	/*
	 * equijoin between names[] and stats[] based on fs_type
	 */
	for (i = 0; i < request->files.files_len; i++) {
	  file = &request->files.files_val[i];
	  for (j = 0; j < file->names.names_len; j++) {
	    f_name = &file->names.names_val[j];
	    for (k = 0; k < file->stats.stats_len; k++) {
		ndmp9_file_stat		filestat;
		char *			raw_name;

		f_stat = &file->stats.stats_val[k];

		if (f_name->fs_type != f_stat->fs_type)
			continue;

		ndmp_4to9_file_stat (f_stat, &filestat,
						file->node, file->fh_info);
		/* begin blechy */
		filestat.fh_info.valid = NDMP9_VALIDITY_VALID;
		filestat.fh_info.value = file->fh_info;
		if (filestat.fh_info.value == NDMP_FH_INVALID_FH_INFO)
			filestat.fh_info.valid = NDMP9_VALIDITY_INVALID;
		/* end blechy */
		ndma_file_stat_to_str (&filestat, statbuf);

		switch (f_name->fs_type) {
		default:
			raw_name = f_name->ndmp4_file_name_u.other_name;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);
			ndmlogf (ixlog, prefix, 0, "%s FST%d %s",
				namebuf, f_name->fs_type, statbuf);
			break;

		case NDMP4_FS_UNIX:
			raw_name = f_name->ndmp4_file_name_u.unix_name;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);
			ndmlogf (ixlog, prefix, 0, "%s UNIX %s",
				namebuf, statbuf);
			break;

		case NDMP4_FS_NT:
			raw_name = f_name->ndmp4_file_name_u.nt_name.nt_path;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);

			raw_name = f_name->ndmp4_file_name_u.nt_name.dos_path;
			ndmcstr_from_str (raw_name, dos_buf, sizeof dos_buf);

			ndmlogf (ixlog, prefix, 0, "%s NT %s %s",
				namebuf, statbuf, dos_buf);
			break;
		}
	    }
	  }
	  /* TODO: pick up straglers which didn't equijoin */
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */

    }
    return 0;
}




/*
 * NDMP[34]_FH_ADD_DIR
 */
int
ndmadr_fh_add_dir (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_control_agent *	ca = &sess->control_acb;
    struct ndmlog *		ixlog = &ca->job.index_log;
    char			prefix[32];
    char			namebuf[NDMOS_CONST_PATH_MAX];
    char			dos_buf[NDMOS_CONST_PATH_MAX];

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_NO_REPLY(ndmp3_fh_add_dir)
	int			i, j;
	ndmp3_dir *		dir;
	ndmp3_file_name *	f_name;

	sprintf (prefix, "%cHd", ref_conn->chan.name[1]);

	for (i = 0; i < request->dirs.dirs_len; i++) {
	  dir = &request->dirs.dirs_val[i];
	  for (j = 0; j < dir->names.names_len; j++) {
	    f_name = &dir->names.names_val[j];
	    {
		char *			raw_name;

		switch (f_name->fs_type) {
		default:
			raw_name = f_name->ndmp3_file_name_u.other_name;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);
			ndmlogf (ixlog, prefix, 0, "%llu %s FST%d %llu",
				dir->parent, namebuf,
				f_name->fs_type, dir->node);
			break;

		case NDMP3_FS_UNIX:
			raw_name = f_name->ndmp3_file_name_u.unix_name;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);
			ndmlogf (ixlog, prefix, 0, "%llu %s UNIX %llu",
				dir->parent, namebuf, dir->node);
			break;

		case NDMP3_FS_NT:
			raw_name = f_name->ndmp3_file_name_u.nt_name.nt_path;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);

			raw_name = f_name->ndmp3_file_name_u.nt_name.dos_path;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);

			ndmlogf (ixlog, prefix, 0, "%llu %s NT %llu %s",
				dir->parent, namebuf, dir->node, dos_buf);
			break;
		}
	    }
	  }
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_POST(ndmp4_fh_add_dir)
	int			i, j;
	ndmp4_dir *		dir;
	ndmp4_file_name *	f_name;

	sprintf (prefix, "%cHd", ref_conn->chan.name[1]);

	for (i = 0; i < request->dirs.dirs_len; i++) {
	  dir = &request->dirs.dirs_val[i];
	  for (j = 0; j < dir->names.names_len; j++) {
	    f_name = &dir->names.names_val[j];
	    {
		char *			raw_name;

		switch (f_name->fs_type) {
		default:
			raw_name = f_name->ndmp4_file_name_u.other_name;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);
			ndmlogf (ixlog, prefix, 0, "%llu %s FST%d %llu",
				dir->parent, namebuf,
				f_name->fs_type, dir->node);
			break;

		case NDMP4_FS_UNIX:
			raw_name = f_name->ndmp4_file_name_u.unix_name;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);
			ndmlogf (ixlog, prefix, 0, "%llu %s UNIX %llu",
				dir->parent, namebuf, dir->node);
			break;

		case NDMP4_FS_NT:
			raw_name = f_name->ndmp4_file_name_u.nt_name.nt_path;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);

			raw_name = f_name->ndmp4_file_name_u.nt_name.dos_path;
			ndmcstr_from_str (raw_name, namebuf, sizeof namebuf);

			ndmlogf (ixlog, prefix, 0, "%llu %s NT %llu %s",
				dir->parent, namebuf, dir->node, dos_buf);
			break;
		}
	    }
	  }
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}




/*
 * NDMP[34]_FH_ADD_NODE
 */
int
ndmadr_fh_add_node (struct ndm_session *sess,
  struct ndmp_xa_buf *xa, struct ndmconn *ref_conn)
{
    struct ndm_control_agent *	ca = &sess->control_acb;
    struct ndmlog *		ixlog = &ca->job.index_log;
    char			prefix[32];
    char			statbuf[100];

    xa->reply.flags |= NDMNMB_FLAG_NO_SEND;
    switch (xa->request.protocol_version) {
    default: return NDMADR_UNIMPLEMENTED_VERSION; /* should never happen */

#ifndef NDMOS_OPTION_NO_NDMP2
    case NDMP2VER:
	return NDMADR_UNSPECIFIED_MESSAGE;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
    case NDMP3VER:
      NDMS_WITH_NO_REPLY(ndmp3_fh_add_node)
	int			i, k;
	ndmp3_node *		node;
	ndmp3_file_stat *	f_stat;

	sprintf (prefix, "%cHn", ref_conn->chan.name[1]);

	for (i = 0; i < request->nodes.nodes_len; i++) {
	  node = &request->nodes.nodes_val[i];
	  {
	    for (k = 0; k < node->stats.stats_len; k++) {
		ndmp9_file_stat		filestat;

		f_stat = &node->stats.stats_val[k];

		ndmp_3to9_file_stat (f_stat, &filestat,
						node->node, node->fh_info);
		/* begin blechy */
		filestat.fh_info.valid = NDMP9_VALIDITY_VALID;
		filestat.fh_info.value = node->fh_info;
		if (filestat.fh_info.value == NDMP_FH_INVALID_FH_INFO)
			filestat.fh_info.valid = NDMP9_VALIDITY_INVALID;
		/* end blechy */
		ndma_file_stat_to_str (&filestat, statbuf);

		switch (f_stat->fs_type) {
		default:
			ndmlogf (ixlog, prefix, 0, "%llu FST%d %s",
				node->node, f_stat->fs_type, statbuf);
			break;

		case NDMP3_FS_UNIX:
			ndmlogf (ixlog, prefix, 0, "%llu UNIX %s",
				node->node, statbuf);
			break;

		case NDMP3_FS_NT:
			ndmlogf (ixlog, prefix, 0, "%llu NT %s",
				node->node, statbuf);
			break;
		}
	    }
	  }
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
    case NDMP4VER:
      NDMS_WITH_POST(ndmp4_fh_add_node)
	int			i, k;
	ndmp4_node *		node;
	ndmp4_file_stat *	f_stat;

	sprintf (prefix, "%cHn", ref_conn->chan.name[1]);

	for (i = 0; i < request->nodes.nodes_len; i++) {
	  node = &request->nodes.nodes_val[i];
	  {
	    for (k = 0; k < node->stats.stats_len; k++) {
		ndmp9_file_stat		filestat;

		f_stat = &node->stats.stats_val[k];

		ndmp_4to9_file_stat (f_stat, &filestat,
						node->node, node->fh_info);
		/* begin blechy */
		filestat.fh_info.valid = NDMP9_VALIDITY_VALID;
		filestat.fh_info.value = node->fh_info;
		if (filestat.fh_info.value == NDMP_FH_INVALID_FH_INFO)
			filestat.fh_info.valid = NDMP9_VALIDITY_INVALID;
		/* end blechy */
		ndma_file_stat_to_str (&filestat, statbuf);

		switch (f_stat->fs_type) {
		default:
			ndmlogf (ixlog, prefix, 0, "%llu FST%d %s",
				node->node, f_stat->fs_type, statbuf);
			break;

		case NDMP4_FS_UNIX:
			ndmlogf (ixlog, prefix, 0, "%llu UNIX %s",
				node->node, statbuf);
			break;

		case NDMP4_FS_NT:
			ndmlogf (ixlog, prefix, 0, "%llu NT %s",
				node->node, statbuf);
			break;
		}
	    }
	  }
	}
      NDMS_ENDWITH
      break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
    }
    return 0;
}
#endif /* !NDMOS_EFFECT_NO_NDMP3_NOR_NDMP4  Surrounds NDMPv[34] FH intfs */
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds FH intfs */




/*
 * Common helper interfaces
 ****************************************************************
 * These do complicated state checks which are called from
 * several of the interfaces above.
 */









#ifndef NDMOS_OPTION_NO_TAPE_AGENT
/*
 * Shortcut for DATA->MOVER READ requests when NDMP9_ADDR_LOCAL
 * (local MOVER). This is implemented here because of the
 * state (sanity) checks. This should track the
 * NDMP9_MOVER_READ stanza in ndma_dispatch_request().
 */

int
ndmta_local_mover_read (struct ndm_session *sess,
  unsigned long long offset, unsigned long long length)
{
	struct ndm_tape_agent *	ta = &sess->tape_acb;
	struct ndmp9_mover_get_state_reply *ms = &ta->mover_state;
	char *			errstr = 0;

	if (ms->state != NDMP9_MOVER_STATE_ACTIVE
	 && ms->state != NDMP9_MOVER_STATE_LISTEN) {
		errstr = "mover_state !ACTIVE";
		goto senderr;
	}
	if (ms->bytes_left_to_read > 0) {
		errstr = "byte_left_to_read";
		goto senderr;
	}
	if (ms->data_connection_addr.addr_type != NDMP9_ADDR_LOCAL) {
		errstr = "mover_addr !LOCAL";
		goto senderr;
	}
	if (ms->mode != NDMP9_MOVER_MODE_WRITE) {
		errstr = "mover_mode !WRITE";
		goto senderr;
	}

	ms->seek_position = offset;
	ms->bytes_left_to_read = length;
	ta->mover_want_pos = offset;

	return 0;

  senderr:
	if (errstr) {
		ndmalogf (sess, 0, 2, "local_read error why=%s", errstr);
	}

	return -1;
}
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */







/*
 * Dispatch Version Table and Dispatch Request Tables (DVT/DRT)
 ****************************************************************
 */

struct ndm_dispatch_request_table *
ndma_drt_lookup (struct ndm_dispatch_version_table *dvt,
  unsigned protocol_version, unsigned message)
{
	struct ndm_dispatch_request_table *	drt;

	for (; dvt->protocol_version >= 0; dvt++) {
		if (dvt->protocol_version == protocol_version)
			break;
	}

	if (dvt->protocol_version < 0)
		return 0;

	for (drt = dvt->dispatch_request_table; drt->message; drt++) {
		if (drt->message == message)
			return drt;
	}

	return 0;
}

struct ndm_dispatch_request_table ndma_dispatch_request_table_v0[] = {
   { NDMP0_CONNECT_OPEN,
     NDM_DRT_FLAG_OK_NOT_CONNECTED+NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_open,
   },
   { NDMP0_CONNECT_CLOSE,
     NDM_DRT_FLAG_OK_NOT_CONNECTED+NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_close,
   },
#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds NOTIFY intfs */
   { NDMP0_NOTIFY_CONNECTED,		0, ndmadr_notify_connected, },
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds NOTIFY intfs */
   {0}
};

#ifndef NDMOS_OPTION_NO_NDMP2
struct ndm_dispatch_request_table ndma_dispatch_request_table_v2[] = {
   { NDMP2_CONNECT_OPEN,
     NDM_DRT_FLAG_OK_NOT_CONNECTED+NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_open
   },
   { NDMP2_CONNECT_CLIENT_AUTH,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_client_auth
   },
   { NDMP2_CONNECT_CLOSE,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_close
   },
   { NDMP2_CONNECT_SERVER_AUTH,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_server_auth
   },
   { NDMP2_CONFIG_GET_HOST_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_host_info
   },
   { NDMP2_CONFIG_GET_BUTYPE_ATTR,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_butype_attr
   },
   { NDMP2_CONFIG_GET_MOVER_TYPE,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_connection_type
   },
   { NDMP2_CONFIG_GET_AUTH_ATTR,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_auth_attr
   },
#ifndef NDMOS_OPTION_NO_ROBOT_AGENT		/* Surrounds SCSI intfs */
   { NDMP2_SCSI_OPEN,			0, ndmadr_scsi_open },
   { NDMP2_SCSI_CLOSE,			0, ndmadr_scsi_close },
   { NDMP2_SCSI_GET_STATE,		0, ndmadr_scsi_get_state },
   { NDMP2_SCSI_SET_TARGET,		0, ndmadr_scsi_set_target },
   { NDMP2_SCSI_RESET_DEVICE,		0, ndmadr_scsi_reset_device },
   { NDMP2_SCSI_RESET_BUS,		0, ndmadr_scsi_reset_bus },
   { NDMP2_SCSI_EXECUTE_CDB,		0, ndmadr_scsi_execute_cdb },
#endif /* !NDMOS_OPTION_NO_ROBOT_AGENT */	/* Surrounds SCSI intfs */
#ifndef NDMOS_OPTION_NO_TAPE_AGENT		/* Surrounds TAPE intfs */
   { NDMP2_TAPE_OPEN,			0, ndmadr_tape_open },
   { NDMP2_TAPE_CLOSE,			0, ndmadr_tape_close },
   { NDMP2_TAPE_GET_STATE,		0, ndmadr_tape_get_state },
   { NDMP2_TAPE_MTIO,			0, ndmadr_tape_mtio },
   { NDMP2_TAPE_WRITE,			0, ndmadr_tape_write },
   { NDMP2_TAPE_READ,			0, ndmadr_tape_read },
   { NDMP2_TAPE_EXECUTE_CDB,		0, ndmadr_tape_execute_cdb },
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */	/* Surrounds TAPE intfs */
#ifndef NDMOS_OPTION_NO_DATA_AGENT		/* Surrounds DATA intfs */
   { NDMP2_DATA_GET_STATE,		0, ndmadr_data_get_state },
   { NDMP2_DATA_START_BACKUP,		0, ndmadr_data_start_backup },
   { NDMP2_DATA_START_RECOVER,		0, ndmadr_data_start_recover },
   { NDMP2_DATA_ABORT,			0, ndmadr_data_abort },
   { NDMP2_DATA_GET_ENV,		0, ndmadr_data_get_env },
   { NDMP2_DATA_STOP,			0, ndmadr_data_stop },
   { NDMP2_DATA_START_RECOVER_FILEHIST,	0,
				ndmadr_data_start_recover_filehist },
#endif /* !NDMOS_OPTION_NO_DATA_AGENT */	/* Surrounds DATA intfs */
#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds NOTIFY intfs */
   { NDMP2_NOTIFY_DATA_HALTED,		0, ndmadr_notify_data_halted },
   { NDMP2_NOTIFY_CONNECTED,		0, ndmadr_notify_connected },
   { NDMP2_NOTIFY_MOVER_HALTED,		0, ndmadr_notify_mover_halted },
   { NDMP2_NOTIFY_MOVER_PAUSED,		0, ndmadr_notify_mover_paused },
   { NDMP2_NOTIFY_DATA_READ,		0, ndmadr_notify_data_read },
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds NOTIFY intfs */
#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds LOG intfs */
   { NDMP2_LOG_LOG,			0, ndmadr_log_log },
   { NDMP2_LOG_DEBUG,			0, ndmadr_log_debug },
   { NDMP2_LOG_FILE,			0, ndmadr_log_file },
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds LOG intfs */
#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds FH intfs */
   { NDMP2_FH_ADD_UNIX_PATH,		0, ndmadr_fh_add_unix_path },
   { NDMP2_FH_ADD_UNIX_DIR,		0, ndmadr_fh_add_unix_dir },
   { NDMP2_FH_ADD_UNIX_NODE,		0, ndmadr_fh_add_unix_node },
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds FH intfs */
#ifndef NDMOS_OPTION_NO_TAPE_AGENT		/* Surrounds MOVER intfs */
   { NDMP2_MOVER_GET_STATE,		0, ndmadr_mover_get_state },
   { NDMP2_MOVER_LISTEN,		0, ndmadr_mover_listen },
   { NDMP2_MOVER_CONTINUE,		0, ndmadr_mover_continue },
   { NDMP2_MOVER_ABORT,			0, ndmadr_mover_abort },
   { NDMP2_MOVER_STOP,			0, ndmadr_mover_stop },
   { NDMP2_MOVER_SET_WINDOW,		0, ndmadr_mover_set_window },
   { NDMP2_MOVER_READ,			0, ndmadr_mover_read },
   { NDMP2_MOVER_CLOSE,			0, ndmadr_mover_close },
   { NDMP2_MOVER_SET_RECORD_SIZE,	0, ndmadr_mover_set_record_size },
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */	/* Surrounds MOVER intfs */
   {0}
};
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
struct ndm_dispatch_request_table ndma_dispatch_request_table_v3[] = {
   { NDMP3_CONNECT_OPEN,
     NDM_DRT_FLAG_OK_NOT_CONNECTED+NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_open
   },
   { NDMP3_CONNECT_CLIENT_AUTH,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_client_auth
   },
   { NDMP3_CONNECT_CLOSE,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_close
   },
   { NDMP3_CONNECT_SERVER_AUTH,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_server_auth
   },
   { NDMP3_CONFIG_GET_HOST_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_host_info
   },
   { NDMP3_CONFIG_GET_CONNECTION_TYPE,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_connection_type
   },
   { NDMP3_CONFIG_GET_AUTH_ATTR,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_auth_attr
   },
   { NDMP3_CONFIG_GET_BUTYPE_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_butype_info
   },
   { NDMP3_CONFIG_GET_FS_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_fs_info
   },
   { NDMP3_CONFIG_GET_TAPE_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_tape_info
   },
   { NDMP3_CONFIG_GET_SCSI_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_scsi_info
   },
   { NDMP3_CONFIG_GET_SERVER_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_server_info
   },
#ifndef NDMOS_OPTION_NO_ROBOT_AGENT		/* Surrounds SCSI intfs */
   { NDMP3_SCSI_OPEN,			0, ndmadr_scsi_open },
   { NDMP3_SCSI_CLOSE,			0, ndmadr_scsi_close },
   { NDMP3_SCSI_GET_STATE,		0, ndmadr_scsi_get_state },
   { NDMP3_SCSI_SET_TARGET,		0, ndmadr_scsi_set_target },
   { NDMP3_SCSI_RESET_DEVICE,		0, ndmadr_scsi_reset_device },
   { NDMP3_SCSI_RESET_BUS,		0, ndmadr_scsi_reset_bus },
   { NDMP3_SCSI_EXECUTE_CDB,		0, ndmadr_scsi_execute_cdb },
#endif /* !NDMOS_OPTION_NO_ROBOT_AGENT */	/* Surrounds SCSI intfs */
#ifndef NDMOS_OPTION_NO_TAPE_AGENT		/* Surrounds TAPE intfs */
   { NDMP3_TAPE_OPEN,			0, ndmadr_tape_open },
   { NDMP3_TAPE_CLOSE,			0, ndmadr_tape_close },
   { NDMP3_TAPE_GET_STATE,		0, ndmadr_tape_get_state },
   { NDMP3_TAPE_MTIO,			0, ndmadr_tape_mtio },
   { NDMP3_TAPE_WRITE,			0, ndmadr_tape_write },
   { NDMP3_TAPE_READ,			0, ndmadr_tape_read },
   { NDMP3_TAPE_EXECUTE_CDB,		0, ndmadr_tape_execute_cdb },
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */	/* Surrounds TAPE intfs */
#ifndef NDMOS_OPTION_NO_DATA_AGENT		/* Surrounds DATA intfs */
   { NDMP3_DATA_GET_STATE,		0, ndmadr_data_get_state },
   { NDMP3_DATA_START_BACKUP,		0, ndmadr_data_start_backup },
   { NDMP3_DATA_START_RECOVER,		0, ndmadr_data_start_recover },
   { NDMP3_DATA_ABORT,			0, ndmadr_data_abort },
   { NDMP3_DATA_GET_ENV,		0, ndmadr_data_get_env },
   { NDMP3_DATA_STOP,			0, ndmadr_data_stop },
   { NDMP3_DATA_LISTEN,			0, ndmadr_data_listen },
   { NDMP3_DATA_CONNECT,		0, ndmadr_data_connect },
   { NDMP3_DATA_START_RECOVER_FILEHIST,	0,
				ndmadr_data_start_recover_filehist },
#endif /* !NDMOS_OPTION_NO_DATA_AGENT */	/* Surrounds DATA intfs */
#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds NOTIFY intfs */
   { NDMP3_NOTIFY_DATA_HALTED,		0, ndmadr_notify_data_halted },
   { NDMP3_NOTIFY_CONNECTED,		0, ndmadr_notify_connected },
   { NDMP3_NOTIFY_MOVER_HALTED,		0, ndmadr_notify_mover_halted },
   { NDMP3_NOTIFY_MOVER_PAUSED,		0, ndmadr_notify_mover_paused },
   { NDMP3_NOTIFY_DATA_READ,		0, ndmadr_notify_data_read },
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds NOTIFY intfs */
#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds LOG intfs */
   { NDMP3_LOG_FILE,			0, ndmadr_log_file },
   { NDMP3_LOG_MESSAGE,			0, ndmadr_log_message },
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds LOG intfs */
#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds FH intfs */
   { NDMP3_FH_ADD_FILE,			0, ndmadr_fh_add_file },
   { NDMP3_FH_ADD_DIR,			0, ndmadr_fh_add_dir },
   { NDMP3_FH_ADD_NODE,			0, ndmadr_fh_add_node },
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds FH intfs */
#ifndef NDMOS_OPTION_NO_TAPE_AGENT		/* Surrounds MOVER intfs */
   { NDMP3_MOVER_GET_STATE,		0, ndmadr_mover_get_state },
   { NDMP3_MOVER_LISTEN,		0, ndmadr_mover_listen },
   { NDMP3_MOVER_CONTINUE,		0, ndmadr_mover_continue },
   { NDMP3_MOVER_ABORT,			0, ndmadr_mover_abort },
   { NDMP3_MOVER_STOP,			0, ndmadr_mover_stop },
   { NDMP3_MOVER_SET_WINDOW,		0, ndmadr_mover_set_window },
   { NDMP3_MOVER_READ,			0, ndmadr_mover_read },
   { NDMP3_MOVER_CLOSE,			0, ndmadr_mover_close },
   { NDMP3_MOVER_SET_RECORD_SIZE,	0, ndmadr_mover_set_record_size },
   { NDMP3_MOVER_CONNECT,		0, ndmadr_mover_connect },
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */	/* Surrounds MOVER intfs */
   {0}
};
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
struct ndm_dispatch_request_table ndma_dispatch_request_table_v4[] = {
   { NDMP4_CONNECT_OPEN,
     NDM_DRT_FLAG_OK_NOT_CONNECTED+NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_open
   },
   { NDMP4_CONNECT_CLIENT_AUTH,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_client_auth
   },
   { NDMP4_CONNECT_CLOSE,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_close
   },
   { NDMP4_CONNECT_SERVER_AUTH,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_connect_server_auth
   },
   { NDMP4_CONFIG_GET_HOST_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_host_info
   },
   { NDMP4_CONFIG_GET_CONNECTION_TYPE,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_connection_type
   },
   { NDMP4_CONFIG_GET_AUTH_ATTR,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_auth_attr
   },
   { NDMP4_CONFIG_GET_BUTYPE_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_butype_info
   },
   { NDMP4_CONFIG_GET_FS_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_fs_info
   },
   { NDMP4_CONFIG_GET_TAPE_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_tape_info
   },
   { NDMP4_CONFIG_GET_SCSI_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_scsi_info
   },
   { NDMP4_CONFIG_GET_SERVER_INFO,
     NDM_DRT_FLAG_OK_NOT_AUTHORIZED,
     ndmadr_config_get_server_info
   },
#ifndef NDMOS_OPTION_NO_ROBOT_AGENT		/* Surrounds SCSI intfs */
   { NDMP4_SCSI_OPEN,			0, ndmadr_scsi_open },
   { NDMP4_SCSI_CLOSE,			0, ndmadr_scsi_close },
   { NDMP4_SCSI_GET_STATE,		0, ndmadr_scsi_get_state },
   { NDMP4_SCSI_RESET_DEVICE,		0, ndmadr_scsi_reset_device },
   { NDMP4_SCSI_EXECUTE_CDB,		0, ndmadr_scsi_execute_cdb },
#endif /* !NDMOS_OPTION_NO_ROBOT_AGENT */	/* Surrounds SCSI intfs */
#ifndef NDMOS_OPTION_NO_TAPE_AGENT		/* Surrounds TAPE intfs */
   { NDMP4_TAPE_OPEN,			0, ndmadr_tape_open },
   { NDMP4_TAPE_CLOSE,			0, ndmadr_tape_close },
   { NDMP4_TAPE_GET_STATE,		0, ndmadr_tape_get_state },
   { NDMP4_TAPE_MTIO,			0, ndmadr_tape_mtio },
   { NDMP4_TAPE_WRITE,			0, ndmadr_tape_write },
   { NDMP4_TAPE_READ,			0, ndmadr_tape_read },
   { NDMP4_TAPE_EXECUTE_CDB,		0, ndmadr_tape_execute_cdb },
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */	/* Surrounds TAPE intfs */
#ifndef NDMOS_OPTION_NO_DATA_AGENT		/* Surrounds DATA intfs */
   { NDMP4_DATA_GET_STATE,		0, ndmadr_data_get_state },
   { NDMP4_DATA_START_BACKUP,		0, ndmadr_data_start_backup },
   { NDMP4_DATA_START_RECOVER,		0, ndmadr_data_start_recover },
   { NDMP4_DATA_ABORT,			0, ndmadr_data_abort },
   { NDMP4_DATA_GET_ENV,		0, ndmadr_data_get_env },
   { NDMP4_DATA_STOP,			0, ndmadr_data_stop },
   { NDMP4_DATA_LISTEN,			0, ndmadr_data_listen },
   { NDMP4_DATA_CONNECT,		0, ndmadr_data_connect },
   { NDMP4_DATA_START_RECOVER_FILEHIST,	0,
				ndmadr_data_start_recover_filehist },
#endif /* !NDMOS_OPTION_NO_DATA_AGENT */	/* Surrounds DATA intfs */
#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds NOTIFY intfs */
   { NDMP4_NOTIFY_DATA_HALTED,		0, ndmadr_notify_data_halted },
   { NDMP4_NOTIFY_CONNECTION_STATUS,	0, ndmadr_notify_connected },
   { NDMP4_NOTIFY_MOVER_HALTED,		0, ndmadr_notify_mover_halted },
   { NDMP4_NOTIFY_MOVER_PAUSED,		0, ndmadr_notify_mover_paused },
   { NDMP4_NOTIFY_DATA_READ,		0, ndmadr_notify_data_read },
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds NOTIFY intfs */
#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds LOG intfs */
   { NDMP4_LOG_FILE,			0, ndmadr_log_file },
   { NDMP4_LOG_MESSAGE,			0, ndmadr_log_message },
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds LOG intfs */
#ifndef NDMOS_OPTION_NO_CONTROL_AGENT		/* Surrounds FH intfs */
   { NDMP4_FH_ADD_FILE,			0, ndmadr_fh_add_file },
   { NDMP4_FH_ADD_DIR,			0, ndmadr_fh_add_dir },
   { NDMP4_FH_ADD_NODE,			0, ndmadr_fh_add_node },
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */	/* Surrounds FH intfs */
#ifndef NDMOS_OPTION_NO_TAPE_AGENT		/* Surrounds MOVER intfs */
   { NDMP4_MOVER_GET_STATE,		0, ndmadr_mover_get_state },
   { NDMP4_MOVER_LISTEN,		0, ndmadr_mover_listen },
   { NDMP4_MOVER_CONTINUE,		0, ndmadr_mover_continue },
   { NDMP4_MOVER_ABORT,			0, ndmadr_mover_abort },
   { NDMP4_MOVER_STOP,			0, ndmadr_mover_stop },
   { NDMP4_MOVER_SET_WINDOW,		0, ndmadr_mover_set_window },
   { NDMP4_MOVER_READ,			0, ndmadr_mover_read },
   { NDMP4_MOVER_CLOSE,			0, ndmadr_mover_close },
   { NDMP4_MOVER_SET_RECORD_SIZE,	0, ndmadr_mover_set_record_size },
   { NDMP4_MOVER_CONNECT,		0, ndmadr_mover_connect },
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */	/* Surrounds MOVER intfs */
   {0}
};
#endif /* !NDMOS_OPTION_NO_NDMP4 */


struct ndm_dispatch_version_table ndma_dispatch_version_table[] = {
	{ 0,		ndma_dispatch_request_table_v0 },
#ifndef NDMOS_OPTION_NO_NDMP2
	{ NDMP2VER,	ndma_dispatch_request_table_v2 },
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	{ NDMP3VER,	ndma_dispatch_request_table_v3 },
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	{ NDMP4VER,	ndma_dispatch_request_table_v4 },
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	{ -1 }
};

