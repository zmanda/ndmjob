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
 * Ident:    $Id: ndma_cops_backreco.c,v 1.1 2004/01/12 18:06:34 ern Exp $
 *
 * Description:
 *
 */


#include "ndmagents.h"


#ifndef NDMOS_OPTION_NO_CONTROL_AGENT


int
ndmca_op_create_backup (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc;

	ca->tape_mode = NDMP9_TAPE_RDWR_MODE;
	ca->mover_mode = NDMP9_MOVER_MODE_READ;
	ca->is_label_op = 0;

	rc = ndmca_backreco_startup (sess);
	if (rc) return rc;

	rc = ndmca_data_start_backup (sess);
	if (rc == 0) {
	    rc = ndmca_monitor_startup (sess);
	    if (rc == 0) {
		rc = ndmca_monitor_backup (sess);
	    }
	}

	if (rc == 0)
	    rc = ndmca_monitor_shutdown (sess);
	else
	    ndmca_monitor_shutdown (sess);

	ndmca_media_tattle (sess);

	return rc;
}

int
ndmca_op_recover_files (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc;

	ca->tape_mode = NDMP9_TAPE_READ_MODE;
	ca->mover_mode = NDMP9_MOVER_MODE_WRITE;
	ca->is_label_op = 0;

	rc = ndmca_backreco_startup (sess);
	if (rc) return rc;

	rc = ndmca_data_start_recover (sess);
	if (rc == 0) {
		rc = ndmca_monitor_startup (sess);
		if (rc == 0) {
			rc = ndmca_monitor_recover (sess);
		}
	}

	if (rc == 0)
	    rc = ndmca_monitor_shutdown (sess);
	else
	    ndmca_monitor_shutdown (sess);

	if (rc == 0) {
	    if (ca->recover_log_file_count > 0) {
		struct ndm_control_agent *ca = &sess->control_acb;
		int		    n_nlist = ca->job.nlist_tab.n_nlist;

		ndmalogf (sess, 0, 0,
			  "LOG_FILE messages: %d OK, %d ERROR, total %d of %d",
			  ca->recover_log_file_ok,
			  ca->recover_log_file_error,
			  ca->recover_log_file_count,
			  n_nlist);
		if (ca->recover_log_file_ok < n_nlist) {
		    rc = 1;
		}
	    } else {
		ndmalogf (sess, 0, 1,
			  "DATA did not report any LOG_FILE messages");
	    }
	}
	
	ndmca_media_tattle (sess);

	return rc;
}

int
ndmca_op_recover_fh (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc;

	ca->tape_mode = NDMP9_TAPE_READ_MODE;
	ca->mover_mode = NDMP9_MOVER_MODE_WRITE;
	ca->is_label_op = 0;

	rc = ndmca_backreco_startup (sess);
	if (rc) return rc;

	rc = ndmca_data_start_recover_filehist (sess);
	if (rc == 0) {
		rc = ndmca_monitor_startup (sess);
		if (rc == 0) {
			rc = ndmca_monitor_recover (sess);
		}
	}

	if (rc == 0)
	    rc = ndmca_monitor_shutdown (sess);
	else
	    ndmca_monitor_shutdown (sess);

	ndmca_media_tattle (sess);

	return rc;
}

char *ndmca_data_est(struct ndm_control_agent *ca)
{
    char *estb;
    static char estb_buf[64];

    estb = 0;
    if (ca->data_state.est_bytes_remain.valid &&
	(ca->data_state.est_bytes_remain.value >= 1024)) {
	snprintf(estb_buf,
		 sizeof (estb_buf),
		 " left %lldKB",
		 ca->data_state.est_bytes_remain.value/1024LL);
	estb = estb_buf;
    }

    return estb;
}

int
ndmca_monitor_backup (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	int			count;
	ndmp9_data_state	ds;
	ndmp9_mover_state	ms;
	char *estb;

	ndmalogf (sess, 0, 3, "Monitoring backup");

	for (count = 0; count < 10; count++) {
		ndmca_mon_wait_for_something (sess, count <= 1 ? 30 : 10);
		if (ndmca_monitor_get_states(sess) < 0)
		    break;

#if 0
		if (count > 2)
			ndmca_mon_show_states(sess);
#endif

		ds = ca->data_state.state;
		ms = ca->mover_state.state;

		estb = ndmca_data_est(ca);

		ndmalogf (sess, 0, 1,
			  "DATA: bytes %lldKB%s  MOVER: written %lldKB record %d",
			  ca->data_state.bytes_processed/1024LL,
			  estb ? estb : "",
			  ca->mover_state.bytes_moved/1024LL,
			  ca->mover_state.record_num);

		if (ds == NDMP9_DATA_STATE_ACTIVE
		 && ms == NDMP9_MOVER_STATE_ACTIVE) {
			count = 0;
			continue;
		}

		/*
		 * Check MOVER for needed tape change during DATA_FLOW_TO_TAPE.
		 * Have to do this before checking DATA. Even if DATA halted,
		 * MOVER may be holding unwritten data. Have to perform
		 * the tape change.
		 */
		if (ms == NDMP9_MOVER_STATE_PAUSED) {
			ndmp9_mover_pause_reason	pr;

			pr = ca->mover_state.pause_reason;

			if (!ca->pending_notify_mover_paused) {
				/* count=count */
				continue;		/* wait for notice */
			}

			ca->pending_notify_mover_paused = 0;

			ndmalogf (sess, 0, 3, "Mover paused, reason=%s",
					ndmp9_mover_pause_reason_to_str (pr));

			/* backups are different then recoverys... When
                         * we reach the end of a window, we signal EOW
                         * except in V2 where we signal EOF. EOM occurs
			 * at EOT (or EOF does).
			 * This is based on reading comments in the email
			 * archives...
			 */
			if ((pr == NDMP9_MOVER_PAUSE_EOM) ||
			    (pr == NDMP9_MOVER_PAUSE_EOW)) {
				if (ndmca_monitor_load_next(sess) == 0) {
					/* count=count */
					continue;	/* Happy */
				}
				/* Something went wrong with tape change. */
			} else if ((sess->plumb.tape->protocol_version <= 2) &&
				   pr == NDMP9_MOVER_PAUSE_EOF) {
				if (ndmca_monitor_load_next(sess) == 0) {
					/* count=count */
					continue;	/* Happy */
				}
				/* Something went wrong with tape change. */
			} else {
				/* All other pause reasons
				 * are critically bogus. */

			}
			ndmalogf (sess, 0, 0,
				"Operation paused w/o remedy, cancelling");
			ndmca_mover_abort (sess);
			return -1;
		}

		/*
		 * If DATA has halted, the show is over.
		 */
		if (ds == NDMP9_DATA_STATE_HALTED) {
			if (ms != NDMP9_MOVER_STATE_HALTED) {
				ndmalogf (sess, 0, 3,
					"DATA halted, MOVER active");
				/*
				 * MOVER still occupied. It might be a
				 * heartbeat away from asking for another
				 * tape. Give it a chance.
				 */
				continue;
			}

			ndmalogf (sess, 0, 2, "Operation done, cleaning up");

			ndmca_monitor_get_post_backup_env (sess);

			return 0;
		}
#if 1
		if (ms == NDMP9_MOVER_STATE_HALTED) {
			if (ds == NDMP9_DATA_STATE_ACTIVE) {
				ndmalogf (sess, 0, 3,
					"MOVER halted, DATA active");
				/*
				 * DATA still occupied. 
				 */
				continue;
			}
		}
#endif

		if (ms != NDMP9_MOVER_STATE_ACTIVE && count == 0) {
			/* Not active. Not paused. Something wrong */
			ndmalogf (sess, 0, 0,
				"Operation in unreasonable state, cancelling");
			return -1;
		}
	}

	ndmalogf (sess, 0, 0, "Operation monitoring mishandled, cancelling");
	return -1;
}

int
ndmca_monitor_get_post_backup_env (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	struct ndmlog *		ixlog = &ca->job.index_log;
	int			rc, i;
	ndmp9_pval *		pv;

	rc = ndmca_data_get_env (sess);
	if (rc) {
		ndmalogf (sess, 0, 0, "fetch post backup env failed");
		return -1;
	}

	for (i = 0; i < ca->job.result_env_tab.n_env; i++) {
		pv = &ca->job.result_env_tab.env[i];

		ndmlogf (ixlog, "DE", 0, "%s=%s", pv->name, pv->value);
	}

	return 0;
}

int
ndmca_monitor_recover (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	int			count, rc;
	ndmp9_data_state	ds;
	ndmp9_mover_state	ms;
	char *estb;
	int last_state_print = 0;

	ndmalogf (sess, 0, 3, "Monitoring recover");

	for (count = 0; count < 10; count++) {
		if (ca->pending_notify_data_read) {
			ca->pending_notify_data_read = 0;

			rc = ndmca_mover_read (sess,
				ca->last_notify_data_read.offset,
				ca->last_notify_data_read.length);
			if (rc) {
				ndmalogf (sess, 0, 0, "data-read failed");
				return -1;
			}
			if (count < 5)
				continue;
		}

		ndmca_mon_wait_for_something (sess, count <= 1 ? 30 : 10);

		if (ndmca_monitor_get_states(sess) < 0)
		    break;

#if 0
		if (count > 2)
			ndmca_mon_show_states(sess);
#endif

		ds = ca->data_state.state;
		ms = ca->mover_state.state;

		estb = ndmca_data_est(ca);

		if ((ds != NDMP9_DATA_STATE_ACTIVE) ||
		    (ms != NDMP9_MOVER_STATE_ACTIVE) ||
		    ((time(0) - last_state_print) >= 5)) {

		    ndmalogf (sess, 0, 1,
			      "DATA: bytes %lldKB%s  MOVER: read %lldKB record %d",
			      ca->data_state.bytes_processed/1024LL,
			      estb ? estb : "",
			      ca->mover_state.bytes_moved/1024LL,
			      ca->mover_state.record_num);
		    last_state_print = time(0);
		}

		if (ds == NDMP9_DATA_STATE_ACTIVE
		 && ms == NDMP9_MOVER_STATE_ACTIVE) {
			count = 0;
			continue;
		}

		/*
		 * Check MOVER for needed tape change during DATA_FLOW_TO_TAPE.
		 * Have to do this before checking DATA. Even if DATA halted,
		 * MOVER may be holding unwritten data. Have to perform
		 * the tape change.
		 */
		if (ms == NDMP9_MOVER_STATE_PAUSED) {
			ndmp9_mover_pause_reason	pr;

			pr = ca->mover_state.pause_reason;

			if (!ca->pending_notify_mover_paused) {
				/* count=count */
				continue;		/* wait for notice */
			}

			ca->pending_notify_mover_paused = 0;

			ndmalogf (sess, 0, 3, "Mover paused, reason=%s",
					ndmp9_mover_pause_reason_to_str (pr));

			if (((pr == NDMP9_MOVER_PAUSE_EOF) ||
			     (pr == NDMP9_MOVER_PAUSE_SEEK))
			    && (ca->cur_media_ix+1 == ca->job.media_tab.n_media)) {
				/*
				 * Last tape consumed by tape agent.
				 * The DATA agent may be just shy
				 * of done, but there is no way for
				 * us to tell. So, close the
				 * image stream from the TAPE
				 * agent side, thus indicating
				 * EOF to the DATA agent.
				 */
				ndmalogf (sess, 0, 2, "End of tapes");
				ndmca_mover_close (sess);
				/* count=count */
				continue;
			}

			if (pr == NDMP9_MOVER_PAUSE_EOM
			 || pr == NDMP9_MOVER_PAUSE_EOF) {
				if (ndmca_monitor_load_next(sess) == 0) {
					/* count=count */
					continue;	/* Happy */
				}
				/* Something went wrong with tape change. */
			} else if (pr == NDMP9_MOVER_PAUSE_SEEK) {
				if (ndmca_monitor_seek_tape(sess) == 0) {
					/* count=count */
					continue;	/* Happy */
				}
				/* Something went wrong with tape change. */
			} else {
				/* All other pause reasons
				 * are critically bogus. */
			}
			ndmalogf (sess, 0, 0,
				"Operation paused w/o remedy, cancelling");
			ndmca_mover_abort (sess);
			return -1;
		}

		/*
		 * If DATA has halted, the show is over.
		 */
		if (ds == NDMP9_DATA_STATE_HALTED) {
			if (ms != NDMP9_MOVER_STATE_HALTED) {
				ndmalogf (sess, 0, 3,
					"DATA halted, MOVER active");
				/*
				 * MOVER still occupied. It might
				 * figure it out. Then again, it might
				 * be awaiting a MOVER_READ. The NDMP
				 * design does not provide a state
				 * for awaiting MOVER_READ, so we have
				 * to guess.
				 */
				if (count > 0) {
					ndmca_mover_close(sess);
				}
				continue;
			}

			ndmalogf (sess, 0, 2, "Operation done, cleaning up");

			ndmca_monitor_get_post_backup_env (sess);

			return 0;
		}

		if (ms != NDMP9_MOVER_STATE_ACTIVE && count == 0) {
			/* Not active. Not paused. Something wrong */
			ndmalogf (sess, 0, 0,
				"Operation in unreasonable state, cancelling");
			return -1;
		}
	}

	ndmalogf (sess, 0, 0, "Operation monitoring mishandled, cancelling");
	return -1;
}


int
ndmca_backreco_startup (struct ndm_session *sess)
{
	int			rc;

	rc = ndmca_op_robot_startup (sess, 1);
	if (rc) return rc;

	rc = ndmca_connect_data_agent(sess);
	if (rc) return rc;

	rc = ndmca_connect_tape_agent(sess);
	if (rc) return rc;

	rc = ndmca_mover_set_record_size (sess);
	if (rc) return rc;

	rc = ndmca_media_load_first (sess);
	if (rc) return rc;

	ndmca_media_calculate_offsets (sess);

	if (sess->control_acb.swap_connect &&
	    (sess->plumb.tape->protocol_version >= 3)) {
	    if (sess->plumb.tape->protocol_version < 4) {
		rc = ndmca_data_listen (sess);
		if (rc) return rc;

		rc = ndmca_media_set_window_current (sess);
		if (rc) return rc;
	    } else {
		rc = ndmca_media_set_window_current (sess);
		if (rc) return rc;

		rc = ndmca_data_listen (sess);
		if (rc) return rc;
	    }
	} else {
	    if (sess->plumb.tape->protocol_version < 4) {
		rc = ndmca_mover_listen (sess);
		if (rc) return rc;

		rc = ndmca_media_set_window_current (sess);
		if (rc) return rc;
	    } else {
		rc = ndmca_media_set_window_current (sess);
		if (rc) return rc;

		rc = ndmca_mover_listen (sess);
		if (rc) return rc;
	    }
	}

	return 0;
}

int
ndmca_monitor_startup (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	ndmp9_data_state	ds;
	ndmp9_mover_state	ms;
	int			count;

	ndmalogf (sess, 0, 3, "Waiting for operation to start");

	for (count = 0; count < 10; count++) {
		if (ndmca_monitor_get_states (sess) < 0)
		    break;

		ds = ca->data_state.state;
		ms = ca->mover_state.state;

		if (ds == NDMP9_DATA_STATE_ACTIVE
		 && ms == NDMP9_MOVER_STATE_ACTIVE) {
			ndmalogf (sess, 0, 1, "Operation started");
			return 0;
		}

		if (ds == NDMP9_DATA_STATE_HALTED
		    && ms == NDMP9_MOVER_STATE_HALTED) {
			/* operation finished immediately */
			return 0;
		}

		if (ds != NDMP9_DATA_STATE_IDLE
		 && ms != NDMP9_MOVER_STATE_IDLE
		 && ms != NDMP9_MOVER_STATE_LISTEN) {
			ndmalogf (sess, 0, 1,
				  "Operation started in unusual fashion");
			return 0;
		}

		ndmca_mon_wait_for_something (sess, 2);
	}

	ndmalogf (sess, 0, 0, "Operation failed to start");
	return -1;
}

/*
 * Just make sure things get finished
 */
int
ndmca_monitor_shutdown (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	ndmp9_data_state	ds;
	ndmp9_data_halt_reason	dhr;
	ndmp9_mover_state	ms;
	ndmp9_mover_halt_reason	mhr;
	int			count;
	int			finish;

	ndmalogf (sess, 0, 3, "Waiting for operation to halt");

	for (count = 0; count < 10; count++) {
		ndmca_mon_wait_for_something (sess, 2);

		if (ndmca_monitor_get_states (sess) < 0)
		    break;

#if 0
		if (count > 2)
			ndmca_mon_show_states(sess);
#endif

		ds = ca->data_state.state;
		ms = ca->mover_state.state;

		if (ds == NDMP9_DATA_STATE_HALTED
		 && ms == NDMP9_MOVER_STATE_HALTED) {
		        dhr = ca->data_state.halt_reason;
			mhr = ca->mover_state.halt_reason;
			break;
		}

		if (count > 2) {
			if (ds != NDMP9_DATA_STATE_HALTED)
				ndmca_data_abort(sess);
			if (ms != NDMP9_MOVER_STATE_HALTED)
				ndmca_mover_abort(sess);
		}
	}

	if (ca->tape_state.error == NDMP9_NO_ERR) {
		ndmca_monitor_unload_last_tape (sess);
	}

	if (count >= 10) {
		ndmalogf (sess, 0, 0,
			"Operation did not halt, something wrong");
	}

	ndmalogf (sess, 0, 2, "Operation halted, stopping");

	ds = ca->data_state.state;
	ms = ca->mover_state.state;
	dhr = ca->data_state.halt_reason;
	mhr = ca->mover_state.halt_reason;

	if ((ds == NDMP9_DATA_STATE_HALTED)
	    && (ms == NDMP9_MOVER_STATE_HALTED)) {
	    if ((dhr == NDMP9_DATA_HALT_SUCCESSFUL) &&
		(mhr == NDMP9_MOVER_HALT_CONNECT_CLOSED)) {
		/* Successful operation */
		ndmalogf (sess, 0, 0, "Operation ended OKAY");
		finish = 0;
	    } else {
		/* Questionable success */
		ndmalogf (sess, 0, 0, "Operation ended questionably");
		finish = 1;
	    }
	} else {
	    ndmalogf (sess, 0, 0, "Operation ended in failure");
	    finish = -1;
	}

	ndmca_data_stop (sess);
	ndmca_mover_stop (sess);

	for (count = 0; count < 10; count++) {
		if (ndmca_monitor_get_states(sess) < 0)
		    break;

#if 0
		if (count > 2)
			ndmca_mon_show_states(sess);
#endif

		ds = ca->data_state.state;
		ms = ca->mover_state.state;

		if (ds == NDMP9_DATA_STATE_IDLE
		 && ms == NDMP9_MOVER_STATE_IDLE) {
			break;
		}
	}

	if (count >= 10) {
		ndmalogf (sess, 0, 0,
			"Operation did not stop, something wrong");
		return -1;
	}

	return finish;
}

int
ndmca_monitor_get_states (struct ndm_session *sess)
{
    	int rc = 0;


	if (ndmca_data_get_state (sess) < 0)
	    rc = -1;
	if (ndmca_mover_get_state (sess) < 0)
	    rc = -1;
	ndmca_tape_get_state_no_tattle (sess);

	return rc;
}

int
ndmca_monitor_load_next (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc;

	ndmalogf (sess, 0, 1, "Operation requires next tape");

	ndmca_media_capture_mover_window (sess);
	ndmca_media_calculate_offsets (sess);

	if (ca->tape_mode == NDMP9_TAPE_RDWR_MODE) {
	    if (ca->mover_state.pause_reason != NDMP9_MOVER_PAUSE_EOM)
		ndmca_media_write_filemarks (sess);
	    else
		ndmalogf (sess, 0, 1, "At EOM, not writing filemarks");
	}

	rc = ndmca_media_unload_current(sess);
	if (rc) return rc;

	rc = ndmca_media_load_next(sess);
	if (rc) return rc;

	rc = ndmca_media_set_window_current (sess);
	if (rc) return rc;

	rc = ndmca_mover_continue(sess);
	if (rc) return rc;

	ndmalogf (sess, 0, 1, "Operation resuming");

	return 0;
}

/* VERY VERY HARD */
int
ndmca_monitor_seek_tape (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc;
	unsigned long long	pos;

	pos = ca->last_notify_mover_paused.seek_position;

	ndmalogf (sess, 0, 1, "Operation requires a different tape");

/*	ndmca_media_capture_mover_window (sess);	// !!! */
	ndmca_media_calculate_offsets (sess);

	rc = ndmca_media_unload_current(sess);
	if (rc) return rc;

	rc = ndmca_media_load_seek (sess, pos);
	if (rc) return rc;

	rc = ndmca_media_set_window_current (sess);
	if (rc) return rc;

	rc = ndmca_mover_continue(sess);
	if (rc) return rc;

	ndmalogf (sess, 0, 1, "Operation resuming");

	return 0;
}

int
ndmca_monitor_unload_last_tape (struct ndm_session *sess)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	int			rc;

	if (!ca->media_is_loaded)
		return 0;

	ndmca_media_capture_mover_window (sess);
	ndmca_media_calculate_offsets (sess);

	if (ca->tape_mode == NDMP9_TAPE_RDWR_MODE) {
		ndmca_media_write_filemarks (sess);
	}

	rc = ndmca_media_unload_current(sess);
	if (rc) return rc;

	return 0;
}

int
ndmca_mon_wait_for_something (struct ndm_session *sess, int max_delay_secs)
{
	struct ndm_control_agent *ca = &sess->control_acb;
	int			delta, notices;
	int			time_ref = time(0) + max_delay_secs;

	ndmalogf (sess, 0, 5, "mon_wait_for_something() entered");

	for (;;) {
		delta = time_ref - time(0);
		if (delta <= 0)
			break;

		notices = 0;
		if (ca->pending_notify_data_read) {
			/* leave visible */
			notices++;
		}
		if (ca->pending_notify_data_halted) {
			/* just used to "wake up" */
			ca->pending_notify_data_halted = 0;
			notices++;
		}
		if (ca->pending_notify_mover_paused) {
			/* leave visible */
			notices++;
		}
		if (ca->pending_notify_mover_halted) {
			/* just used to "wake up" */
			ca->pending_notify_mover_halted = 0;
			notices++;
		}

		ndma_session_quantum (sess, notices ? 0 : delta);

		if (notices)
			break;
	}

	ndmalogf (sess, 0, 5, "mon_wait_for_something() happened, resid=%d",
			delta);

	return 0;
}
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */
