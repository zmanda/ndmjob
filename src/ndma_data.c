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
 * Ident:    $Id: ndma_data.c,v 1.1.1.1 2003/10/14 19:12:39 ern Exp $
 *
 * Description:
 *
 */


#include "ndmagents.h"

#ifndef NDMOS_OPTION_NO_DATA_AGENT



/*
 * Initialization and Cleanup
 ****************************************************************
 */

/* Initialize -- Set data structure to know value, ignore current value */
int
ndmda_initialize (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;

	NDMOS_MACRO_ZEROFILL (da);
	da->data_state.state = NDMP9_DATA_STATE_IDLE;
	ndmchan_initialize (&da->formatter_error, "dfp-error");
	ndmchan_initialize (&da->formatter_image, "dfp-image");
	ndmda_fh_initialize (sess);

	return 0;
}

/* Commission -- Get agent ready. Entire session has been initialize()d */
int
ndmda_commission (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;

	da->data_state.state = NDMP9_DATA_STATE_IDLE;
	ndmda_fh_commission (sess);

	return 0;
}

/* Decommission -- Discard agent */
int
ndmda_decommission (struct ndm_session *sess)
{
	ndmis_data_close (sess);
	ndmda_purge_environment (sess);
	ndmda_purge_nlist (sess);
	ndmda_fh_decommission (sess);

	ndmda_commission (sess);

	return 0;
}

/* Belay -- Cancel partially issued activation/start */
int
ndmda_belay (struct ndm_session *sess)
{
	ndmda_fh_belay (sess);
	return ndmda_decommission (sess);
}




/*
 * Semantic actions -- called from ndma_dispatch()
 ****************************************************************
 */

ndmp9_error
ndmda_data_start_backup (struct ndm_session *sess)
{
	ndmp9_error	error;

	error = ndmda_gtar_start_backup (sess);
	if (error == NDMP9_NO_ERR)
		ndmis_data_start (sess, NDMCHAN_MODE_WRITE);
	return error;
}

ndmp9_error
ndmda_data_start_recover (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	ndmp9_error		error;

	error = ndmda_gtar_start_recover (sess);

	if (error != NDMP9_NO_ERR)
		return error;

	ndmis_data_start (sess, NDMCHAN_MODE_READ);

	da->recover_cb.state = NDMDA_RECO_STATE_START;

	return NDMP9_NO_ERR;
}

ndmp9_error
ndmda_data_start_recover_fh (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	ndmp9_error		error;

	error = ndmda_gtar_start_recover_fh (sess);
	if (error != NDMP9_NO_ERR)
		return error;

	ndmis_data_start (sess, NDMCHAN_MODE_READ);

	da->recover_cb.state = NDMDA_RECO_STATE_START;

	return NDMP9_NO_ERR;
}

void
ndmda_sync_state (struct ndm_session *sess)
{
	/* no-op, always accurate */
}

void
ndmda_data_abort (struct ndm_session *sess)
{
	ndmda_data_halt (sess, NDMP9_DATA_HALT_ABORTED);
}

void
ndmda_sync_environment (struct ndm_session *sess)
{
	/* no-op, always accurate */
}

ndmp9_error
ndmda_data_listen (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;

	da->data_state.state = NDMP9_DATA_STATE_LISTEN;
	da->data_state.halt_reason = NDMP9_DATA_HALT_NA;

	return NDMP9_NO_ERR;
}

ndmp9_error
ndmda_data_connect (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;

	da->data_state.state = NDMP9_DATA_STATE_CONNECTED;
	da->data_state.halt_reason = NDMP9_DATA_HALT_NA;

	return NDMP9_NO_ERR;
}

void
ndmda_data_halt (struct ndm_session *sess, ndmp9_data_halt_reason reason)
{
	struct ndm_data_agent *	da = &sess->data_acb;

	da->data_state.state = NDMP9_DATA_STATE_HALTED;
	da->data_state.halt_reason = reason;
	da->data_notify_pending = 1;

	ndmda_fh_flush (sess);

	ndmis_data_close (sess);

	ndmchan_cleanup (&da->formatter_image);
	ndmchan_cleanup (&da->formatter_error);

	/* this needs to be better */
	if (da->formatter_pid) {
		sleep (1);	/* give gtar a chance to stop by itself */
		kill (da->formatter_pid, SIGTERM);
	}
}

void
ndmda_data_stop (struct ndm_session *sess)
{
	ndmda_decommission (sess);
}




/*
 * Quantum -- get a bit of work done
 ****************************************************************
 */

int
ndmda_quantum (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	int			did_something = 0;	/* did nothing */

	did_something |= ndmda_quantum_stderr (sess);

	switch (da->data_state.state) {
	default:
		ndmalogf (sess, 0, 0, "BOTCH data state");
		return -1;

	case NDMP9_DATA_STATE_IDLE:
	case NDMP9_DATA_STATE_HALTED:
	case NDMP9_DATA_STATE_CONNECTED:
		break;

	case NDMP9_DATA_STATE_LISTEN:
		switch (sess->plumb.image_stream.data_ep.connect_status) {
		case NDMIS_CONN_LISTEN:		/* no connection yet */
			break;

		case NDMIS_CONN_ACCEPTED:	/* we're in business */
			/* drum roll please... */
			da->data_state.state = NDMP9_DATA_STATE_CONNECTED;
			/* tah-dah */
			did_something++;	/* did something */
			break;

		case NDMIS_CONN_BOTCHED:	/* accept() went south */
		default:			/* ain't suppose to happen */
			ndmda_data_halt (sess, NDMP9_DATA_HALT_CONNECT_ERROR);
			did_something++;	/* did something */
			break;
		}
		break;

	case NDMP9_DATA_STATE_ACTIVE:
		did_something |= ndmda_quantum_stderr (sess);

		switch (da->data_state.operation) {
		case NDMP9_DATA_OP_NOACTION:
			break;

		case NDMP9_DATA_OP_BACKUP:
			did_something |= ndmda_quantum_backup (sess);
			break;

		case NDMP9_DATA_OP_RESTORE:
		case NDMP9_DATA_OP_RESTORE_FILEHIST:
			did_something |= ndmda_quantum_recover_common (sess);
			break;
		}
		break;

	}

	ndmda_send_notice (sess);

	return did_something;
}

int
ndmda_quantum_stderr (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	struct ndmchan *	ch = &da->formatter_error;
	int			did_something = 0;
	char *			p;
	char *			data;
	char *			pend;
	unsigned		n_ready;

  again:
	n_ready = ndmchan_n_ready (ch);
	if (n_ready == 0)
		return did_something;

	data = p = &ch->data[ch->beg_ix];
	pend = p + n_ready;

	while (p < pend && *p != '\n') p++;


	if (p < pend && *p == '\n') {
		*p++ = 0;
		ndmda_send_logmsg (sess, "%s", data);
		ch->beg_ix += p - data;
		did_something++;
		goto again;
	}

	if (!ch->eof)
		return did_something;

	/* content w/o newline, and EOF */
	/* p == pend */
	if (ch->end_ix >= ch->data_size) {
		if (data != ch->data) {
			ndmchan_compress (ch);
			goto again;
		}
		/* that's one huge message */
		p--;	/* lose last byte */
	}

	ch->data[ch->end_ix++] = '\n';
	did_something++;
	goto again;
}

int
ndmda_quantum_backup (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	struct ndmchan *	from_chan = &da->formatter_image;
	struct ndmchan *	to_chan = &sess->plumb.image_stream.chan;
	unsigned		n_ready, n_avail, n_copy;
	int			rc;

  again:
	n_copy = n_ready = ndmchan_n_ready (from_chan);
	if (n_ready == 0) {
		if (from_chan->eof) {
			to_chan->eof = 1;
			if (ndmchan_n_ready (to_chan) == 0) {
				ndmda_data_halt (sess,
					NDMP9_DATA_HALT_SUCCESSFUL);
			}
		}
		return 0;	/* data blocked */
	}

	if (da->enable_hist && da->pass_resid == 0) {
		rc = ndmda_gtar_snoop (sess, from_chan);
		if (rc > 0) {
			ndmchan_compress (from_chan);
			/* need more header data to proceed */
			return 0;	/* data blocked */
		}

		if (rc < 0) {
			/* error. doom. */
			ndmda_send_logmsg (sess, "digest tar hdr failed");
			ndmda_data_halt (sess, NDMP9_DATA_HALT_INTERNAL_ERROR);
			return 0;
		}

		assert (da->pass_resid > 0);
	}

	n_avail = ndmchan_n_avail (to_chan);
	if (n_copy > n_avail)
		n_copy = n_avail;

	if (da->enable_hist) {
		if (n_copy > da->pass_resid)
			n_copy = da->pass_resid;
	}

	if (n_copy > 0) {
		bcopy (&from_chan->data[from_chan->beg_ix],
			&to_chan->data[to_chan->end_ix],
			n_copy);
		from_chan->beg_ix += n_copy;
		to_chan->end_ix += n_copy;
		da->data_state.bytes_processed += n_copy;
		da->pass_resid -= n_copy;
		goto again;	/* do as much as possible */
	}

	return 0;

}

#if 0
int
ndmda_quantum_recover_fh (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	struct ndmchan *	from_chan = &sess->plumb.image_stream.chan;
	unsigned		n_ready, n_copy;
	unsigned long long	still_expecting;
	int			rc;

  again:
	n_copy = n_ready = ndmchan_n_ready (from_chan);
	if (n_ready == 0) {
		if (from_chan->eof) {
			ndmda_data_halt (sess, NDMP9_DATA_HALT_SUCCESSFUL);
		}
		return 0;	/* data blocked */
	}

	assert (n_ready <= da->reco_read_length);

	still_expecting = da->reco_read_length;
	still_expecting -= n_ready;

	switch (da->reco_disposition) {
	default:
		/* error. doom. */
		ndmalogf (sess, 0, 1, "reco_disp bogus");
		ndmda_data_halt (sess, NDMP9_DATA_HALT_INTERNAL_ERROR);
		return 0;

	case AQUIRE:
		assert (da->pass_resid == 0);

		rc = ndmda_gtar_snoop (sess, from_chan);
		if (rc > 0) {
			ndmchan_compress (from_chan);
			/* need more header data to proceed */
			return 0;	/* data blocked */
		}

		if (rc < 0) {
			/* error. doom. */
			ndmalogf (sess, 0, 1, "reco_disp bogus");
			ndmda_data_halt (sess, NDMP9_DATA_HALT_INTERNAL_ERROR);
			return 0;
		}

		if (da->reco_soft_eof) {
			ndmalogf (sess, 0, 3, "soft EOF");
			ndmda_data_halt (sess, NDMP9_DATA_HALT_SUCCESSFUL);
			return 0;
		}

		da->reco_disposition = DISCARD;
		goto again;

	case DISCARD:
		assert (da->pass_resid > 0);

		if (n_copy > da->pass_resid)
			n_copy = da->pass_resid;

		if (n_copy > 0) {
			from_chan->beg_ix += n_copy;
			da->reco_want_offset += n_copy;
			da->reco_read_offset += n_copy;
			da->reco_read_length -= n_copy;
			da->data_state.bytes_processed += n_copy;
			da->pass_resid -= n_copy;

		} else if (still_expecting == 0) {

			/* skip via seek */
			n_copy = da->pass_resid;

			da->reco_want_offset += n_copy;
			da->reco_read_offset += n_copy;
			da->reco_read_length -= n_copy;
			da->pass_resid -= n_copy;
		}

		if (da->pass_resid == 0) {
			da->reco_prev_disposition = DISCARD;
			da->reco_disposition = AQUIRE;
		}
		goto again;
	}


	if (da->enable_hist) {
	}

	return 0;
}


/*
 * Helper functions for recover
 ****************************************************************
 */

void
ndmda_reco_preread (struct ndm_session *sess, unsigned n_bytes)
{

}
#endif




/*
 * Send LOG and NOTIFY messages
 ****************************************************************
 */
void
ndmda_send_logmsg (struct ndm_session *sess, char *fmt, ...)
{
	struct ndmconn *	conn = sess->plumb.control;
	char			buf[4096];
	va_list			ap;

	va_start (ap, fmt);
	vsprintf (buf, fmt, ap);
	va_end (ap);

	ndma_send_logmsg (sess, buf, conn);
}


void
ndmda_send_notice (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;

	if (!da->data_notify_pending)
		return;

	da->data_notify_pending = 0;

	switch (da->data_state.state) {
	case NDMP9_DATA_STATE_HALTED:
		ndma_notify_data_halted (sess);
		break;

	default:
		/* Hmm. Why are we here. Race? */
		break;
	}
}

void
ndmda_send_data_read (struct ndm_session *sess,
  unsigned long long offset, unsigned long long length)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	ndmp9_addr_type		addr_type;

	addr_type = da->data_state.data_connection_addr.addr_type;
#if 0
	da->reco_read_offset = offset;
	da->reco_read_length = length;
#endif

	if (NDMP9_ADDR_LOCAL == addr_type) {
#ifndef NDMOS_OPTION_NO_TAPE_AGENT
		if (ndmta_local_mover_read (sess, offset, length) != 0) {
			ndmda_send_logmsg (sess, "local_mover_read failed");
			ndmda_data_halt (sess, NDMP9_DATA_HALT_INTERNAL_ERROR);
		}
#else /* !NDMOS_OPTION_NO_TAPE_AGENT */
		ndmda_send_logmsg (sess, "local_mover_read not configured");
		ndmda_data_halt (sess, NDMP9_DATA_HALT_INTERNAL_ERROR);
#endif /* !NDMOS_OPTION_NO_TAPE_AGENT */
		return;
	}

	switch (addr_type) {
	case NDMP9_ADDR_TCP:
		ndma_notify_data_read (sess, offset, length);
		break;

	default:
		ndmda_send_logmsg (sess, "bogus mover.addr_type");
		ndmda_data_halt (sess, NDMP9_DATA_HALT_INTERNAL_ERROR);
		break;
	}
}




/*
 * Misc -- env[] and nlist[] subroutines, etc
 ****************************************************************
 */

int
ndmda_copy_environment (struct ndm_session *sess,
  ndmp9_pval *env, unsigned n_env)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	int			i;
	ndmp9_pval *		src_pv;
	ndmp9_pval *		dst_pv;

	for (i = 0; i < n_env; i++) {
		src_pv = &env[i];
		dst_pv = &da->env_tab.env[da->env_tab.n_env];

		dst_pv->name  = NDMOS_API_STRDUP (src_pv->name);
		dst_pv->value = NDMOS_API_STRDUP (src_pv->value);

		if (!dst_pv->name || !dst_pv->value)
			goto fail;

		da->env_tab.n_env++;
	}

	return 0;

  fail:
	for (i = 0; i < da->env_tab.n_env; i++) {
		char *		p;

		dst_pv = &da->env_tab.env[da->env_tab.n_env];

		if ((p = dst_pv->name) != 0)
			NDMOS_API_FREE (p);

		if ((p = dst_pv->value) != 0)
			NDMOS_API_FREE (p);
	}
	da->env_tab.n_env = 0;

	return -1;
}

struct ndmp9_pval *
ndmda_find_env (struct ndm_session *sess, char *name)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	int			i;
	struct ndmp9_pval *	pv;

	for (i = 0; i < da->env_tab.n_env; i++) {
		pv = &da->env_tab.env[i];
		if (strcmp (pv->name, name) == 0)
			return pv;
	}

	return 0;
}


int
ndmda_interpret_boolean_value (char *value_str, int default_value)
{
	if (strcasecmp (value_str, "y") == 0
	 || strcasecmp (value_str, "yes") == 0
	 || strcasecmp (value_str, "t") == 0
	 || strcasecmp (value_str, "true") == 0
	 || strcasecmp (value_str, "1") == 0)
		return 1;

	if (strcasecmp (value_str, "n") == 0
	 || strcasecmp (value_str, "no") == 0
	 || strcasecmp (value_str, "f") == 0
	 || strcasecmp (value_str, "false") == 0
	 || strcasecmp (value_str, "0") == 0)
		return 0;

	return default_value;
}

void
ndmda_purge_environment (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	int			i;
	struct ndmp9_pval *	pv;

	for (i = 0; i < da->env_tab.n_env; i++) {
		pv = &da->env_tab.env[i];

		if (pv->name)  NDMOS_API_FREE (pv->name);
		if (pv->value) NDMOS_API_FREE (pv->value);
		pv->name = 0;
		pv->value = 0;
	}
	da->env_tab.n_env = 0;
}


int
ndmda_copy_nlist (struct ndm_session *sess,
  ndmp9_name *nlist, unsigned n_nlist)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	int			i, j;
	ndmp9_name *		src_nl;
	ndmp9_name *		dst_nl;

	for (i = 0; i < n_nlist; i++) {
		j = da->nlist_tab.n_nlist;
		src_nl = &nlist[i];
		dst_nl = &da->nlist_tab.nlist[j];

		dst_nl->name = NDMOS_API_STRDUP (src_nl->name);
		dst_nl->dest = NDMOS_API_STRDUP (src_nl->dest);
		dst_nl->fh_info = src_nl->fh_info;
		da->nlist_tab.result_err[j] = NDMP9_UNDEFINED_ERR;
		da->nlist_tab.result_count[j] = 0;

		if (!dst_nl->name || !dst_nl->dest)
			return -1;	/* no mem */

		da->nlist_tab.n_nlist++;
	}

	/* sort */

	return 0;
}

void
ndmda_purge_nlist (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	int			i;
	struct ndmp9_name *	nl;

	for (i = 0; i < da->nlist_tab.n_nlist; i++) {
		nl = &da->nlist_tab.nlist[i];

		if (nl->name)  NDMOS_API_FREE (nl->name);
		if (nl->dest)  NDMOS_API_FREE (nl->dest);

		nl->name = 0;
		nl->dest = 0;
	}
	da->nlist_tab.n_nlist = 0;
}

int
ndmda_count_invalid_fh_info (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	int			i, count;
	struct ndmp9_name *	nl;

	count = 0;
	for (i = 0; i < da->nlist_tab.n_nlist; i++) {
		nl = &da->nlist_tab.nlist[i];

		if (nl->fh_info == NDMP_LENGTH_INFINITY)
			count++;
	}

	return count;
}

int
ndmda_count_invalid_fh_info_pending (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	int			i, count;
	struct ndmp9_name *	nl;

	count = 0;
	for (i = 0; i < da->nlist_tab.n_nlist; i++) {
		nl = &da->nlist_tab.nlist[i];

		if (da->nlist_tab.result_err[i] == NDMP9_UNDEFINED_ERR
		 && nl->fh_info == NDMP_LENGTH_INFINITY)
			count++;
	}

	return count;
}

#endif /* !NDMOS_OPTION_NO_DATA_AGENT */
