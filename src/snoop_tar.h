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
 * Ident:    $Id: snoop_tar.h,v 1.1 2003/10/14 19:12:47 ern Exp $
 *
 * Description:
 *
 */


#include "tarhdr_gnu.h"

struct tar_digest_hdr {
	char *		name;
	int		name_len;
	char *		linkname;
	int		linkname_len;

	/* looks a lot like struct stat, huh */
	char		linkflag;	/* ftype */
	unsigned	mode;
	unsigned long	uid, gid;
	unsigned long	size;
	unsigned long	mtime, atime, ctime;
	unsigned	dev_maj, dev_min;

	struct header *	primary_header;

	unsigned	n_header_records;
	unsigned long	n_content_records;
	int		is_end_marker;
};

struct tar_snoop_cb {
	char *			buf;
	unsigned		n_buf;
	char *			scan;
	unsigned		n_scan;

	char			eof;
#define TSCB_EOF_HARD (1<<0)
#define TSCB_EOF_SOFT (1<<1)

	unsigned long		recno;
	unsigned long		passing;

	struct tar_digest_hdr	tdh;

	void *			app_data;
	void			(*notice_file)(struct tar_snoop_cb *tscb);
	int			(*pass_content)(struct tar_snoop_cb *tscb,
						char *data, int n_data_rec);
};


extern int	tdh_digest (char *data, unsigned data_len,
					struct tar_digest_hdr *tdh);
extern int	tscb_step (struct tar_snoop_cb *tscb);
extern int	tscb_fill (struct tar_snoop_cb *tscb, int fd);

