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
 * Ident:    $Id: ndma_data_fh.c,v 1.1.1.1 2003/10/14 19:12:39 ern Exp $
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
ndmda_fh_initialize (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	struct ndmfhheap *	fhh = &da->fhh;

	ndmfhh_initialize (fhh);

	return 0;
}

/* Commission -- Get agent ready. Entire session has been initialize()d */
int
ndmda_fh_commission (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	struct ndmfhheap *	fhh = &da->fhh;

	ndmfhh_commission (fhh, &da->fhh_buf, sizeof da->fhh_buf);

	return 0;
}

/* Decommission -- Discard agent */
int
ndmda_fh_decommission (struct ndm_session *sess)
{
	return 0;
}

/* Belay -- Cancel partially issued activation/start */
int
ndmda_fh_belay (struct ndm_session *sess)
{
	return 0;
}




/*
 * Semantic actions -- called from ndmda_XXX() butype formatters
 ****************************************************************
 */

void
ndmda_fh_add_unix_path (struct ndm_session *sess,
  ndmp9_file_stat *filestat, char *name)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	int			nlen = strlen (name) + 1;
	int			rc;

	switch (da->protocol_version) {
	default:	/* impossible */
	    break;

#ifndef NDMOS_OPTION_NO_NDMP2
	case NDMP2VER:
	    {
		ndmp2_fh_unix_path *		nfup;

		rc = ndmda_fh_prepare (sess,
			NDMP2VER, NDMP2_FH_ADD_UNIX_PATH,
			sizeof (ndmp2_fh_unix_path),
			1, nlen);

		if (rc != NDMFHH_RET_OK)
			return;

		nfup = ndmfhh_add_entry (&da->fhh);
		ndmp_9to2_unix_file_stat (filestat, &nfup->fstat);
		nfup->name = ndmfhh_save_item (&da->fhh, name, nlen);
	    }
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP2 */
#ifndef NDMOS_OPTION_NO_NDMP3
	case NDMP3VER:
	    {
		ndmp3_file *		nf;
		ndmp3_file_name *	fname;
		ndmp3_file_stat *	fstat;

		rc = ndmda_fh_prepare (sess,
			NDMP3VER, NDMP3_FH_ADD_FILE,
			sizeof (ndmp3_file),
			3,				/*    3 */
			sizeof (ndmp3_file_stat)	/* 1of3 */
			+ sizeof (ndmp3_file_name)	/* 2of3 */
			+ nlen);			/* 3of3 */

		if (rc != NDMFHH_RET_OK)
			return;

		fstat = ndmfhh_add_item (&da->fhh, sizeof *fstat); /* 1of3 */
		ndmp_9to3_file_stat (filestat, fstat);

		fname = ndmfhh_add_item (&da->fhh, sizeof *fname); /* 2of3 */
		fname->fs_type = NDMP3_FS_UNIX;
		fname->ndmp3_file_name_u.unix_name =
			ndmfhh_save_item (&da->fhh, name, nlen);   /* 3of3 */

		nf = ndmfhh_add_entry (&da->fhh);
		nf->names.names_len = 1;
		nf->names.names_val = fname;
		nf->stats.stats_len = 1;
		nf->stats.stats_val = fstat;
		nf->node = filestat->node.value;
		nf->fh_info = filestat->fh_info.value;
	    }
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP3 */
#ifndef NDMOS_OPTION_NO_NDMP4
	case NDMP4VER:
	    {
		ndmp4_file *		nf;
		ndmp4_file_name *	fname;
		ndmp4_file_stat *	fstat;

		rc = ndmda_fh_prepare (sess,
			NDMP4VER, NDMP4_FH_ADD_FILE,
			sizeof (ndmp4_file),
			3,				/*    3 */
			sizeof (ndmp4_file_stat)	/* 1of3 */
			+ sizeof (ndmp4_file_name)	/* 2of3 */
			+ nlen);			/* 3of3 */

		if (rc != NDMFHH_RET_OK)
			return;

		fstat = ndmfhh_add_item (&da->fhh, sizeof *fstat); /* 1of3 */
		ndmp_9to4_file_stat (filestat, fstat);

		fname = ndmfhh_add_item (&da->fhh, sizeof *fname); /* 2of3 */
		fname->fs_type = NDMP4_FS_UNIX;
		fname->ndmp4_file_name_u.unix_name =
			ndmfhh_save_item (&da->fhh, name, nlen);   /* 3of3 */

		nf = ndmfhh_add_entry (&da->fhh);
		nf->names.names_len = 1;
		nf->names.names_val = fname;
		nf->stats.stats_len = 1;
		nf->stats.stats_val = fstat;
		nf->node = filestat->node.value;
		nf->fh_info = filestat->fh_info.value;
	    }
	    break;
#endif /* !NDMOS_OPTION_NO_NDMP4 */
	}
}

/* TODO: ndmda_fh_add_unix_node () -- much like ndmda_fh_add_unix_path() */
/* TODO: ndmda_fh_add_unix_dir () -- much like ndmda_fh_add_unix_path() */




/*
 * Helpers -- prepare/flush
 ****************************************************************
 */

int
ndmda_fh_prepare (struct ndm_session *sess,
  int vers, int msg, int entry_size,
  unsigned n_item, unsigned total_size_of_items)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	struct ndmfhheap *	fhh = &da->fhh;
	int			fhtype = (vers<<16) + msg;
	int			rc;

	rc = ndmfhh_prepare (fhh, fhtype, entry_size,
				n_item, total_size_of_items);

	if (rc == NDMFHH_RET_OK)
		return NDMFHH_RET_OK;

	ndmda_fh_flush (sess);

	rc = ndmfhh_prepare (fhh, fhtype, entry_size,
				n_item, total_size_of_items);

	return rc;
}

void
ndmda_fh_flush (struct ndm_session *sess)
{
	struct ndm_data_agent *	da = &sess->data_acb;
	struct ndmfhheap *	fhh = &da->fhh;
	int			rc;
	int			fhtype;
	void *			table;
	unsigned		n_entry;

	rc = ndmfhh_get_table (fhh, &fhtype, &table, &n_entry);
	if (rc == NDMFHH_RET_OK && n_entry > 0) {
		struct ndmp_xa_buf	xa;
		struct ndmfhh_generic_table *request;

		request = (void *) &xa.request.body;
		NDMOS_MACRO_ZEROFILL (&xa);

		xa.request.protocol_version = fhtype >> 16;
		xa.request.header.message = fhtype & 0xFFFF;

		request->table_len = n_entry;
		request->table_val = table;

		ndma_send_to_control (sess, &xa, sess->plumb.data);
	}

	ndmfhh_reset (fhh);
}

#endif /* !NDMOS_OPTION_NO_DATA_AGENT */
