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
 * Ident:    $Id: ndma_data_recover.c,v 1.1.1.1 2003/10/14 19:12:39 ern Exp $
 *
 * Description:
 *
 */


#include "ndmagents.h"
#include "snoop_tar.h"

#ifndef NDMOS_OPTION_NO_DATA_AGENT






int
ndmda_quantum_recover_common (struct ndm_session *sess)
{
	struct ndm_data_agent *		da = &sess->data_acb;
	struct ndm_data_recover_cb *	reco = &da->recover_cb;
	int				rc;
	int				did_something = 0;

	for (;;) {
		if (da->data_state.state != NDMP9_DATA_STATE_ACTIVE)
			break;

		rc = ndmda_reco_assess_channels (sess);
		if (rc)
			return 0;	/* can't proceed */

		rc = ndmda_reco_assess_intervals (sess);
		if (rc)
			return 0;	/* can't proceed */

		switch (reco->state) {
		default:
			return ndmda_reco_internal_error (sess,
					"reco->state invalid");

		case NDMDA_RECO_STATE_START:
			rc = ndmda_reco_state_start (sess);
			break;

		case NDMDA_RECO_STATE_PASS_THRU:
			rc = ndmda_reco_state_pass_thru (sess);
			break;

		case NDMDA_RECO_STATE_CHOOSE_NLENT:
			rc = ndmda_reco_state_choose_nlent (sess);
			break;

		case NDMDA_RECO_STATE_ACQUIRE:
			rc = ndmda_reco_state_acquire (sess);
			break;

		case NDMDA_RECO_STATE_DISPOSE:
			rc = ndmda_reco_state_dispose (sess);
			break;

		case NDMDA_RECO_STATE_FINISH_NLENT:
			rc = ndmda_reco_state_finish_nlent (sess);
			break;

		case NDMDA_RECO_STATE_ALL_DONE:
			rc = ndmda_reco_state_all_done (sess);
			break;
		}

		if (rc <= 0)
			break;
		did_something++;
	}

	return did_something;
}


int
ndmda_reco_state_start (struct ndm_session *sess)
{
	struct ndm_data_agent *		da = &sess->data_acb;
	struct ndm_data_recover_cb *	reco = &da->recover_cb;

	if (!reco->enable_direct) {
		reco->access_method = NDMDA_RECO_ACCESS_SEQUENTIAL;
	} else {
		reco->access_method = NDMDA_RECO_ACCESS_DIRECT;
	}

	/*
	 * TODO: more to look at than HIST=
	 */
	if (da->enable_hist) {
		reco->acquire_mode = NDMDA_RECO_ACQUIRE_EVERYTHING;
		reco->state = NDMDA_RECO_STATE_ACQUIRE;
	} else {
		reco->acquire_mode = NDMDA_RECO_ACQUIRE_EVERYTHING;
		reco->state = NDMDA_RECO_STATE_PASS_THRU;
		reco->want.offset = 0;
		reco->want.length = NDMP_LENGTH_INFINITY;
		reco->access_method = NDMDA_RECO_ACCESS_SEQUENTIAL;
	}

	if (reco->access_method == NDMDA_RECO_ACCESS_SEQUENTIAL) {
		ndmda_reco_send_data_read (sess, 0ULL, NDMP_LENGTH_INFINITY);
	}

	return 1; /* did something */
	return 0; /* wait our turn */
}

int
ndmda_reco_pass_thru (struct ndm_session *sess)
{
	struct ndm_data_agent *		da = &sess->data_acb;
	struct ndm_data_recover_cb *	reco = &da->recover_cb;
	struct ndmchan *		image_chan =
						&sess->plumb.image_stream.chan;
	struct ndmchan *		format_chan = &da->formatter_image;
	int				n_copy = reco->n_ready;
	int				n_avail= ndmchan_n_avail(format_chan);

	if (n_copy > n_avail)
		n_copy = n_avail;

	if (n_copy > reco->want.length)
		n_copy = reco->want.length;

	if (n_copy == 0)
		return 0;

	NDMOS_API_BCOPY (
		&image_chan->data[image_chan->beg_ix],
		&format_chan->data[format_chan->end_ix],
		n_copy);

	image_chan->beg_ix += n_copy;
	format_chan->end_ix += n_copy;
	reco->want.offset += n_copy;
	reco->want.length -= n_copy;
	reco->have.offset += n_copy;
	reco->have.length -= n_copy;
	reco->total_passed += n_copy;
	da->data_state.bytes_processed += n_copy;

	return 1;
}

int
ndmda_reco_state_pass_thru (struct ndm_session *sess)
{
	struct ndm_data_agent *		da = &sess->data_acb;
	struct ndm_data_recover_cb *	reco = &da->recover_cb;

	if (reco->expect.length == 0) {
		reco->state = NDMDA_RECO_STATE_ALL_DONE;
		return 1;
	}

	return ndmda_reco_pass_thru (sess);
}

int
ndmda_reco_state_choose_nlent (struct ndm_session *sess)
{
	return ndmda_reco_internal_error (sess, "choose_nlent");
}

int
ndmda_reco_state_acquire (struct ndm_session *sess)
{
	struct ndm_data_agent *		da = &sess->data_acb;
	struct ndm_data_recover_cb *	reco = &da->recover_cb;
	struct tar_digest_hdr		tdh;
	int				rc;

	rc = ndmda_reco_align_to_wanted (sess);
	if (rc <= 0)
		return rc;

	rc = tdh_digest (reco->data, reco->n_ready, &tdh);
	ndmalogf (sess, 0, 7, "tdh_digest=%d", rc);
	if (rc < 0) {
		/* Header is all wrong. Could try to recover. */
		return ndmda_reco_internal_error (sess, "tar image corrupted");
	}

	if (rc > 0) {
		/*
		 * rc is how many records (512 bytes)
		 * are needed to digest header.
		 */
		reco->want.length = rc * RECORDSIZE;
		rc = ndmda_reco_obtain_wanted (sess);
		return rc;
	}

	reco->object_size = tdh.n_header_records;
	reco->object_size += tdh.n_content_records;
	reco->object_size *= RECORDSIZE;

	/* rc==0, header digested */
	if (tdh.is_end_marker) {
		/* reached end of tar image */

		if (da->enable_hist)
			ndmda_fh_flush (sess);

		switch (reco->acquire_mode) {
		case NDMDA_RECO_ACQUIRE_EVERYTHING:
			/* we're done */
			reco->disposition = NDMDA_RECO_DISPOSITION_PASS;
			reco->state_after_disposition =
						NDMDA_RECO_STATE_ALL_DONE;
			reco->want.length = reco->object_size;
			return 0;

		case NDMDA_RECO_ACQUIRE_SEARCHING:
			/* we're done */
			reco->disposition = NDMDA_RECO_DISPOSITION_PASS;
			reco->state_after_disposition =
						NDMDA_RECO_STATE_ALL_DONE;
			reco->want.length = reco->object_size;
			return 0;

		case NDMDA_RECO_ACQUIRE_MATCHING:
			/* Choose another entry (there probably isn't one) */
			reco->state = NDMDA_RECO_STATE_FINISH_NLENT;
			return 0;

		default:
			/* uh-oh, we're done */
			return ndmda_reco_internal_error (sess,
					"aquire mode at soft EOF");
		}
		/*notreached*/
	}

#if 0
//	reco->acquired_name = tdh.name;
#endif

	switch (reco->acquire_mode) {
	case NDMDA_RECO_ACQUIRE_SEARCHING:
		/*
		 * Scan all nlent[] entries and see
		 * if this matches any of them.
		 * If it does, pass it.
		 * If it does not,
		 *     if previous disposition was PASS
		 *         this is an PASS->DISCARD edge
		 *         discard this, keep searching
		 *     else
		 *         discard this, keep searching
		 */
		/* uh-oh, we're toast */
		return ndmda_reco_internal_error (sess, "acquire-searching");

	case NDMDA_RECO_ACQUIRE_MATCHING:
		/*
		 * Match against current nlent[].
		 * If it matches, pass it.
		 * If not, call this nlent[] done
		 * and CHOOSE again.
		 */
		/* uh-oh, we're toast */
		return ndmda_reco_internal_error (sess, "acquire-searching");

	case NDMDA_RECO_ACQUIRE_EVERYTHING:
		/*
		 * Boring pass-thru or history enabled
		 */
		reco->disposition = NDMDA_RECO_DISPOSITION_PASS;
		reco->state_after_disposition = NDMDA_RECO_STATE_ACQUIRE;
		reco->want.length = reco->object_size;
		break;

	default:
		/* uh-oh, we're done */
		return ndmda_reco_internal_error (sess, "aquire aquire_mode");
	}

	/* both reco->disposition and reco->state_after_disposition are set */

	if (reco->disposition == NDMDA_RECO_DISPOSITION_PASS
	 && da->enable_hist) {
		ndmda_gtar_fh_add (sess, &tdh);
	}

	reco->state = NDMDA_RECO_STATE_DISPOSE;

	return 1;	/* did something, do more */
}

int
ndmda_reco_state_dispose (struct ndm_session *sess)
{
	struct ndm_data_agent *		da = &sess->data_acb;
	struct ndm_data_recover_cb *	reco = &da->recover_cb;
	int				rc;


	switch (reco->disposition) {
	default:
		/* uh-oh, we're done */
		return ndmda_reco_internal_error (sess,
					"disposition at DISPOSE");

	case NDMDA_RECO_DISPOSITION_PASS:
		if (da->data_state.operation == NDMP9_DATA_OP_RESTORE) {
			if (reco->want.length > 0) {
				rc = ndmda_reco_align_to_wanted (sess);
				if (rc <= 0)
					return rc;
				rc = ndmda_reco_obtain_wanted (sess);
				if (rc < 0)
					return rc;
				rc = ndmda_reco_pass_thru (sess);
			} else {
				reco->state = reco->state_after_disposition;
				rc = 1;
			}
			break;
		}
		/* fall through to.... */

	case NDMDA_RECO_DISPOSITION_DISCARD:
		reco->total_discarded += reco->object_size;
		reco->want.offset += reco->object_size;
		reco->want.length -= reco->object_size;
		da->data_state.bytes_processed += reco->object_size;
		reco->state = reco->state_after_disposition;
		rc = 1;
		break;
	}

	return rc;
}

int
ndmda_reco_state_finish_nlent (struct ndm_session *sess)
{
	return ndmda_reco_internal_error (sess, "finish_nlent");
}

int
ndmda_reco_state_all_done (struct ndm_session *sess)
{
	struct ndm_data_agent *		da = &sess->data_acb;
	struct ndmchan *		format_chan = &da->formatter_image;
	struct ndm_data_recover_cb *	reco = &da->recover_cb;


	/* need to let formater channel drain */
	if (ndmchan_n_ready (format_chan) > 0)
		return 0;	/* draining */

	ndmchan_cleanup (format_chan);	/* close */

	ndmda_send_logmsg (sess, "passed=%lld discard=%lld",
				reco->total_passed, reco->total_discarded);
	ndmda_send_logmsg (sess, "skipped=%lld n_read=%d",
				reco->total_skipped, reco->n_data_read);

	if (reco->want.length > 0) {
		ndmda_send_logmsg (sess,
				"halting due to disconnected image stream");

		ndmda_data_halt (sess, NDMP9_DATA_HALT_CONNECT_ERROR);
	} else {
		ndmda_data_halt (sess, NDMP9_DATA_HALT_SUCCESSFUL);
	}

	return 0;
}





int
ndmda_reco_assess_channels (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	struct ndm_data_recover_cb *reco = &da->recover_cb;
	struct ndmchan *	image_chan = &sess->plumb.image_stream.chan;
	struct ndmchan *	format_chan = &da->formatter_image;

	if (da->data_state.operation == NDMP9_DATA_OP_RESTORE) {
		if (format_chan->mode != NDMCHAN_MODE_WRITE) {
			return ndmda_reco_internal_error (sess,
							"format!=WRITE");
		}

		if (format_chan->error) {
			/* could sniff format_chan->saved_errno */
			return ndmda_reco_internal_error (sess,
							"format gone?");
		}
	}

	if (ndmchan_n_ready (image_chan) == 0 && image_chan->eof) {
		reco->reading.length = 0;
	}

	return 0;
}

int
ndmda_reco_assess_intervals (struct ndm_session *sess)
{
	struct ndm_data_agent *		da = &sess->data_acb;
	struct ndm_data_recover_cb *	reco = &da->recover_cb;
	struct ndmchan *		image_chan =
						&sess->plumb.image_stream.chan;
	int				n_ready;
	ndmp9_u_quad			have_end, reading_end;
	ndmp9_u_quad			newly_arrived;


	n_ready = ndmchan_n_ready (image_chan);

	reco->n_ready = n_ready;
	reco->data = &image_chan->data[image_chan->beg_ix];

	if (n_ready > 0) {
		/* reco->have.offset is right */
		reco->have.length = n_ready;

		/*
		 * Check whatever DATA_READ is pending for
		 * the image stream.
		 */
		if (reco->reading.length > 0) {
			/* adjust reco->reading now */
			have_end = reco->have.offset + reco->have.length;
			reading_end = reco->reading.offset
						+ reco->reading.length;

			/*
			 * Sanity checks -- the reco->have interval
			 * should adjoin the reco->reading interval.
			 * Intersection is OK: it means that a portion
			 * of the image stream has arrived that we
			 * requested via DATA_READ.
			 */
			if (have_end < reco->reading.offset) {
				/* disjoint -- below */
				ndmalogf (sess, 0, 7, "have_end=%lld read=%lld",
					have_end, reco->reading.offset);
				return ndmda_reco_internal_error (sess,
					"have_end < reco->reading.offset");
			}
			if (have_end > reading_end) {
				/* disjoint -- above */
				return ndmda_reco_internal_error (sess,
					"have_end > reading_end");
			}

			newly_arrived = have_end - reco->reading.offset;
			reco->reading.offset += newly_arrived;
			reco->reading.length -= newly_arrived;
		}

		reco->expect.offset = reco->have.offset;
		reco->expect.length = reco->have.length + reco->reading.length;
	} else if (reco->reading.length > 0) {
		reco->have.offset = reco->reading.offset;
		reco->have.length = 0;
		reco->expect.offset = reco->reading.offset;
		reco->expect.length = reco->reading.length;
	} else {
		reco->have.length = 0;
		reco->have.offset = NDMP_INVALID_U_QUAD;
		reco->expect = reco->have;
	}


	if (sess->param.log_level >= 7) {
		char		buf[200];
		char *		p = buf;

		sprintf (p, " state=%d", reco->state);
		while (*p) p++;
		sprintf (p, " have=%lld/%lld",
			reco->have.offset, reco->have.length);
		while (*p) p++;
		sprintf (p, " want=%lld/%lld",
			reco->want.offset, reco->want.length);
		while (*p) p++;
		sprintf (p, " read=%lld/%lld",
			reco->reading.offset, reco->reading.length);

		ndmalogf (sess, 0, 7, "RECO: %s", buf);
	}

	return 0;
}

int	/* <0 error, >0 aligned, =0 blocked */
ndmda_reco_align_to_wanted (struct ndm_session *sess)
{
	struct ndm_data_agent *		da = &sess->data_acb;
	struct ndm_data_recover_cb *	reco = &da->recover_cb;
	struct ndmchan *		image_chan =
						&sess->plumb.image_stream.chan;
	ndmp9_u_quad			unwanted;
	int				rc;

  again:
	if (reco->expect.length > 0) {
		/*
		 * We reco->have image stream and/or its on its way
		 * ala reco->reading. Deal with it.
		 */
		if (reco->expect.offset < reco->want.offset) {
			/*
			 * OK. Discard unwanted portion.
			 * It'll align eventually.
			 */
			unwanted = reco->want.offset - reco->have.offset;
			if (unwanted > reco->have.length)
				unwanted = reco->have.length;

			if (unwanted > 0) {
				/*
				 * We can do something. This might
				 * cause alignment. Apply unwanted,
				 * reassess intervals, and
				 * reevaluate.
				 */
				reco->have.offset += unwanted;
				image_chan->beg_ix += unwanted;
				reco->total_skipped += unwanted;
				rc = ndmda_reco_assess_intervals (sess);
				if (rc) return rc;
				goto again;
			}

			if (ndmchan_n_ready(image_chan) > 0) {
				return ndmda_reco_internal_error (sess,
						"expect<want & ready");
			}

			/*
			 * Can't do anything right now.
			 * We don't reco->have image stream
			 * to discard. A DATA_READ is still
			 * in progress so we can't issue another.
			 */
			return 0;	/* blocked */
		}

		if (reco->expect.offset > reco->want.offset) {
			/*
			 * Must discard everything reco->expect'ed.
			 * Then issue a DATA_READ back to
			 * reco->want.offset. If in SEQUENTIAL
			 * mode, this won't work out, so give
			 * up now.
			 */
			if (reco->enable_direct) {
				return ndmda_reco_internal_error (sess,
						"want<expect !direct");
			}
		}

		if (reco->expect.offset != reco->want.offset) {
			/* extra sanity check */
			return ndmda_reco_internal_error (sess,
						"expect!=want s/b");
		}

		/*
		 * reco->expect is aligned with reco->want.
		 */
		return 1;	/* aligned */
	}

	/*
	 * Nothing reco->expect'ed (have+reading).
	 * Implicitly we're aligned.
	 */

	return 1;	/* aligned */
}

int	/* <0 error, >0 aligned, =0 blocked */
ndmda_reco_obtain_wanted (struct ndm_session *sess)
{
	struct ndm_data_agent *		da = &sess->data_acb;
	struct ndm_data_recover_cb *	reco = &da->recover_cb;
	struct ndmchan *		image_chan =
						&sess->plumb.image_stream.chan;
	struct ndm_data_recovery_interval need;
	ndmp9_u_quad			want_end;
	ndmp9_u_quad			have_end;

	/*
	 * Assume:
	 *   1) We're aligned
	 *   2) the intervals are accurate per assess_intervals().
	 *   3) We don't reco->have what we reco->want
	 *
	 * What we reco->want might be reco->reading.
	 */

	if (reco->reading.length > 0) {
		/*
		 * Nothing we can do about it right now
		 */
		return 0;
	}

	if (reco->want.length == 0) {
		return ndmda_reco_internal_error (sess, "obtain want==0");
	}

	want_end = reco->want.offset + reco->want.length;

	if (reco->have.length > 0) {
		have_end = reco->have.offset + reco->have.length;
		if (have_end >= want_end)
			return 0;	/* got it in the buffer*/

		need.offset = have_end;
		need.length = want_end - have_end;
	} else {
		/*
		 * nothing in buffer. So, need to DATA_READ
		 * what we reco->want
		 */
		need = reco->want;
	}

	if (need.length == 0) {
		return ndmda_reco_internal_error (sess, "obtain need==0");
	}

	if (image_chan->eof) {
		reco->state = NDMDA_RECO_STATE_ALL_DONE;
		return -1;	/* this ain't good */
	}

#if 0
	Causes an unpleasant EOF, do not do this
	/*
	 * When in direct mode, the round-trip time for DATA_READ
	 * is fairly substantial. It's nice to skip over big
	 * files though. So, always ask for 10k (arbitrary) at least.
	 * We'll skip through unwanted parts of the image stream.
	 */
	if (need.length < 10*1024)
		need.length = 10*1024;
#endif

	return ndmda_reco_send_data_read (sess, need.offset, need.length);
}

int
ndmda_reco_send_data_read (struct ndm_session *sess,
  unsigned long long offset, unsigned long long length)
{
	struct ndm_data_agent *		da = &sess->data_acb;
	struct ndm_data_recover_cb *	reco = &da->recover_cb;
	ndmp9_addr_type			addr_type;

	ndmalogf (sess, 0, 7, "sending read %lld/%lld", offset, length);

	addr_type = da->data_state.data_connection_addr.addr_type;

	if (NDMP9_ADDR_LOCAL == addr_type) {
#ifndef NDMOS_OPTION_NO_TAPE_AGENT
		if (ndmta_local_mover_read (sess, offset, length) != 0) {
			return ndmda_reco_internal_error (sess,
					"send_read LOCAL failed");
		}
#else /* !NDMOS_OPTION_NO_TAPE_AGENT */
		return ndmda_reco_internal_error (sess,
					"send_read LOCAL not configured");
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */
		goto done;
	}

	switch (addr_type) {
	case NDMP9_ADDR_TCP:
		if (ndma_notify_data_read (sess, offset, length) != 0) {
			return ndmda_reco_internal_error (sess,
					"send_read failed");
		}
		break;

	default:
		return ndmda_reco_internal_error (sess,
					"send_read bogus addr_type");
		break;
	}

  done:
	reco->last_data_read.offset = offset;
	reco->last_data_read.length = length;

	reco->reading = reco->last_data_read;

	reco->n_data_read++;

	return 1;
}

int
ndmda_reco_internal_error (struct ndm_session *sess, char *why)
{
	ndmalogf (sess, 0, 0, "RECOVERY INTERNAL ERROR: %s", why);

	ndmda_data_halt (sess, NDMP9_DATA_HALT_INTERNAL_ERROR);

	return -1;
}



#endif /* !NDMOS_OPTION_NO_DATA_AGENT */
