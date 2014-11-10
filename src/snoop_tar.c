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
 * Ident:    $Id: snoop_tar.c,v 1.1 2004/01/12 18:06:32 ern Exp $
 *
 * Description:
 *
 */


#include "ndmos.h"	/* obtains prototypes */
#include "snoop_tar.h"



static long	from_oct (int digs, char *where);
//static int	strnlen (char *s, int maxlen);
static int	raw_sum (unsigned char *buf, int n);


/*
 * >0 Need this many records to complete file header.
 * 0  Header recognized, Refer to tscb->n_header_records
 *    and tscb->n_content_records.
 * <0 Error.
 */

int
tdh_digest (char *data, unsigned data_len, struct tar_digest_hdr *tdh)
{
	char *			scan = data;
	char *			scan_end = data + data_len;
	int			lf;
	unsigned long		size, size_nrec;
	unsigned		sum;
	struct header *		hdr;

	tdh->n_header_records = 0;
	tdh->n_content_records = 0;
	tdh->name = 0;
	tdh->linkname = 0;
	tdh->is_end_marker = 0;

  top:
	if (scan + RECORDSIZE > scan_end) {
		/* need at least one more record */
		return tdh->n_header_records + 1;
	}

	hdr = (struct header *) scan;
	tdh->n_header_records++;
	sum = raw_sum ((unsigned char*)hdr, RECORDSIZE);

	if (sum == 0) {
		tdh->is_end_marker = 1;
		return 0;
	}

	sum -= raw_sum ((unsigned char *)hdr->chksum, 8);
	sum += ' ' * 8;

	if (sum != from_oct (8, hdr->chksum)) {
		return -1;
	}

	scan += RECORDSIZE;

	size = from_oct (12, hdr->size);
	size_nrec = (size + RECORDSIZE - 1) / RECORDSIZE;
	lf = hdr->linkflag;

	if (lf == LF_LONGNAME || lf == LF_LONGLINK) {
		if (scan + (size_nrec*RECORDSIZE) > scan_end) {
			return tdh->n_header_records + size_nrec;
		}
	}

	if (lf == LF_LONGNAME) {
		if (tdh->name) {
			/* duplicate LF_LONGNAME. Ignore first. */
		}
		tdh->name = scan;
		tdh->name_len = size;

		scan += size_nrec * RECORDSIZE;
		tdh->n_header_records += size_nrec;
		goto top;
	}

	if (hdr->linkflag == LF_LONGLINK) {
		if (tdh->linkname) {
			/* duplicate LF_LONGLINK. Ignore first. */
		}
		tdh->linkname = scan;
		tdh->linkname_len = size;

		scan += size_nrec * RECORDSIZE;
		tdh->n_header_records += size_nrec;
		goto top;
	}

	tdh->primary_header = hdr;

	/* slurp everything up */
	tdh->linkflag = lf;
	tdh->mode  = from_oct (8, hdr->mode);
	tdh->size  = size;
	tdh->uid   = from_oct (8, hdr->uid);
	tdh->gid   = from_oct (8, hdr->gid);
	tdh->mtime = from_oct (12, hdr->mtime);
	tdh->atime = 0;	/* TODO: sometimes available */
	tdh->ctime = 0;	/* TODO: sometimes available */
	if (lf == LF_OLDNORMAL || lf == LF_NORMAL) {
		tdh->n_content_records = size_nrec;
	} else {
		tdh->n_content_records = 0;
	}

	if (!tdh->name) {
		tdh->name = hdr->arch_name;
		tdh->name_len = strnlen (tdh->name, NAMSIZ);
	}
	if (lf == LF_LINK || lf == LF_SYMLINK) {
		if (!tdh->linkname) {
			tdh->linkname = hdr->arch_linkname;
			tdh->linkname_len = strnlen (tdh->linkname, NAMSIZ);
		}
	} else if (tdh->linkname) {
		/* warning: LONGLINK w/o (SYM)LINK */
		tdh->linkname = 0;
	}

	/* ignore trailing slashes in name by reducing name length */
	while (tdh->name_len > 0 && tdh->name[tdh->name_len-1] == '/')
		tdh->name_len--;

	return 0;
}

int
tscb_step (struct tar_snoop_cb *tscb)
{
	int		rc;
	char *		scan_end = tscb->scan + tscb->n_scan;

	while (tscb->scan+RECORDSIZE <= scan_end) {
		if (tscb->passing) {
			unsigned	n_scan_rec = tscb->n_scan / RECORDSIZE;
			int		n_wr;
			unsigned	n_bytes;

			if (n_scan_rec > tscb->passing)
				n_scan_rec = tscb->passing;

			if (tscb->pass_content) {
				int	(*pc)() = tscb->pass_content;

				n_wr = (*pc) (tscb, tscb->scan, n_scan_rec);
			} else {
				n_wr = n_scan_rec;
			}

			if (n_wr <= 0) {
				break;
			}

			n_bytes = n_wr * RECORDSIZE;
			/* write */
			tscb->scan += n_bytes;
			tscb->n_scan -= n_bytes;
			tscb->passing -= n_wr;
			tscb->recno += n_wr;
			continue;
		}

		if (tscb->eof) {
			break;
		}

		bzero (&tscb->tdh, sizeof tscb->tdh);
		rc = tdh_digest (tscb->scan, tscb->n_scan, &tscb->tdh);
		if (rc < 0) {
			return -1;
		}
		if (rc > 0) {
			if (tscb->n_scan == tscb->n_buf) {
				/* no chance of this working out */
				return -2;
			}
			break;
		}

		if (tscb->tdh.is_end_marker) {
			tscb->eof |= TSCB_EOF_SOFT;
			/* note_eof */
		} else {
			if (tscb->notice_file)
				(*tscb->notice_file)(tscb);
		}

		tscb->passing = tscb->tdh.n_header_records;
		tscb->passing += tscb->tdh.n_content_records;
	}

	return 0;
}

int
tscb_fill (struct tar_snoop_cb *tscb, int fd)
{
	int		rc;
	char *		buf_end = tscb->buf + tscb->n_buf;
	char *		scan_end;
	int		n_avail;

	if (tscb->n_scan == 0) {
		/* rewind to maximize I/O size */
		tscb->scan = tscb->buf;
	}

	scan_end = tscb->scan + tscb->n_scan;
	n_avail = buf_end - scan_end;

	if (n_avail == 0) {
		if (tscb->scan == tscb->buf) {
			return 0;	/* can't read more, but no EOF */
		}
		if (tscb->n_scan > 0)
			bcopy (tscb->scan, tscb->buf, tscb->n_scan);

		tscb->scan = tscb->buf;

		scan_end = tscb->scan + tscb->n_scan;
		n_avail = buf_end - scan_end;
	}

	rc = read (fd, scan_end, n_avail);
	if (rc > 0) {
		tscb->n_scan += rc;
	} else if (rc == 0) {
		tscb->eof |= TSCB_EOF_HARD;
	} else {
		/* error */
#if 0
		if (errno == EWOULDBLOCK) {
			rc = 0;
		}
#endif
	}

	return rc;
}


/*
 * from_oct() borrowed from GNU tar. Thanx.
 */
/*
 * Quick and dirty octal conversion.
 *
 * Result is -1 if the field is invalid (all blank, or nonoctal).
 */
#define isodigit(c) ('0' <= (c) && (c) <= '7')

static long
from_oct (int digs, char *where)
{
  register long value;

  while (isspace ((unsigned char) *where))
    {				/* Skip spaces */
      where++;
      if (--digs <= 0)
	return -1;		/* All blank field */
    }
  value = 0;
  while (digs > 0 && isodigit (*where))
    {				/* Scan til nonoctal */
      value = (value << 3) | (*where++ - '0');
      --digs;
    }

  if (digs > 0 && *where && !isspace ((unsigned char) *where))
    return -1;			/* Ended on non-space/nul */

  return value;
}

//static int
//strnlen (char *s, int maxlen) {
//	return strlen(s);
//}

static int
raw_sum (unsigned char *buf, int n) {
	int		sum = 0;

	while (n-- > 0) {
		sum += *buf++;
	}
	return sum;
}









#ifdef SELF_TEST

void
notice_file  (struct tar_snoop_cb *tscb)
{
	fprintf (stderr, "@%-3d %d+%-4d '%c'%04o %4d/%-4d %6d %s\n",
		tscb->recno,
		tscb->tdh.n_header_records,
		tscb->tdh.n_content_records,
		tscb->tdh.linkflag,
		tscb->tdh.mode & 07777,
		tscb->tdh.uid, tscb->tdh.gid,
		tscb->tdh.size,
		tscb->tdh.name);
	if (tscb->tdh.linkname)
		fprintf (stderr, "   link-to %s\n", tscb->tdh.linkname);
}


main () {
	struct tar_snoop_cb	tscb;
	char			buf[64*RECORDSIZE];
	int			rc;

	bzero (&tscb, sizeof tscb);

	tscb.buf = buf;
	tscb.n_buf = sizeof buf;
	tscb.scan = buf;
	tscb.n_scan = 0;

	tscb.notice_file = notice_file;

	for (;;) {
		if (tscb.eof) break;

		rc = tscb_fill (&tscb, 0);
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
		fprintf (stderr, "tar stream ended w/o EOF marker\n");
	}

	return 0;
}

#endif /* SELF_TEST */
