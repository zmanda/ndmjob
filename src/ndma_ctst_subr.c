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
 * Ident:    $Id: ndma_ctst_subr.c,v 1.1 2003/10/14 19:12:39 ern Exp $
 *
 * Description:
 *
 */


#include "ndmagents.h"


#ifndef NDMOS_OPTION_NO_CONTROL_AGENT


int
ndmca_test_load_tape (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc;

	ca->tape_mode = NDMP9_TAPE_READ_MODE;
	ca->is_label_op = 1;

	rc = ndmca_op_robot_startup (sess, 1);
	if (rc) return rc;

	rc = ndmca_connect_tape_agent(sess);
	if (rc) return rc;

	rc = ndmca_media_load_first (sess);
	if (rc) return rc;

	ndmca_tape_close (sess);

	return 0;
}

int
ndmca_test_unload_tape (struct ndm_session *sess)
{
	ndmca_tape_open (sess);

	ndmca_media_unload_current(sess);

	return 0;
}

int
ndmca_test_check_expect (struct ndmconn *conn, int rc, ndmp9_error expect_err)
{
	struct ndm_session *sess = conn->context;
	struct ndmp_xa_buf *xa = &conn->call_xa_buf;
	int		protocol_version = conn->protocol_version;
	int		msg = (int) conn->last_message;
	char *		msgname = ndmp_message_to_str (protocol_version, msg);
	ndmp9_error	reply_error = conn->last_reply_error;

	if (rc >= 0) {
		/* Call succeeded. Body valid */
		if (reply_error == expect_err) {
			/* Worked exactly as expected */
			rc = 0;
		} else if (reply_error != NDMP9_NO_ERR
		        && expect_err != NDMP9_NO_ERR) {
			/* both are errors, don't be picky about the codes */
			rc = 2;
		} else {
			/* intolerable mismatch */
		}
	}

	if (rc == 0) {
		ndmalogf (sess, "Test", 2,
			"%s #%d -- Passed %s/%s",
			sess->control_acb.test_phase,
			sess->control_acb.test_step,
			msgname,
			ndmp9_error_to_str (expect_err));
		sess->control_acb.n_step_pass++;
	} else {
		ndmalogf (sess, "Test", 1,
			"%s #%d -- %s %s/%s got %s",
			sess->control_acb.test_phase,
			sess->control_acb.test_step,
			(rc == 2) ? "Almost" : "Failed",
			msgname,
			ndmp9_error_to_str (expect_err),
			ndmp9_error_to_str (reply_error));

		ndma_tattle (conn, xa, rc);

		if (rc == 2) {
			rc = 0;
			sess->control_acb.n_step_warn++;
		} else {
			sess->control_acb.n_step_fail++;
		}
	}

	sess->control_acb.test_step++;

	return rc;
}


int
ndmca_test_call (struct ndmconn *conn,
  struct ndmp_xa_buf *xa, ndmp9_error expect_err)
{
	struct ndm_session *sess = conn->context;
	int		protocol_version = conn->protocol_version;
	unsigned	msg = xa->request.header.message;
	char *		msgname = ndmp_message_to_str (protocol_version, msg);
	unsigned	reply_error;
	int		rc;

	rc = ndma_call_no_tattle (conn, xa);

	reply_error = ndmnmb_get_reply_error (&xa->reply);

	if (rc >= 0) {
		/* Call succeeded. Body valid */
		if (reply_error == expect_err) {
			/* Worked exactly as expected */
			rc = 0;
		} else if (reply_error != NDMP9_NO_ERR
		        && expect_err != NDMP9_NO_ERR) {
			/* both are errors, don't be picky about the codes */
			rc = 2;
		} else {
			/* intolerable mismatch */
		}
	}

	if (rc == 0) {
		ndmalogf (sess, "Test", 2,
			"%s #%d -- Passed %s/%s",
			sess->control_acb.test_phase,
			sess->control_acb.test_step,
			msgname,
			ndmp9_error_to_str (expect_err));
		sess->control_acb.n_step_pass++;
	} else {
		ndmalogf (sess, "Test", 1,
			"%s #%d -- %s %s/%s got %s",
			sess->control_acb.test_phase,
			sess->control_acb.test_step,
			(rc == 2) ? "Almost" : "Failed",
			msgname,
			ndmp9_error_to_str (expect_err),
			ndmp9_error_to_str (reply_error));

		ndma_tattle (conn, xa, rc);

		if (rc == 2) {
			rc = 0;
			sess->control_acb.n_step_warn++;
		} else {
			sess->control_acb.n_step_fail++;
		}
	}

	sess->control_acb.test_step++;

	return rc;
}

void
ndmca_test_phase (struct ndm_session *sess, char *test_phase, char *desc)
{
	ndmalogf (sess, "TEST", 0, "Test %s -- %s", test_phase, desc);
	sess->control_acb.test_phase = test_phase;
	sess->control_acb.test_step = 1;
	sess->control_acb.n_step_pass = 0;
	sess->control_acb.n_step_fail = 0;
	sess->control_acb.n_step_warn = 0;
}

void
ndmca_test_log_step (struct ndm_session *sess, int level, char *msg)
{
	ndmalogf (sess, "Test", level, "%s #%d -- %s",
		sess->control_acb.test_phase,
		sess->control_acb.test_step,
		msg);
	sess->control_acb.test_step++;
}

void
ndmca_test_log_note (struct ndm_session *sess, int level, char *msg)
{
	ndmalogf (sess, "Test", level, "%s #%d %s",
		sess->control_acb.test_phase,
		sess->control_acb.test_step,
		msg);
}

void
ndmca_test_done_phase (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	char *			status;

	if (ca->n_step_fail)
		status = "Failed";
	else if (ca->n_step_warn)
		status = "Almost";
	else if (ca->n_step_pass > 0)
		status = "Passed";
	else
		status = "Whiffed";

	ndmalogf (sess, "TEST", 0, "Test %s %s -- pass=%d warn=%d fail=%d",
			ca->test_phase,
			status,
			ca->n_step_pass,
			ca->n_step_warn,
			ca->n_step_fail);

	ca->total_n_step_pass += ca->n_step_pass;
	ca->total_n_step_warn += ca->n_step_warn;
	ca->total_n_step_fail += ca->n_step_fail;
}

void
ndmca_test_done_series (struct ndm_session *sess, char *series_name)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	char *			status;

	if (ca->total_n_step_fail)
		status = "Failed";
	else if (ca->total_n_step_warn)
		status = "Almost";
	else
		status = "Passed";

	ndmalogf (sess, "TEST", 0, "FINAL %s %s -- pass=%d warn=%d fail=%d",
			series_name,
			status,
			ca->total_n_step_pass,
			ca->total_n_step_warn,
			ca->total_n_step_fail);
}



void
ndmca_test_pass (struct ndm_session *sess)
{
	ndmalogf (sess, "TEST", 0, "Test %s Passed",
			sess->control_acb.test_phase);
}

void
ndmca_test_fail (struct ndm_session *sess)
{
	ndmalogf (sess, "TEST", 0, "Test %s Failed",
			sess->control_acb.test_phase);
}



void
ndmca_test_fill_data (char *buf, int bufsize, int recno, int fileno)
{
	char *		src;
	char *		srcend;
	char *		dst = buf;
	char *		dstend = buf+bufsize;
	unsigned short	sequence = 0;
	struct {
		unsigned short	fileno;
		unsigned short	sequence;
		unsigned long	recno;
	}		x;

	x.fileno = fileno;
	x.recno = recno;

	srcend = (char *) &x;
	srcend += sizeof x;

	while (dst < dstend) {
		x.sequence = sequence++;
		src = (char *) &x;

		while (src < srcend && dst < dstend)
			*dst++ = *src++;
	}
}
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */
