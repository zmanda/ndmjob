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
 * Ident:    $Id: ndma_comm_subr.c,v 1.1.1.1 2003/10/14 19:16:38 ern Exp $
 *
 * Description:
 *
 */


#include "ndmagents.h"

void
ndmalogf (struct ndm_session *sess, char *tag, int level, char *fmt, ...)
{
	va_list		ap;

	if (sess->param.log_level < level)
		return;

	if (!tag) tag = sess->param.log_tag;
	if (!tag) tag = "???";

	va_start (ap, fmt);
	ndmlogfv (&sess->param.log, tag, level, fmt, ap);
	va_end (ap);
}


void
ndmalogfv (struct ndm_session *sess, char *tag,
  int level, char *fmt, va_list ap)
{
	if (sess->param.log_level < level)
		return;

	if (!tag) tag = sess->param.log_tag;
	if (!tag) tag = "???";

	ndmlogfv (&sess->param.log, tag, level, fmt, ap);
}


#ifndef NDMOS_OPTION_NO_NDMP2
char *
ndma_log_dbg_tag (ndmp2_debug_level lev)
{
	switch (lev) {
	case NDMP2_DBG_USER_INFO:	return "ui";
	case NDMP2_DBG_USER_SUMMARY:	return "us";
	case NDMP2_DBG_USER_DETAIL:	return "ud";
	case NDMP2_DBG_DIAG_INFO:	return "di";
	case NDMP2_DBG_DIAG_SUMMARY:	return "ds";
	case NDMP2_DBG_DIAG_DETAIL:	return "dd";
	case NDMP2_DBG_PROG_INFO:	return "pi";
	case NDMP2_DBG_PROG_SUMMARY:	return "ps";
	case NDMP2_DBG_PROG_DETAIL:	return "pd";
	default:			return "??";
	}
}
#endif /* !NDMOS_OPTION_NO_NDMP2 */


#ifndef NDMOS_OPTION_NO_NDMP3
char *
ndma_log_message_tag_v3 (ndmp3_log_type log_type)
{
	switch (log_type) {
	case NDMP3_LOG_NORMAL:		return "n";
	case NDMP3_LOG_DEBUG:		return "d";
	case NDMP3_LOG_ERROR:		return "e";
	case NDMP3_LOG_WARNING:		return "w";
	default:			return "?";
	}
}
#endif /* !NDMOS_OPTION_NO_NDMP3 */

#ifndef NDMOS_OPTION_NO_NDMP4
char *
ndma_log_message_tag_v4 (ndmp4_log_type log_type)
{
	switch (log_type) {
	case NDMP4_LOG_NORMAL:		return "n";
	case NDMP4_LOG_DEBUG:		return "d";
	case NDMP4_LOG_ERROR:		return "e";
	case NDMP4_LOG_WARNING:		return "w";
	default:			return "?";
	}
}
#endif /* !NDMOS_OPTION_NO_NDMP4 */


char *
ndma_file_stat_to_str (ndmp9_file_stat *fstat, char *buf)
{
	char *			p = buf;

	switch (fstat->ftype) {
	case NDMP9_FILE_DIR:	*p++ = 'd';	break;
	case NDMP9_FILE_FIFO:	*p++ = 'p';	break;
	case NDMP9_FILE_CSPEC:	*p++ = 'c';	break;
	case NDMP9_FILE_BSPEC:	*p++ = 'b';	break;
	case NDMP9_FILE_REG:	*p++ = '-';	break;
	case NDMP9_FILE_SLINK:	*p++ = 'l';	break;
	case NDMP9_FILE_SOCK:	*p++ = 's';	break;
	case NDMP9_FILE_REGISTRY: *p++ = 'R';	break;
	case NDMP9_FILE_OTHER:	*p++ = 'o';	break;
	default:		*p++ = '?';	break;
	}

	if (fstat->mode.valid) {
		sprintf (p, "%04lo", fstat->mode.value & 07777);
	} else {
		strcpy (p, "----");
	}
	while (*p) p++;

	if (fstat->uid.valid) {
		sprintf (p, " %5ld", fstat->uid.value);
	} else {
		strcpy (p, " -uid-");
	}
	while (*p) p++;

	if (fstat->gid.valid) {
		sprintf (p, " %5ld", fstat->gid.value);
	} else {
		strcpy (p, " -gid-");
	}
	while (*p) p++;


	if (fstat->ftype == NDMP9_FILE_REG
	 || fstat->ftype == NDMP9_FILE_SLINK) {
		if (fstat->size.valid) {
			sprintf (p, " %6lld", fstat->size.value);
		} else {
			strcpy (p, " -size-");
		}
	} else {
		/* ignore size on other file types */
		strcpy (p, " ------");
	}
	while (*p) p++;

#if 0
	/* tar -t can not recover atime/ctime */
	/* they are also not particularly interesting in the index */
	sprintf (p, " %lx %lx %lx", fstat->mtime, fstat->atime, fstat->ctime);
#else
	if (fstat->mtime.valid) {
		sprintf (p, " %lx", fstat->mtime.value);
	} else {
		strcpy (p, " -mtime-");
	}
#endif
	while (*p) p++;

	if (fstat->fh_info.valid) {
		sprintf (p, " @%lld", fstat->fh_info.value);
	} else {
		strcpy (p, " @?fhinfo?");
	}

	return buf;
}
