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
 * Ident:    $Id: ndmjob_main.c,v 1.1.1.1 2003/10/14 19:18:15 ern Exp $
 *
 * Description:
 *
 */


#define GLOBAL
#include "ndmjob.h"


int
main (int ac, char *av[])
{
	NDMOS_MACRO_ZEROFILL(&the_session);
	d_debug = -1;

	/* ready the_param early so logging works during process_args() */
	NDMOS_MACRO_ZEROFILL (&the_param);
	the_param.log.deliver = ndmjob_log_deliver;
	the_param.log_level = 0;
	the_param.log_tag = "SESS";

#ifndef NDMOS_OPTION_NO_CONTROL_AGENT
	b_bsize = 20;
	index_fp = stderr;
	o_tape_addr = -1;
	o_from_addr = -1;
	o_to_addr = -1;
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */
	log_fp = stderr;

	process_args (ac, av);

	if (the_param.log_level < d_debug)
		the_param.log_level = d_debug;
	if (the_param.log_level < v_verbose)
		the_param.log_level = v_verbose;
	the_param.config_file_name = o_config_file;

	if (the_mode == 'D') {
		the_session.param = the_param;

		if (n_noop) {
			dump_settings();
			return 0;
		}
		start_log_file ();
		ndma_daemon_session (&the_session);
		return 0;
	}

#ifndef NDMOS_OPTION_NO_CONTROL_AGENT
	build_job();		/* might not return */

	the_session.param = the_param;
	the_session.control_acb.job = the_job;

	if (n_noop) {
		dump_settings();
		return 0;
	}

	start_log_file ();

	start_index_file ();

	ndma_client_session (&the_session);

	ndmjob_log (1, "Complete");
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */

	return 0;
}


int
start_log_file (void)
{
	if (L_log_file) {
		FILE *		lfp;

		lfp = fopen (L_log_file, "w");
		if (!lfp) {
			error_byebye ("can't open -L logfile");
		}
		log_fp = lfp;
	}

	return 0;
}

#ifndef NDMOS_OPTION_NO_CONTROL_AGENT
int
start_index_file (void)
{
	if (I_index_file && strcmp (I_index_file, "-") != 0) {
		FILE *		ifp;

		ifp = fopen (I_index_file, "w");
		if (!ifp) {
			error_byebye ("can't open -I logfile");
		}
		index_fp = ifp;
		fprintf (ifp, "##ndmjob -I\n");
	} else {
		index_fp = log_fp;
	}

	return 0;
}
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */

void
error_byebye (char *fmt, ...)
{
	va_list		ap;
	char		buf[256];

	va_start (ap, fmt);
	vsprintf (buf, fmt, ap);
	va_end (ap);
	ndmjob_log (0, "FATAL: %s", buf);
#if 0
	fprintf (stderr, "%s: %s\n", progname, buf);
#endif
	exit(1);
}

void
ndmjob_log_deliver (struct ndmlog *log, char *tag, int lev, char *msg)
{
	char		tagbuf[32];

	if (the_mode == 'D') {
		char	buf[32];

		sprintf (buf, "%s(%d)", tag, (int)getpid());
		sprintf (tagbuf, "%-11s", buf);
	} else {
		sprintf (tagbuf, "%-4s", tag);
	}

	if (d_debug >= lev) {
		if (!o_no_time_stamps) {
			fprintf (log_fp, "%s %s %s\n",
				tagbuf, ndmlog_time_stamp(), msg);
		} else {
			fprintf (log_fp, "%s t[x] %s\n",
				tagbuf, msg);
		}
		fflush (log_fp);
	}

	if (v_verbose >= lev) {
		printf ("%s\n", msg);
		fflush (stdout);
	}
}

#ifndef NDMOS_OPTION_NO_CONTROL_AGENT
void
ndmjob_ixlog_deliver (struct ndmlog *log, char *tag, int lev, char *msg)
{
	fprintf (index_fp, "%s %s\n", tag, msg);
	fflush (index_fp);
}
#endif /* !NDMOS_OPTION_NO_CONTROL_AGENT */

void
ndmjob_log (int lev, char *fmt, ...)
{
	va_list		ap;
	char		buf[1024];

	va_start (ap, fmt);
	vsprintf (buf, fmt, ap);
	va_end (ap);

	ndmjob_log_deliver (&the_param.log, the_param.log_tag, lev, buf);
}
