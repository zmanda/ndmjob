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
 * Ident:    $Id: ndmjob_job.c,v 1.1 2003/10/14 19:12:40 ern Exp $
 *
 * Description:
 *
 */


#include "ndmjob.h"


#ifndef NDMOS_OPTION_NO_CONTROL_AGENT


int
build_job (void)
{
	struct ndm_job_param *	job = &the_job;
	int			i, rc, n_err;
	char			errbuf[100];

	NDMOS_MACRO_ZEROFILL(job);

	args_to_job ();

	ndma_job_auto_adjust (job);

	if (o_rules)
		apply_rules (job, o_rules);

	i = n_err = 0;
	do {
		rc = ndma_job_audit (job, errbuf, i);
		if (rc > n_err || rc < 0) {
			ndmjob_log (0, "error: %s", errbuf);
		}
		n_err = rc;
	} while (i++ < n_err);

	if (n_err) {
		error_byebye ("can't proceed");
		/* no return */
	}

	return 0;
}


int
args_to_job (void)
{
	struct ndm_job_param *	job = &the_job;
	int			i;

	switch (the_mode) {
	case NDM_JOB_OP_QUERY_AGENTS:
	case NDM_JOB_OP_INIT_LABELS:
	case NDM_JOB_OP_LIST_LABELS:
	case NDM_JOB_OP_REMEDY_ROBOT:
	case NDM_JOB_OP_TEST_TAPE:
	case NDM_JOB_OP_TEST_MOVER:
	case NDM_JOB_OP_TEST_DATA:
	case NDM_JOB_OP_REWIND_TAPE:
	case NDM_JOB_OP_EJECT_TAPE:
	case NDM_JOB_OP_MOVE_TAPE:
	case NDM_JOB_OP_IMPORT_TAPE:
	case NDM_JOB_OP_EXPORT_TAPE:
	case NDM_JOB_OP_LOAD_TAPE:
	case NDM_JOB_OP_UNLOAD_TAPE:
	case NDM_JOB_OP_INIT_ELEM_STATUS:
		break;

	case NDM_JOB_OP_BACKUP:
		args_to_job_backup_env();
		break;

	case NDM_JOB_OP_TOC:
		if (J_index_file)
			jndex_doit();
		args_to_job_recover_env();
		args_to_job_recover_nlist();
		break;

	case NDM_JOB_OP_EXTRACT:
		jndex_doit();
		args_to_job_recover_env();
		args_to_job_recover_nlist();
		break;

	case 'D':		/* -o daemon */
		return 0;

	default:
		printf ("mode -%c not implemented yet\n", the_mode);
		break;
	}
	job->operation = the_mode;

	/* DATA agent */
	job->data_agent  = D_data_agent;
	job->bu_type = B_bu_type;
	for (i = 0; i < nn_E_environment; i++)
		job->env_tab.env[i] = E_environment[i];
	job->env_tab.n_env = nn_E_environment;
	if (the_mode == NDM_JOB_OP_EXTRACT || the_mode == NDM_JOB_OP_TOC) {
		for (i = 0; i < n_file_arg; i++)
			job->nlist_tab.nlist[i] = nlist[i];
		job->nlist_tab.n_nlist = n_file_arg;
	}
	job->index_log.deliver = ndmjob_ixlog_deliver;

	/* TAPE agent */
	job->tape_agent  = T_tape_agent;
	job->tape_device = f_tape_device;
	job->record_size = b_bsize * 512;
	job->tape_timeout = o_tape_timeout;
	job->use_eject = o_use_eject;
	job->tape_target = o_tape_scsi;

	/* ROBOT agent */
	job->robot_agent = R_robot_agent;
	job->robot_target = r_robot_target;
	job->robot_timeout = o_robot_timeout;
	if (o_tape_addr >= 0) {
		job->drive_addr = o_tape_addr;
		job->drive_addr_given = 1;
	}
	if (o_from_addr >= 0) {
		job->from_addr = o_from_addr;
		job->from_addr_given = 1;
	}
	if (o_to_addr >= 0) {
		job->to_addr = o_to_addr;
		job->to_addr_given = 1;
	}
	if (ROBOT_GIVEN())
		job->have_robot = 1;

	/* media */
	for (i = 0; i < n_m_media; i++)
		job->media_tab.media[i] = m_media[i];
	job->media_tab.n_media = n_m_media;

	return 0;
}


int
args_to_job_backup_env (void)
{
 	int		n_env = n_E_environment;
	int		i;

	if (C_chdir) {
		E_environment[n_env].name = "PREFIX";
		E_environment[n_env].value = C_chdir;
		n_env++;
	}

	E_environment[n_env].name = "HIST";
	E_environment[n_env].value = I_index_file ? "Yes" : "No";
	n_env++;

	E_environment[n_env].name = "TYPE";
	E_environment[n_env].value = B_bu_type;
	n_env++;

	if (U_user) {
		E_environment[n_env].name = "USER";
		E_environment[n_env].value = U_user;
		n_env++;
	}

	for (i = 0; i < n_e_exclude_pattern; i++) {
		E_environment[n_env].name = "EXCLUDE";
		E_environment[n_env].value = e_exclude_pattern[i];
		n_env++;
	}
	for (i = 0; i < n_file_arg; i++) {
		E_environment[n_env].name = "FILES";
		E_environment[n_env].value = file_arg[i];
		n_env++;
	}

	if (o_rules) {
		E_environment[n_env].name = "RULES";
		E_environment[n_env].value = o_rules;
	}

	nn_E_environment = n_env;

	return n_env;
}

int
args_to_job_recover_env (void)
{
 	int		n_env = n_E_environment;
	int		i;

	if (C_chdir) {
		E_environment[n_env].name = "RECOVER_PREFIX";
		E_environment[n_env].value = C_chdir;
		n_env++;
	}

	E_environment[n_env].name = "RECOVER_HIST";
	E_environment[n_env].value = I_index_file ? "Yes" : "No";
	n_env++;

	E_environment[n_env].name = "RECOVER_TYPE";
	E_environment[n_env].value = B_bu_type;
	n_env++;

	if (U_user) {
		E_environment[n_env].name = "RECOVER_USER";
		E_environment[n_env].value = U_user;
		n_env++;
	}

	for (i = 0; i < n_e_exclude_pattern; i++) {
		E_environment[n_env].name = "RECOVER_EXCLUDE";
		E_environment[n_env].value = e_exclude_pattern[i];
		n_env++;
	}

	if (o_rules) {
		E_environment[n_env].name = "RECOVER_RULES";
		E_environment[n_env].value = o_rules;
	}

	nn_E_environment = n_env;

	/* file_arg[]s are done in nlist[] */

	jndex_merge_environment ();

	return nn_E_environment;
}

int
args_to_job_recover_nlist (void)
{
	int		not_found = 0;
	int		i, prefix_len, len;
	char *		dest;

	if (C_chdir) {
		prefix_len = strlen (C_chdir) + 2;
	} else {
		prefix_len = 0;
	}

	for (i = 0; i < n_file_arg; i++) {
		len = strlen (file_arg[i]) + prefix_len;

		dest = NDMOS_API_MALLOC (len);
		*dest = 0;
		if (C_chdir) {
			strcpy (dest, C_chdir);
			strcat (dest, "/");
		}
		strcat (dest, file_arg[i]);

		nlist[i].name = file_arg[i];
		nlist[i].dest = dest;
		if (ji_fh_info_found[i]) {
			nlist[i].fh_info = ji_fh_info[i];
		} else {
			not_found++;
			nlist[i].fh_info = NDMP_LENGTH_INFINITY;
		}
	}

	return not_found;	/* should ALWAYS be 0 */
}


/*
 * Index files are sequentially searched. They can be VERY big.
 * There is a credible effort for efficiency here.
 * Probably lots and lots and lots of room for improvement.
 */


int
jndex_doit (void)
{
	if (!J_index_file) {
		/* Hmmm. */
		ndmjob_log (1, "Warning: No -J input index?");	/* no return */
		return 0;
	}

	ndmjob_log (1, "Processing input index (-J%s)", J_index_file);

	jndex_calc_file_arg_hash_keys ();
	jndex_scan_input_index ();
	jndex_tattle();

	if (jndex_audit_not_found ()) {
		ndmjob_log (1,
			"Warning: Missing index entries, valid file name(s)?");
	}

 	jndex_merge_media ();

	return 0;
}

int
jndex_tattle (void)
{
	char		buf[100];
	int		i;

	for (i = 0; i < n_ji_media; i++) {
		struct ndmmedia *	me = &ji_media[i];

		ndmmedia_to_str (me, buf);
		ndmjob_log (3, "ji me[%d] %s", i, buf);
	}

	for (i = 0; i < n_ji_environment; i++) {
		ndmp9_pval *		pv = &ji_environment[i];

		ndmjob_log (3, "ji env[%d] %s=%s", i, pv->name, pv->value);
	}

	for (i = 0; i < n_file_arg; i++) {
		if (ji_fh_info_found[i]) {
			ndmjob_log (3, "ji fil[%d] fi=%lld %s",
				i, ji_fh_info[i], file_arg[i]);
		} else {
			ndmjob_log (3, "ji fil[%d] not-found %s",
				i, file_arg[i]);
		}
	}

	return 0;
}

int
jndex_merge_media (void)
{
	struct ndmmedia *	me;
	struct ndmmedia *	jme;
	int			i, j;

	for (j = 0; j < n_ji_media; j++) {
		jme = &ji_media[j];

		if (! jme->valid_label)
			continue;	/* can't match it up */

		for (i = 0; i < n_m_media; i++) {
			me = &m_media[i];

			if (! me->valid_label)
				continue;	/* can't match it up */

			if (strcmp (jme->label, me->label) != 0)
				continue;

			if (!jme->valid_slot &&  me->valid_slot) {
				jme->slot_addr = me->slot_addr;
				jme->valid_slot = 1;
			}
		}
	}

	for (i = 0; i < n_ji_media; i++) {
		m_media[i] = ji_media[i];
	}
	n_m_media = i;

	ndmjob_log (3, "After merging input -J index with -m entries");
	for (i = 0; i < n_m_media; i++) {
		char		buf[40];

		me = &m_media[i];
		ndmmedia_to_str (me, buf);
		ndmjob_log (3, "%d: -m %s", i+1, buf);
	}

	return 0;
}

int
jndex_audit_not_found (void)
{
	int		i;
	int		not_found = 0;

	for (i = 0; i < n_file_arg; i++) {
		if (!ji_fh_info_found[i]) {
			ndmjob_log (0, "No index entry for %s", file_arg[i]);
			not_found++;
			ji_fh_info[i] = NDMP_LENGTH_INFINITY;
		}
	}

	return not_found;
}

int
jndex_merge_environment (void)
{
	int		i;

	for (i = 0; i < n_ji_environment; i++) {
		E_environment[nn_E_environment++] = ji_environment[i];
	}

	return 0;
}

int
jndex_calc_file_arg_hash_keys (void)
{
	int		i;

	for (i = 0; i < n_file_arg; i++) {
		ji_hash_key[i] = j_hash_key (file_arg[i]);
	}

	return 0;
}

int
jndex_scan_input_index (void)
{
	FILE *		fp;
	char		buf[256];
	char *		atsign;
	char *		p;
	char *		q;
	int		c, i;

	fp = fopen(J_index_file, "r");
	if (!fp) {
		perror (J_index_file);
		error_byebye ("Can not open -J%s input index", J_index_file);
	}

	if (fgets (buf, sizeof buf, fp) == NULL) {
		fclose (fp);
		error_byebye ("Failed read first line of -J%s", J_index_file);
	}

	if (strcmp (buf, "##ndmjob -I\n") != 0) {
		ndmjob_log (1, "Warning: wrong header in -J%s", J_index_file);
		fseek (fp, (off_t)0, 0);
	}

    for (;;) {
	p = buf;
	atsign = 0;
	while ((c = getc(fp)) != EOF && c != '\n') {
		if (c == '@' && !atsign)
			atsign = p;
		if (p < &buf[255])
			*p++ = c;
	}
	*p = 0;
	if (p == buf && c == EOF)
		break;

	/* DIp d0755 211 100 ------ 3621c15a @1024 foo/bar/ */
	if (buf[0] == 'D'
	 && buf[1] == 'H'
	 && buf[2] == 'f'
	 && buf[3] == ' ') {
		unsigned long		hash_key;

		if (!atsign) {
			goto malformed;
		}

		p = atsign;
		while (*p && *p != ' ') p++;
		if (*p != ' ' || p[1] == 0) {
			goto malformed;
		}
		p++;

		hash_key = j_hash_key (p);
		for (i = 0; i < n_file_arg; i++) {
			if (ji_fh_info_found[i])
				continue;	/* already found (use first) */

			if (ji_hash_key[i] != hash_key)
				continue;	/* won't match */

			/* possible match */
			if (j_match (file_arg[i], p)) {
				/* yee-ha */
				ji_fh_info_found[i] = 1;
				ji_fh_info[i] =
					NDMOS_API_STRTOLL (atsign+1, 0, 0);
				ndmjob_log (2, "JMatch arg='%s' ind='%s'",
					file_arg[i], buf);
			}
		}

		continue;
	}

	/* CM 1 T103/10850K */
	if (buf[0] == 'C'
	 && buf[1] == 'M'
	 && buf[2] == ' ') {
		struct ndmmedia *	me;

		if (n_ji_media >= NDM_MAX_MEDIA) {
			goto overflow;
		}

		me = &ji_media[n_ji_media];
		if (ndmmedia_from_str (me, &buf[5])) {
			goto malformed;
		}
		n_ji_media++;

		continue;
	}

	/* DE HIST=Yes */
	if (buf[0] == 'D'
	 && buf[1] == 'E'
	 && buf[2] == ' ') {
		if (n_ji_environment >= NDM_MAX_ENV) {
			goto overflow;
		}

		p = &buf[2];
		while (*p == ' ') p++;

		if (!strchr (p, '=')) {
			goto malformed;
		}

		p = NDMOS_API_STRDUP (p);
		q = strchr (p, '=');
		*q++ = 0;

		ji_environment[n_ji_environment].name = p;
		ji_environment[n_ji_environment].value = q;

		n_ji_environment++;

		continue;
	}

	/* unknown or irrelevant line type */
	continue;

  malformed:
	ndmjob_log (1, "Malformed in -J%s: %s", J_index_file, buf);
	continue;

  overflow:
	ndmjob_log (1, "Overflow in -J%s: %s", J_index_file, buf);
    }

	fclose (fp);

	return 0;
}


unsigned long
j_hash_key (char *str)
{
	unsigned long		hash_key;
	char *			p = str;
	int			c;
	int			hfact = 5001;

	hash_key = 0;
	while ((c = *p++) != 0) {
		if (c == '/')
			continue;	/* ignore these for now (see below) */

		c &= 0xDF;		/* case insensitive hash */
		hash_key = hash_key * hfact + c;
	}

	return hash_key;
}

int
j_match (char *name1, char *name2)
{
	char *		p1 = name1;
	char *		p2 = name2;

	for (;;) {
		/* ignore leading forward slashes */
		while (*p1 == '/') p1++;
		while (*p2 == '/') p2++;

		/* Scan exactly matching portion quickly. */
		while (*p1 && *p1 != '/' && *p1 == *p2) p1++, p2++;

		if (*p1 == 0 && *p2 == 0) {
			/* It matches */
			return 1;
		}

		if (*p1 == '/' && *p2 == '/')
			continue;	/* to ignore leading */

		if (*p1 == '/' && *p2 == 0)
			continue;	/* to ignore leading */

		if (*p1 == 0 && *p2 == '/')
			continue;	/* to ignore leading */

		/* no match */
		return 0;
	}

}
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */
