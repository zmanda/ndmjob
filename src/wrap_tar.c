#include "ndmos.h"
#include "wraplib.h"
#include "snoop_tar.h"


struct gtar_ccb {
	struct wrap_ccb *	wccb;

	FILE *			gtar_fp;	/* gtar's stdin or stdout */

	int			fdmap[3];
	int			gtar_pid;
	char			cmd[WRAP_MAX_COMMAND];
	char			iobuf[64*RECORDSIZE];
};

int	gtar_backup (struct gtar_ccb *gtar);
int	gtar_backup_cmd (struct gtar_ccb *gtar);
int	gtar_recovfh (struct gtar_ccb *gtar);
void	send_file_history  (struct wrap_ccb *wccb,
		struct tar_digest_hdr *tdh, unsigned long long fhinfo);


int
main (int argc, char *argv[])
{
	struct wrap_ccb		_wccb, *wccb = &_wccb;
	struct gtar_ccb		_gtar, *gtar = &_gtar;
	int			rc;

	NDMOS_MACRO_ZEROFILL (gtar);
	gtar->wccb = wccb;

	rc = wrap_main (argc, argv, wccb);
	if (rc) {
		fprintf (stderr, "Error: %s\n", wccb->errmsg);
		return 1;
	}

	wccb->iobuf = gtar->iobuf;
	wccb->n_iobuf = sizeof gtar->iobuf;

	switch (wccb->op) {
	case WRAP_CCB_OP_BACKUP:
		rc = gtar_backup (gtar);
		break;

	case WRAP_CCB_OP_RECOVER_FILEHIST:
		rc = gtar_recovfh (gtar);
		break;

	default:
		fprintf (stderr, "Op unimplemented\n");
		break;
	}

	return 0;
}

void
send_file_history  (struct wrap_ccb *wccb,
  struct tar_digest_hdr *tdh, unsigned long long fhinfo)
{
	struct wrap_fstat	fstat;

	fstat.valid = 0;
	switch (tdh->linkflag) {
	case LF_OLDNORMAL:	fstat.ftype = WRAP_FTYPE_REG;	break;
	case LF_NORMAL:		fstat.ftype = WRAP_FTYPE_REG;	break;
	case LF_LINK:		fstat.ftype = WRAP_FTYPE_REG;	break;
	case LF_SYMLINK:	fstat.ftype = WRAP_FTYPE_SLINK;	break;
	case LF_CHR:		fstat.ftype = WRAP_FTYPE_CSPEC;	break;
	case LF_BLK:		fstat.ftype = WRAP_FTYPE_BSPEC;	break;
	case LF_DIR:		fstat.ftype = WRAP_FTYPE_DIR;	break;
	case LF_FIFO:		fstat.ftype = WRAP_FTYPE_FIFO;	break;
	case LF_CONTIG:		fstat.ftype = WRAP_FTYPE_REG;	break;
	default:		fstat.ftype = WRAP_FTYPE_OTHER;	break;
	}
	fstat.valid |= WRAP_FSTAT_VALID_FTYPE;

	fstat.mode = tdh->mode & 07777;
	fstat.valid |= WRAP_FSTAT_VALID_MODE;

	fstat.uid = tdh->uid;
	fstat.valid |= WRAP_FSTAT_VALID_UID;

	fstat.gid = tdh->gid;
	fstat.valid |= WRAP_FSTAT_VALID_GID;

	fstat.size = tdh->size;
	fstat.valid |= WRAP_FSTAT_VALID_SIZE;

	if (tdh->atime) {
		fstat.atime = tdh->atime;
		fstat.valid |= WRAP_FSTAT_VALID_ATIME;
	}

	if (tdh->mtime) {
		fstat.mtime = tdh->mtime;
		fstat.valid |= WRAP_FSTAT_VALID_MTIME;
	}

	if (tdh->ctime) {
		fstat.ctime = tdh->ctime;
		fstat.valid |= WRAP_FSTAT_VALID_CTIME;
	}

	wrap_send_add_file (wccb->index_fp, tdh->name, fhinfo, &fstat);
}

void
backup_notice_file  (struct tar_snoop_cb *tscb)
{
	struct gtar_ccb *	gtar = (struct gtar_ccb *)tscb->app_data;
	struct wrap_ccb *	wccb = gtar->wccb;
	unsigned long long	fhinfo;


	fhinfo = tscb->recno;
	fhinfo *= RECORDSIZE;

	send_file_history  (wccb, &tscb->tdh, fhinfo);
}

int
backup_pass_content (struct tar_snoop_cb *tscb, char *data, int n_data_rec)
{
	struct gtar_ccb *	gtar = (struct gtar_ccb *)tscb->app_data;
	struct wrap_ccb *	wccb = gtar->wccb;
	int			rc;

	rc = write (wccb->data_conn_fd, data, n_data_rec * RECORDSIZE);
	return n_data_rec;
}

int
gtar_backup (struct gtar_ccb *gtar)
{
	int			rc;
	struct tar_snoop_cb 	tscb;

	NDMOS_MACRO_ZEROFILL (&tscb);
	tscb.buf = gtar->iobuf;
	tscb.n_buf = sizeof gtar->iobuf;
	tscb.scan = tscb.buf;
	tscb.n_scan = 0;
	tscb.app_data = (void*) gtar;

	tscb.notice_file = backup_notice_file;
	tscb.pass_content = backup_pass_content;

	rc = gtar_backup_cmd (gtar);
	if (rc)
		return rc;

	gtar->fdmap[0] = WRAP_FDMAP_DEV_NULL;
	gtar->fdmap[1] = WRAP_FDMAP_OUTPUT_PIPE;
	gtar->fdmap[2] = 2;

	rc = wrap_pipe_fork_exec (gtar->cmd, gtar->fdmap);
	gtar->gtar_pid = rc;

	wrap_log (gtar->wccb, "fdmap={%d,%d,%d} pid=%d",
			gtar->fdmap[0], gtar->fdmap[1], gtar->fdmap[2],
			gtar->gtar_pid);

	if (rc <= 0)
		return rc;

	if (!gtar->wccb->hist_enable || !gtar->wccb->index_fp)
		tscb.notice_file = 0;

	fflush(stdout);		/* shouldn't be anything there */

	for (;;) {
		if (tscb.eof) break;

		rc = tscb_fill (&tscb, gtar->fdmap[1]);
		if (rc < 0) {
			perror ("tscb_fill()");
			exit(2);
		}

		rc = tscb_step (&tscb);
		if (rc < 0) {
			fprintf (stderr, "tscb_step() error\n");
			exit(3);
		}
	}

	if (! (tscb.eof & TSCB_EOF_SOFT)) {
		wrap_log (gtar->wccb, "tar stream ended w/o EOF marker.");
	}

	return 0;
}

int
gtar_backup_cmd (struct gtar_ccb *gtar)
{
	struct wrap_ccb *	wccb = gtar->wccb;
	char *			cmd = gtar->cmd;
	int			i, n_file;
	char *			val;
	struct wrap_env *	env;

	*cmd = 0;

	strcpy (cmd, "tar -c -f-");

	val = wrap_find_env (wccb, "PREFIX");
	if (val) {
		wrap_cmd_add_with_sh_escapes (cmd, "-C");
		wrap_cmd_add_with_sh_escapes (cmd, val);
	}

	for (i = 0; i < wccb->n_env; i++) {
		env = &wccb->env[i];

		if (strcmp (env->name, "EXCLUDE") != 0)
			continue;

		wrap_cmd_add_with_sh_escapes (cmd, "--exclude");
		wrap_cmd_add_with_sh_escapes (cmd, env->value);
	}

	n_file = 0;
	for (i = 0; i < wccb->n_env; i++) {
		env = &wccb->env[i];

		if (strcmp (env->name, "FILES") != 0)
			continue;

		n_file++;
		wrap_cmd_add_allow_file_wildcards (cmd, env->value);
	}

	if (n_file == 0)
		wrap_cmd_add_allow_file_wildcards (cmd, ".");


	wrap_log (wccb, "CMD: %s", cmd);

	return 0;
}

int
gtar_recovfh (struct gtar_ccb *gtar)
{
	struct wrap_ccb *	wccb = gtar->wccb;
	struct tar_digest_hdr	tdh;
	int			rc;
	unsigned long long	len;

	/* we don't need to spawn tar(1) for this */
	for (;;) {
		rc = tdh_digest (wccb->have, wccb->have_length, &tdh);
		if (rc > 0) {
			rc = wrap_reco_must_have (wccb, rc*RECORDSIZE);
			if (rc)
				break;
			continue;
		}

		if (rc < 0 || tdh.is_end_marker) {
			break;
		}

		send_file_history (wccb, &tdh, wccb->want_offset);

		len = tdh.n_header_records;
		len += tdh.n_content_records;
		len *= RECORDSIZE;

		wccb->want_offset += len;
		wrap_reco_must_have (wccb, 0);	/* aligns */
	}
	return 0;
}


#if 0

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
	da->data_state.operation = NDMP9_DATA_OP_RECOVER;

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
	da->data_state.operation = NDMP9_DATA_OP_RECOVER_FILEHIST;

	return NDMP9_NO_ERR;
}

#endif

