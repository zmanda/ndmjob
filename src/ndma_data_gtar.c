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
 * Ident:    $Id: ndma_data_gtar.c,v 1.1 2003/10/14 19:12:39 ern Exp $
 *
 * Description:
 *
 */


#include "ndmagents.h"
#include "snoop_tar.h"

#ifndef NDMOS_OPTION_NO_DATA_AGENT

void ndmda_gtar_fh_add (struct ndm_session *sess, struct tar_digest_hdr *tdh);


int
ndmda_butype_gtar_config_get_attrs (struct ndm_session *sess,
  u_long *attrs_p, int protocol_version)
{
	switch (protocol_version) {
#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
		*attrs_p = 0;
		break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */

#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
		*attrs_p = NDMP3_BUTYPE_BACKUP_FILE_HISTORY
			 + NDMP3_BUTYPE_BACKUP_FILELIST
			 + NDMP3_BUTYPE_RECOVER_FILELIST
			 + NDMP3_BUTYPE_BACKUP_DIRECT
			 + NDMP3_BUTYPE_RECOVER_DIRECT;
		break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
		*attrs_p = NDMP4_BUTYPE_BACKUP_FILELIST
			 + NDMP4_BUTYPE_RECOVER_FILELIST
			 + NDMP4_BUTYPE_BACKUP_DIRECT
			 + NDMP4_BUTYPE_RECOVER_DIRECT
			 + NDMP4_BUTYPE_BACKUP_FH_FILE;
		break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */

	default:
		return -1;
	}

	return 0;
}

int
ndmda_butype_gtar_config_get_default_env (struct ndm_session *sess,
  ndmp9_pval **env_p, int *n_env_p, int protocol_version)
{
	*env_p = 0;
	*n_env_p = 0;
	return 0;
}

int
ndmda_butype_gtar_attach (struct ndm_session *sess)
{
	return -1;
}



ndmp9_error
ndmda_gtar_start_backup (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	char			cmd[NDMDA_MAX_CMD];
	int			i, n_file;
	struct ndmp9_pval *	pv;

	*cmd = 0;

	ndmda_add_to_cmd (cmd, "tar");
	ndmda_add_to_cmd (cmd, "-c");
	ndmda_add_to_cmd (cmd, "-f-");

	pv = ndmda_find_env (sess, "PREFIX");
	if (pv) {
		ndmda_add_to_cmd (cmd, "-C");
		ndmda_add_to_cmd (cmd, pv->value);
	}

	for (i = 0; i < da->env_tab.n_env; i++) {
		pv = &da->env_tab.env[i];

		if (strcmp (pv->name, "EXCLUDE") != 0)
			continue;

		ndmda_add_to_cmd (cmd, "--exclude");
		ndmda_add_to_cmd (cmd, pv->value);
	}

	n_file = 0;
	for (i = 0; i < da->env_tab.n_env; i++) {
		pv = &da->env_tab.env[i];

		if (strcmp (pv->name, "FILES") != 0)
			continue;

		n_file++;
		ndmda_add_to_cmd_allow_file_wildcards (cmd, pv->value);
	}

	if (n_file == 0)
		ndmda_add_to_cmd (cmd, ".");

	pv = ndmda_find_env (sess, "HIST");
	if (pv) {
		if (ndmda_interpret_boolean_value (pv->value, 0)) {
			da->enable_hist = 1;
			ndmda_send_logmsg (sess, "History Enabled");
		}
	}

#if 0
	if (ndmda_add_to_cmd (cmd, " ") != 0)	/* overflow cmd */
		return NDMP9_ILLEGAL_ARGS_ERR;
#endif

	ndmda_send_logmsg (sess, "CMD: %s", cmd);

	if (ndmda_pipe_fork_exec (sess, cmd, 1) < 0)
		return NDMP9_UNDEFINED_ERR;

	da->data_state.state = NDMP9_DATA_STATE_ACTIVE;
	da->data_state.operation = NDMP9_DATA_OP_BACKUP;

	return NDMP9_NO_ERR;
}

ndmp9_error
ndmda_gtar_prepare_reco_common (struct ndm_session *sess, char *cmd)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	int			i;
	struct ndmp9_pval *	pv;

	pv = ndmda_find_env (sess, "RECOVER_PREFIX");
	if (pv) {
		ndmda_add_to_cmd (cmd, "-C");
		ndmda_add_to_cmd (cmd, pv->value);
	}

	for (i = 0; i < da->env_tab.n_env; i++) {
		pv = &da->env_tab.env[i];

		if (strcmp (pv->name, "RECOVER_EXCLUDE") != 0)
			continue;

		ndmda_add_to_cmd (cmd, "--exclude");
		ndmda_add_to_cmd (cmd, pv->value);
	}

	pv = ndmda_find_env (sess, "RECOVER_HIST");
	if (pv) {
		if (ndmda_interpret_boolean_value (pv->value, 0)) {
			da->enable_hist = 1;
			ndmda_send_logmsg (sess, "History Enabled");
		}
	}

	da->recover_cb.enable_direct = 0;		/* default */
	pv = ndmda_find_env (sess, "RECOVER_DIRECT");
	if (!pv)
		pv = ndmda_find_env (sess, "DIRECT");
	if (pv) {
		if (ndmda_interpret_boolean_value (pv->value, 0)) {
			da->recover_cb.enable_direct = 1;
			ndmda_send_logmsg (sess, "Direct Access Enabled");
		}
	}

	da->recover_cb.fh_info_valid = 1;		/* default */
	pv = ndmda_find_env (sess, "RECOVER_FH_INFO_VALID");
	if (pv) {
		if (ndmda_interpret_boolean_value (pv->value, 1)) {
			da->recover_cb.fh_info_valid = 0;
		}
	}

#if 0
	if (ndmda_add_to_cmd (cmd, " ") != 0)	/* overflow cmd */
		return NDMP9_ILLEGAL_ARGS_ERR;
#endif

	return NDMP9_NO_ERR;
}

ndmp9_error
ndmda_gtar_start_recover (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	char			cmd[NDMDA_MAX_CMD];
	ndmp9_error		err;

	*cmd = 0;

	ndmda_add_to_cmd (cmd, "tar");
	ndmda_add_to_cmd (cmd, "-x");
	ndmda_add_to_cmd (cmd, "-f-");

	err = ndmda_gtar_prepare_reco_common (sess, cmd);
	if (err != NDMP9_NO_ERR)
		return err;

	ndmda_send_logmsg (sess, "CMD: %s", cmd);

	if (ndmda_pipe_fork_exec (sess, cmd, 0) < 0)
		return NDMP9_UNDEFINED_ERR;

	da->data_state.state = NDMP9_DATA_STATE_ACTIVE;
	da->data_state.operation = NDMP9_DATA_OP_RESTORE;

	return NDMP9_NO_ERR;
}

ndmp9_error
ndmda_gtar_start_recover_fh (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	char			cmd[NDMDA_MAX_CMD];	/* not really used */
	ndmp9_error		err;

	*cmd = 0;

	ndmda_add_to_cmd (cmd, "tar");
	ndmda_add_to_cmd (cmd, "-t");
	ndmda_add_to_cmd (cmd, "-f-");

	err = ndmda_gtar_prepare_reco_common (sess, cmd);
	if (err != NDMP9_NO_ERR)
		return err;

	/* no pid necessary */

	if (!da->enable_hist) {
		da->enable_hist = 1;
		ndmda_send_logmsg (sess, "History Enabled");
	}

	da->data_state.state = NDMP9_DATA_STATE_ACTIVE;
	da->data_state.operation = NDMP9_DATA_OP_RESTORE_FILEHIST;

	return NDMP9_NO_ERR;
}

int
ndmda_gtar_snoop (struct ndm_session *sess, struct ndmchan *ch)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	struct ndmchan *	from_chan = ch;
	char *			data = &from_chan->data[from_chan->beg_ix];
	unsigned		n_ready;
	int			rc;
	struct tar_digest_hdr	tdh;

	n_ready = ndmchan_n_ready (from_chan);
	rc = tdh_digest (data, n_ready, &tdh);
	if (rc != 0)
		return rc;	/* error or undeflow */

	/* have complete header */
	da->pass_resid = tdh.n_header_records;
	da->pass_resid += tdh.n_content_records;
	if (!tdh.is_end_marker) {
		if (da->enable_hist)
			ndmda_gtar_fh_add (sess, &tdh);
	} else {
		if (da->enable_hist)
			ndmda_fh_flush (sess);
	}
	da->pass_resid *= RECORDSIZE;

	return 0;
}

void
ndmda_gtar_fh_add (struct ndm_session *sess, struct tar_digest_hdr *tdh)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	char *			name = tdh->name;
	ndmp9_file_stat		filestat;

	NDMOS_MACRO_ZEROFILL (&filestat);

	switch (tdh->linkflag) {
	default:
	case LF_OLDNORMAL:
	case LF_NORMAL:
	case LF_LINK:
	case LF_CONTIG:
		filestat.ftype = NDMP9_FILE_REG;
		break;

	case LF_SYMLINK:
		filestat.ftype = NDMP9_FILE_SLINK;
		break;

	case LF_CHR:
		filestat.ftype = NDMP9_FILE_CSPEC;
		break;

	case LF_BLK:
		filestat.ftype = NDMP9_FILE_BSPEC;
		break;

	case LF_DIR:
		filestat.ftype = NDMP9_FILE_DIR;
		break;

	case LF_FIFO:
		filestat.ftype = NDMP9_FILE_FIFO;
		break;
	}

	filestat.mtime.valid = NDMP9_VALIDITY_VALID;
	filestat.mtime.value = tdh->mtime;
	filestat.atime.valid = NDMP9_VALIDITY_VALID;
	filestat.atime.value = tdh->atime;
	filestat.ctime.valid = NDMP9_VALIDITY_VALID;
	filestat.ctime.value = tdh->ctime;
	filestat.uid.valid = NDMP9_VALIDITY_VALID;
	filestat.uid.value = tdh->uid;
	filestat.gid.valid = NDMP9_VALIDITY_VALID;
	filestat.gid.value = tdh->gid;
	filestat.mode.valid = NDMP9_VALIDITY_VALID;
	filestat.mode.value = tdh->mode;
	filestat.size.valid = NDMP9_VALIDITY_VALID;
	filestat.size.value = tdh->size;
	filestat.fh_info.valid = NDMP9_VALIDITY_VALID;
	filestat.fh_info.value = da->data_state.bytes_processed;

	ndmda_fh_add_unix_path (sess, &filestat, name);
}

#endif /* !NDMOS_OPTION_NO_DATA_AGENT */
