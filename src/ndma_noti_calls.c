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
 * Ident:    $Id: ndma_noti_calls.c,v 1.1.1.1 2003/10/14 19:18:24 ern Exp $
 *
 * Description:
 *
 */


#include "ndmagents.h"


#ifndef NDMOS_OPTION_NO_DATA_AGENT


/*
 * DATA Agent originated calls
 ****************************************************************
 */

int
ndma_notify_data_halted (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.control;
	struct ndm_data_agent *	da = &sess->data_acb;

	assert (da->data_state.state == NDMP9_DATA_STATE_HALTED);
	assert (da->data_state.halt_reason != NDMP9_DATA_HALT_NA);

	NDMC_WITH_NO_REPLY(ndmp9_notify_data_halted, NDMP9VER)
		request->reason = da->data_state.halt_reason;
		ndma_send_to_control (sess, xa, sess->plumb.data);
	NDMC_ENDWITH

	return 0;
}

int
ndma_notify_data_read (struct ndm_session *sess,
  unsigned long long offset, unsigned long long length)
{
	struct ndmconn *	conn = sess->plumb.control;

	NDMC_WITH_NO_REPLY(ndmp9_notify_data_read, NDMP9VER)
		request->offset = offset;
		request->length = length;
		ndma_send_to_control (sess, xa, sess->plumb.data);
	NDMC_ENDWITH

	return 0;
}

#endif /* !NDMOS_OPTION_NO_DATA_AGENT */


#ifndef NDMOS_OPTION_NO_TAPE_AGENT

int
ndma_notify_mover_halted (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.control;
	struct ndm_tape_agent *	ta = &sess->tape_acb;

	assert (ta->mover_state.state == NDMP9_MOVER_STATE_HALTED);
	assert (ta->mover_state.halt_reason != NDMP9_MOVER_HALT_NA);

	NDMC_WITH_NO_REPLY(ndmp9_notify_mover_halted, NDMP9VER)
		request->reason = ta->mover_state.halt_reason;
		ndma_send_to_control (sess, xa, sess->plumb.tape);
	NDMC_ENDWITH

	return 0;
}

int
ndma_notify_mover_paused (struct ndm_session *sess)
{
	struct ndmconn *	conn = sess->plumb.control;
	struct ndm_tape_agent *	ta = &sess->tape_acb;

	assert (ta->mover_state.state == NDMP9_MOVER_STATE_PAUSED);
	assert (ta->mover_state.pause_reason != NDMP9_MOVER_PAUSE_NA);

	NDMC_WITH_NO_REPLY(ndmp9_notify_mover_paused, NDMP9VER)
		request->reason = ta->mover_state.pause_reason;
		request->seek_position = ta->mover_want_pos;
		ndma_send_to_control (sess, xa, sess->plumb.tape);
	NDMC_ENDWITH

	return 0;
}

#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */


#ifndef NDMOS_EFFECT_NO_SERVER_AGENTS
void
ndma_send_logmsg (struct ndm_session *sess, char *buf,
  struct ndmconn *conn)
{

	switch (conn->protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    NDMC_WITH_NO_REPLY(ndmp2_log_log, NDMP2VER)
		request->entry = buf;
		ndma_send_to_control (sess, xa, conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    NDMC_WITH_NO_REPLY(ndmp3_log_message, NDMP3VER)
		request->log_type = NDMP3_LOG_NORMAL;
		request->message_id = time(0);
		request->entry = buf;
		ndma_send_to_control (sess, xa, conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    NDMC_WITH_POST(ndmp4_log_message, NDMP4VER)
		request->log_type = NDMP4_LOG_NORMAL;
		request->message_id = time(0);
		request->entry = buf;
		ndma_send_to_control (sess, xa, conn);
	    NDMC_ENDWITH
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */

	default:
		/* BOGUS */
		break;
	}
}

#endif /* !NDMOS_EFFECT_NO_SERVER_AGENTS */
