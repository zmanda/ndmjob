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
 * Ident:    $Id: ndma_ctrl_calls.c,v 1.1.1.1 2003/10/14 19:12:38 ern Exp $
 *
 * Description:
 *
 */


#include "ndmagents.h"


#ifndef NDMOS_OPTION_NO_CONTROL_AGENT


/*
 * DATA Agent calls
 ****************************************************************
 */

int
ndmca_data_get_state (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.data;
	struct ndm_control_agent *ca = &sess->control_acb;
	struct ndmp9_data_get_state_reply *state = &ca->data_state;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_data_get_state, NDMP2VER)
		rc = NDMC_CALL(conn);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->data_state);
			ca->data_state.state = -1;
		} else {
			ndmp_2to9_data_get_state_reply (reply, state);
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_data_get_state, NDMP3VER)
		rc = NDMC_CALL(conn);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->data_state);
			ca->data_state.state = -1;
		} else {
			ndmp_3to9_data_get_state_reply (reply, state);
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_data_get_state, NDMP4VER)
		rc = NDMC_CALL(conn);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->data_state);
			ca->data_state.state = -1;
		} else {
			ndmp_4to9_data_get_state_reply (reply, state);
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

#ifndef NDMOS_OPTION_NO_NDMP3
int
ndmca_data_connect (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.data;
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    rc = NDMADR_UNIMPLEMENTED_VERSION;	/* can't happen */
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH(ndmp3_data_connect, NDMP3VER)
		ndmp_9to3_addr (&ca->mover_addr, &request->addr);
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH(ndmp4_data_connect, NDMP4VER)
		ndmp_9to4_addr (&ca->mover_addr, &request->addr);
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}
#endif /* !NDMOS_OPTION_NO_NDMP3 */

int
ndmca_data_start_backup (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.data;
	struct ndm_control_agent *ca = &sess->control_acb;
	unsigned		n_env = ca->job.env_tab.n_env;
	ndmp9_pval *		env = ca->job.env_tab.env;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH(ndmp2_data_start_backup, NDMP2VER)
		ndmp2_pval	envbuf[NDM_MAX_ENV];

		ndmp_9to2_mover_addr (&ca->mover_addr, &request->mover);
		request->bu_type = ca->job.bu_type;

		ndmp_9to2_pval_vec (env, envbuf, n_env);
		request->env.env_len = n_env;
		request->env.env_val = envbuf;

		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    if ( (rc = ndmca_data_connect (sess)) != 0)
		break;

	    NDMC_WITH(ndmp3_data_start_backup, NDMP3VER)
		ndmp3_pval	envbuf[NDM_MAX_ENV];

		request->bu_type = ca->job.bu_type;

		ndmp_9to3_pval_vec (env, envbuf, n_env);
		request->env.env_len = n_env;
		request->env.env_val = envbuf;

		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    if ( (rc = ndmca_data_connect (sess)) != 0)
		break;

	    NDMC_WITH(ndmp4_data_start_backup, NDMP4VER)
		ndmp4_pval	envbuf[NDM_MAX_ENV];

		request->bu_type = ca->job.bu_type;

		ndmp_9to4_pval_vec (env, envbuf, n_env);
		request->env.env_len = n_env;
		request->env.env_val = envbuf;

		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_data_start_recover (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.data;
	struct ndm_control_agent *ca = &sess->control_acb;
	unsigned		n_env = ca->job.env_tab.n_env;
	ndmp9_pval *		env = ca->job.env_tab.env;
	unsigned		n_nlist = ca->job.nlist_tab.n_nlist;
	ndmp9_name *		nlist = ca->job.nlist_tab.nlist;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH(ndmp2_data_start_recover, NDMP2VER)
		ndmp2_pval	envbuf[NDM_MAX_ENV];
		ndmp2_name	nlistbuf[NDM_MAX_NLIST];

		ndmp_9to2_mover_addr (&ca->mover_addr, &request->mover);
		request->bu_type = ca->job.bu_type;

		ndmp_9to2_pval_vec (env, envbuf, n_env);
		request->env.env_len = n_env;
		request->env.env_val = envbuf;

		ndmp_9to2_name_vec (nlist, nlistbuf, n_nlist);
		request->nlist.nlist_len = n_nlist;
		request->nlist.nlist_val = nlistbuf;

		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    if ( (rc = ndmca_data_connect (sess)) != 0)
		break;

	    NDMC_WITH(ndmp3_data_start_recover, NDMP3VER)
		ndmp3_pval	envbuf[NDM_MAX_ENV];
		ndmp3_name	nlistbuf[NDM_MAX_NLIST];

		request->bu_type = ca->job.bu_type;

		ndmp_9to3_pval_vec (env, envbuf, n_env);
		request->env.env_len = n_env;
		request->env.env_val = envbuf;

		ndmp_9to3_name_vec (nlist, nlistbuf, n_nlist);
		request->nlist.nlist_len = n_nlist;
		request->nlist.nlist_val = nlistbuf;

		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    if ( (rc = ndmca_data_connect (sess)) != 0)
		break;

	    NDMC_WITH(ndmp4_data_start_recover, NDMP4VER)
		ndmp4_pval	envbuf[NDM_MAX_ENV];
		ndmp4_name	nlistbuf[NDM_MAX_NLIST];

		request->bu_type = ca->job.bu_type;

		ndmp_9to4_pval_vec (env, envbuf, n_env);
		request->env.env_len = n_env;
		request->env.env_val = envbuf;

		ndmp_9to4_name_vec (nlist, nlistbuf, n_nlist);
		request->nlist.nlist_len = n_nlist;
		request->nlist.nlist_val = nlistbuf;

		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_data_start_recover_filehist (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.data;
	struct ndm_control_agent *ca = &sess->control_acb;
	unsigned		n_env = ca->job.env_tab.n_env;
	ndmp9_pval *		env = ca->job.env_tab.env;
	unsigned		n_nlist = ca->job.nlist_tab.n_nlist;
	ndmp9_name *		nlist = ca->job.nlist_tab.nlist;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH(ndmp2_data_start_recover_filehist, NDMP2VER)
		ndmp2_pval	envbuf[NDM_MAX_ENV];
		ndmp2_name	nlistbuf[NDM_MAX_NLIST];

		ndmp_9to2_mover_addr (&ca->mover_addr, &request->mover);
		request->bu_type = ca->job.bu_type;

		ndmp_9to2_pval_vec (env, envbuf, n_env);
		request->env.env_len = n_env;
		request->env.env_val = envbuf;

		ndmp_9to2_name_vec (nlist, nlistbuf, n_nlist);
		request->nlist.nlist_len = n_nlist;
		request->nlist.nlist_val = nlistbuf;

		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    if ( (rc = ndmca_data_connect (sess)) != 0)
		break;

	    NDMC_WITH(ndmp3_data_start_recover_filehist, NDMP3VER)
		ndmp3_pval	envbuf[NDM_MAX_ENV];
		ndmp3_name	nlistbuf[NDM_MAX_NLIST];

		request->bu_type = ca->job.bu_type;

		ndmp_9to3_pval_vec (env, envbuf, n_env);
		request->env.env_len = n_env;
		request->env.env_val = envbuf;

		ndmp_9to3_name_vec (nlist, nlistbuf, n_nlist);
		request->nlist.nlist_len = n_nlist;
		request->nlist.nlist_val = nlistbuf;

		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    if ( (rc = ndmca_data_connect (sess)) != 0)
		break;

	    NDMC_WITH(ndmp4_data_start_recover_filehist, NDMP4VER)
		ndmp4_pval	envbuf[NDM_MAX_ENV];
		ndmp4_name	nlistbuf[NDM_MAX_NLIST];

		request->bu_type = ca->job.bu_type;

		ndmp_9to4_pval_vec (env, envbuf, n_env);
		request->env.env_len = n_env;
		request->env.env_val = envbuf;

		ndmp_9to4_name_vec (nlist, nlistbuf, n_nlist);
		request->nlist.nlist_len = n_nlist;
		request->nlist.nlist_val = nlistbuf;

		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_data_abort (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.data;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_data_abort, NDMP2VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_data_abort, NDMP3VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_data_abort, NDMP4VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_data_get_env (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.data;
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;
	int			i;
	ndmp9_pval *		d_pv;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_data_get_env, NDMP2VER)
		rc = NDMC_CALL(conn);
		if (rc) return rc;

		for (i = 0; i < reply->env.env_len; i++) {
			ndmp2_pval *		s_pv;

			s_pv = &reply->env.env_val[i];
			d_pv = &ca->job.result_env_tab.env[i];
			d_pv->name  = NDMOS_API_STRDUP (s_pv->name);
			d_pv->value = NDMOS_API_STRDUP (s_pv->value);
		}
		ca->job.result_env_tab.n_env = i;

		NDMC_FREE_REPLY();
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_data_get_env, NDMP3VER)
		rc = NDMC_CALL(conn);
		if (rc) return rc;

		for (i = 0; i < reply->env.env_len; i++) {
			ndmp3_pval *		s_pv;

			s_pv = &reply->env.env_val[i];
			d_pv = &ca->job.result_env_tab.env[i];
			d_pv->name  = NDMOS_API_STRDUP (s_pv->name);
			d_pv->value = NDMOS_API_STRDUP (s_pv->value);
		}
		ca->job.result_env_tab.n_env = i;

		NDMC_FREE_REPLY();
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_data_get_env, NDMP4VER)
		rc = NDMC_CALL(conn);
		if (rc) return rc;

		for (i = 0; i < reply->env.env_len; i++) {
			ndmp4_pval *		s_pv;

			s_pv = &reply->env.env_val[i];
			d_pv = &ca->job.result_env_tab.env[i];
			d_pv->name  = NDMOS_API_STRDUP (s_pv->name);
			d_pv->value = NDMOS_API_STRDUP (s_pv->value);
		}
		ca->job.result_env_tab.n_env = i;

		NDMC_FREE_REPLY();
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_data_stop (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.data;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_data_stop, NDMP2VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_data_stop, NDMP3VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_data_stop, NDMP4VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}





/*
 * TAPE Agent calls -- TAPE
 ****************************************************************
 */

int
ndmca_tape_open (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.tape;
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH (ndmp2_tape_open, NDMP2VER)
		request->device.name = ca->job.tape_device;
		request->mode = ca->tape_mode;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH (ndmp3_tape_open, NDMP3VER)
		request->device = ca->job.tape_device;
		request->mode = ca->tape_mode;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH (ndmp4_tape_open, NDMP4VER)
		request->device = ca->job.tape_device;
		request->mode = ca->tape_mode;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_tape_close (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.tape;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_tape_close, NDMP2VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_tape_close, NDMP3VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_tape_close, NDMP4VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_tape_get_state (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.tape;
	struct ndm_control_agent *ca = &sess->control_acb;
	struct ndmp9_tape_get_state_reply *state = &ca->tape_state;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_tape_get_state, NDMP2VER)
		rc = NDMC_CALL(conn);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->tape_state);
			/* tape_state.state = -1; */
		} else {
			ndmp_2to9_tape_get_state_reply (reply, state);
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_tape_get_state, NDMP3VER)
		rc = NDMC_CALL(conn);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->tape_state);
			/* tape_state.state = -1; */
		} else {
			ndmp_3to9_tape_get_state_reply (reply, state);
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_tape_get_state, NDMP4VER)
		rc = NDMC_CALL(conn);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->tape_state);
			/* tape_state.state = -1; */
		} else {
			ndmp_4to9_tape_get_state_reply (reply, state);
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_tape_get_state_no_tattle (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.tape;
	struct ndm_control_agent *ca = &sess->control_acb;
	struct ndmp9_tape_get_state_reply *state = &ca->tape_state;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_tape_get_state, NDMP2VER)
		rc = ndma_call_no_tattle (conn, xa);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->tape_state);
			/* tape_state.state = -1; */
		} else {
			ndmp_2to9_tape_get_state_reply (reply, state);
		}
		if (rc < 0
		 ||  (reply->error != NDMP2_DEV_NOT_OPEN_ERR
		   && reply->error != NDMP2_NO_ERR))
			ndma_tattle (sess->plumb.tape, xa, rc);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_tape_get_state, NDMP3VER)
		rc = ndma_call_no_tattle (conn, xa);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->tape_state);
			/* tape_state.state = -1; */
		} else {
			ndmp_3to9_tape_get_state_reply (reply, state);
		}
		if (rc < 0
		 ||  (reply->error != NDMP3_DEV_NOT_OPEN_ERR
		   && reply->error != NDMP3_NO_ERR))
			ndma_tattle (sess->plumb.tape, xa, rc);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_tape_get_state, NDMP4VER)
		rc = ndma_call_no_tattle (conn, xa);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->tape_state);
			/* tape_state.state = -1; */
		} else {
			ndmp_4to9_tape_get_state_reply (reply, state);
		}
		if (rc < 0
		 ||  (reply->error != NDMP4_DEV_NOT_OPEN_ERR
		   && reply->error != NDMP4_NO_ERR))
			ndma_tattle (sess->plumb.tape, xa, rc);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_tape_mtio (struct ndm_session *sess,
  ndmp9_tape_mtio_op op, u_long count, u_long *resid)
{
	struct ndmconn *	conn = sess->plumb.tape;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH(ndmp2_tape_mtio, NDMP2VER)
		request->tape_op = op;
		request->count = count;

		rc = NDMC_CALL(conn);
		if (rc) return rc;

		if (resid)
			*resid = reply->resid_count;
		else if (reply->resid_count != 0)
			return -1;
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH(ndmp3_tape_mtio, NDMP3VER)
		request->tape_op = op;
		request->count = count;

		rc = NDMC_CALL(conn);
		if (rc) return rc;

		if (resid)
			*resid = reply->resid_count;
		else if (reply->resid_count != 0)
			return -1;
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH(ndmp4_tape_mtio, NDMP4VER)
		request->tape_op = op;
		request->count = count;

		rc = NDMC_CALL(conn);
		if (rc) return rc;

		if (resid)
			*resid = reply->resid_count;
		else if (reply->resid_count != 0)
			return -1;
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_tape_write (struct ndm_session *sess, char *buf, unsigned count)
{
	struct ndmconn *	conn = sess->plumb.tape;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH(ndmp2_tape_write, NDMP2VER)
		request->data_out.data_out_len = count;
		request->data_out.data_out_val = buf;
		rc = NDMC_CALL(conn);
		if (rc == 0) {
			if (reply->count != count)
				rc = -1;
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH(ndmp3_tape_write, NDMP3VER)
		request->data_out.data_out_len = count;
		request->data_out.data_out_val = buf;
		rc = NDMC_CALL(conn);
		if (rc == 0) {
			if (reply->count != count)
				rc = -1;
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH(ndmp4_tape_write, NDMP4VER)
		request->data_out.data_out_len = count;
		request->data_out.data_out_val = buf;
		rc = NDMC_CALL(conn);
		if (rc == 0) {
			if (reply->count != count)
				rc = -1;
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_tape_read (struct ndm_session *sess, char *buf, unsigned count)
{
	struct ndmconn *	conn = sess->plumb.tape;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH(ndmp2_tape_read, NDMP2VER)
		request->count = count;
		rc = NDMC_CALL(conn);
		if (rc == 0) {
			if (reply->data_in.data_in_len == count) {
				bcopy (reply->data_in.data_in_val,
							buf, count);
			} else {
				rc = -1;
			}
		}
		NDMC_FREE_REPLY();
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH(ndmp3_tape_read, NDMP3VER)
		request->count = count;
		rc = NDMC_CALL(conn);
		if (rc == 0) {
			if (reply->data_in.data_in_len == count) {
				bcopy (reply->data_in.data_in_val,
							buf, count);
			} else {
				rc = -1;
			}
		}
		NDMC_FREE_REPLY();
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH(ndmp4_tape_read, NDMP4VER)
		request->count = count;
		rc = NDMC_CALL(conn);
		if (rc == 0) {
			if (reply->data_in.data_in_len == count) {
				bcopy (reply->data_in.data_in_val,
							buf, count);
			} else {
				rc = -1;
			}
		}
		NDMC_FREE_REPLY();
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}


/*
 * TAPE Agent calls -- MOVER
 ****************************************************************
 */

int
ndmca_mover_get_state (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.tape;
	struct ndm_control_agent *ca = &sess->control_acb;
	struct ndmp9_mover_get_state_reply *state = &ca->mover_state;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_mover_get_state, NDMP2VER)
		rc = NDMC_CALL(conn);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->mover_state);
			ca->mover_state.state = -1;
		} else {
			ndmp_2to9_mover_get_state_reply (reply, state);
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_mover_get_state, NDMP3VER)
		rc = NDMC_CALL(conn);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->mover_state);
			ca->mover_state.state = -1;
		} else {
			ndmp_3to9_mover_get_state_reply (reply, state);
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_mover_get_state, NDMP4VER)
		rc = NDMC_CALL(conn);
		if (rc) {
			NDMOS_MACRO_ZEROFILL (&ca->mover_state);
			ca->mover_state.state = -1;
		} else {
			ndmp_4to9_mover_get_state_reply (reply, state);
		}
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_mover_listen (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.tape;
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH(ndmp2_mover_listen, NDMP2VER)
		request->mode = ca->mover_mode;

		if (sess->plumb.tape == sess->plumb.data) {
			request->addr_type = NDMP2_ADDR_LOCAL;
		} else {
			request->addr_type = NDMP2_ADDR_TCP;
		}

		rc = NDMC_CALL(conn);
		if (rc) return rc;

		if (request->addr_type != reply->mover.addr_type) {
			ndmalogf (sess, 0, 0,
				"MOVER_LISTEN addr_type mismatch");
			return -1;
		}

		ndmp_2to9_mover_addr (&reply->mover, &ca->mover_addr);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH(ndmp3_mover_listen, NDMP3VER)
		request->mode = ca->mover_mode;

		if (sess->plumb.tape == sess->plumb.data) {
			request->addr_type = NDMP3_ADDR_LOCAL;
		} else {
			request->addr_type = NDMP3_ADDR_TCP;
		}

		rc = NDMC_CALL(conn);
		if (rc) return rc;

		if (request->addr_type
		 != reply->data_connection_addr.addr_type) {
			ndmalogf (sess, 0, 0,
				"MOVER_LISTEN addr_type mismatch");
			return -1;
		}

		ndmp_3to9_addr (&reply->data_connection_addr, &ca->mover_addr);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH(ndmp4_mover_listen, NDMP4VER)
		request->mode = ca->mover_mode;

		if (sess->plumb.tape == sess->plumb.data) {
			request->addr_type = NDMP4_ADDR_LOCAL;
		} else {
			request->addr_type = NDMP4_ADDR_TCP;
		}

		rc = NDMC_CALL(conn);
		if (rc) return rc;

		if (request->addr_type
		 != reply->data_connection_addr.addr_type) {
			ndmalogf (sess, 0, 0,
				"MOVER_LISTEN addr_type mismatch");
			return -1;
		}

		ndmp_4to9_addr (&reply->data_connection_addr, &ca->mover_addr);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return 0;
}

int
ndmca_mover_continue (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.tape;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_mover_continue, NDMP2VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_mover_continue, NDMP3VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_mover_continue, NDMP4VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_mover_abort (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.tape;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_mover_abort, NDMP2VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_mover_abort, NDMP3VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_mover_abort, NDMP4VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_mover_stop (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.tape;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_mover_stop, NDMP2VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_mover_stop, NDMP3VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_mover_stop, NDMP4VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_mover_set_window (struct ndm_session *sess,
  unsigned long long offset, unsigned long long length)
{
	struct ndmconn *	conn = sess->plumb.tape;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH(ndmp2_mover_set_window, NDMP2VER)
		request->offset = offset;
		request->length = length;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH(ndmp3_mover_set_window, NDMP3VER)
		request->offset = offset;
		request->length = length;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH(ndmp4_mover_set_window, NDMP4VER)
		request->offset = offset;
		request->length = length;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_mover_read (struct ndm_session *sess,
  unsigned long long offset, unsigned long long length)
{
	struct ndmconn *	conn = sess->plumb.tape;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH(ndmp2_mover_read, NDMP2VER)
		request->offset = offset;
		request->length = length;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH(ndmp3_mover_read, NDMP3VER)
		request->offset = offset;
		request->length = length;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH(ndmp4_mover_read, NDMP4VER)
		request->offset = offset;
		request->length = length;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_mover_close (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.tape;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_VOID_REQUEST(ndmp2_mover_close, NDMP2VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_VOID_REQUEST(ndmp3_mover_close, NDMP3VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_VOID_REQUEST(ndmp4_mover_close, NDMP4VER)
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}

int
ndmca_mover_set_record_size (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.tape;
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc = NDMADR_UNIMPLEMENTED_VERSION;

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH(ndmp2_mover_set_record_size, NDMP2VER)
		request->len = ca->job.record_size;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH(ndmp3_mover_set_record_size, NDMP3VER)
		request->len = ca->job.record_size;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH(ndmp4_mover_set_record_size, NDMP4VER)
		request->len = ca->job.record_size;
		rc = NDMC_CALL(conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}

	return rc;
}
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */
