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
 * Ident:    $Id: ndma_ctst_data.c,v 1.1.1.1 2003/10/14 19:18:26 ern Exp $
 *
 * Description:
 *
 ********************************************************************
 *
 * NDMP Elements of a test-data session
 *
 *                   +-----+     ###########
 *                   | Job |----># CONTROL #
 *                   +-----+     #  Agent  #
 *                               #         #
 *                               ###########
 *                                 |      #
 *                +----------------+      #
 *                | control connection    #     CONTROL
 *                V                       #     impersonates
 *           ############                 #     TAPE side of
 *           #  DATA    #                 #     image stream
 *           #  Agent   #                 #
 *  +-----+  # +------+ # image           #
 *  |FILES|====|butype|===================#
 *  +-----+  # +------+ # stream
 *           ############
 *
 *
 ********************************************************************
 *
 */


#include "ndmagents.h"


#ifndef NDMOS_OPTION_NO_CONTROL_AGENT


extern int	ndmca_td_wrapper (struct ndm_session *sess,
				int (*func)(struct ndm_session *sess));


extern int	ndmca_op_test_data (struct ndm_session *sess);
extern int	ndmca_td_idle (struct ndm_session *sess);

extern int	ndmca_test_check_data_state  (struct ndm_session *sess,
			ndmp9_data_state expected, int reason);
extern int	ndmca_test_data_get_state (struct ndm_session *sess,
			ndmp9_error expect_err);
extern int	ndmca_test_data_abort (struct ndm_session *sess,
			ndmp9_error expect_err);
extern int	ndmca_test_data_stop (struct ndm_session *sess,
			ndmp9_error expect_err);



int
ndmca_op_test_data (struct ndm_session *sess)
{
	struct ndmconn *	conn;
	int			(*save_call) (struct ndmconn *conn,
						struct ndmp_xa_buf *xa);
	int			rc;

	rc = ndmca_connect_tape_agent(sess);
	if (rc) return rc;

	conn = sess->plumb.data;
	save_call = conn->call;
	conn->call = ndma_call_no_tattle;

	rc = ndmca_td_wrapper (sess, ndmca_td_idle);

	ndmca_test_done_series (sess, "test-data");

	return 0;
}

int
ndmca_td_wrapper (struct ndm_session *sess,
  int (*func)(struct ndm_session *sess))
{
	int		rc;

	rc = (*func)(sess);

	if (rc != 0) {
		ndmalogf (sess, "Test", 1, "Failure");
	}

	ndmca_test_done_phase (sess);

	/* clean up mess */
	ndmca_test_log_note (sess, 2, "Cleaning up...");

	rc = 0;

	return rc;
}

int
ndmca_td_idle (struct ndm_session *sess)
{
	int		rc;

	ndmca_test_phase (sess, "D-IDLE", "Data IDLE State Series");

	rc = ndmca_test_check_data_state  (sess, NDMP9_DATA_STATE_IDLE, 0);
	if (rc) return rc;

	rc = ndmca_test_data_abort (sess, NDMP9_ILLEGAL_STATE_ERR);
	if (rc) return rc;

	rc = ndmca_test_data_stop (sess, NDMP9_ILLEGAL_STATE_ERR);
	if (rc) return rc;

	return 0;	/* pass */
}




int
ndmca_test_check_data_state  (struct ndm_session *sess,
  ndmp9_data_state expected, int reason)
{
	struct ndm_control_agent *	ca = &sess->control_acb;
	ndmp9_data_get_state_reply *	ds = &ca->data_state;
	int				rc;
	char *				what;
	char				errbuf[100];

	strcpy (errbuf, "???");

	what = "get_state";
	rc = ndmca_data_get_state (sess);
	if (rc) goto fail;

	what = "state self-consistent";
	/* make sure the sensed state is self consistent */
	switch (ds->state) {
	case NDMP9_DATA_STATE_IDLE:
	case NDMP9_DATA_STATE_ACTIVE:
	case NDMP9_DATA_STATE_LISTEN:
	case NDMP9_DATA_STATE_CONNECTED:
		if (ds->halt_reason != NDMP9_DATA_HALT_NA) {
			strcpy (errbuf, "reason != NA");
			goto fail;
		}
		break;

	case NDMP9_DATA_STATE_HALTED:
		break;

	default:
		strcpy (errbuf, "bogus state");
		goto fail;
	}

	what = "state";
	if (ds->state != expected) {
		sprintf (errbuf, "expected %s got %s",
			ndmp9_data_state_to_str (expected),
			ndmp9_data_state_to_str (ds->state));
		goto fail;
	}

	what = "reason";
	switch (ds->state) {
	case NDMP9_DATA_STATE_HALTED:
		if (ds->halt_reason != reason) {
			sprintf (errbuf, "expected %s got %s",
			    ndmp9_data_halt_reason_to_str (reason),
			    ndmp9_data_halt_reason_to_str (ds->halt_reason));
			goto fail;
		}
		break;

	default:
		break;
	}

	ndmalogf (sess, "Test", 2,
		"%s #%d -- Passed data check %s",
		sess->control_acb.test_phase,
		sess->control_acb.test_step,
		ndmp9_data_state_to_str (expected));

	sess->control_acb.test_step++;

	return 0;

  fail:
	ndmalogf (sess, "Test", 1,
		"%s #%d -- Failed data check, %s: %s",
		sess->control_acb.test_phase,
		sess->control_acb.test_step,
		what,
		errbuf);

	sess->control_acb.test_step++;

	return -1;
}



#define NDMTEST_CALL(CONN) ndmca_test_call(CONN, xa, expect_err);


int
ndmca_test_data_get_state (struct ndm_session *sess, ndmp9_error expect_err)
{
	struct ndmconn *	conn = sess->plumb.data;
	int			rc = ndmca_data_get_state (sess);

	rc = ndmca_test_check_expect (conn, rc, expect_err);

	return rc;
}

int
ndmca_test_data_abort (struct ndm_session *sess, ndmp9_error expect_err)
{
	struct ndmconn *	conn = sess->plumb.data;
	int			rc = ndmca_data_abort (sess);

	rc = ndmca_test_check_expect (conn, rc, expect_err);

	return rc;
}

int
ndmca_test_data_stop (struct ndm_session *sess, ndmp9_error expect_err)
{
	struct ndmconn *	conn = sess->plumb.data;
	int			rc = ndmca_data_stop (sess);

	rc = ndmca_test_check_expect (conn, rc, expect_err);

	return rc;
}




#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */
