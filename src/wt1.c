#include "ndmos.h"
#include "wraplib.h"

#define BLKSIZE		(16*1024)
#define NBLOCK		1000

void
fillbuf (char *buf, unsigned n_buf, int blkno)
{
	long *		w = (long *) buf;
	int		nw = n_buf / sizeof *w;
	int		i;

	for (i = 0; i < nw; i++) {
		w[i] = (blkno<<16) + i;
	}
}

int
chkbuf (char *buf, unsigned n_buf, int blkno)
{
	long *		w = (long *) buf;
	int		nw = n_buf / sizeof *w;
	int		i;

	for (i = 0; i < nw; i++) {
		if (w[i] != (blkno<<16) + i) {
printf ("got 0x%lx exp 0x%lx\n", w[i], ((long)blkno<<16) + i);
			return -1;
		}
	}
	return 0;
}

int
main (int ac, char *av[])
{
	char		iobuf[BLKSIZE];
	struct wrap_ccb	_wccb, *wccb = &_wccb;
	int		fd, i, rc;

	NDMOS_MACRO_ZEROFILL (wccb);

	wccb->iobuf = iobuf;
	wccb->n_iobuf = sizeof iobuf;

	fd = creat ("wt1.data", 0666);
	if (fd < 0) {
		perror ("create wt1.data");
		return 1;
	}

	for (i = 0; i < NBLOCK; i++) {
		fillbuf (iobuf, sizeof iobuf, i);
		write (fd, iobuf, sizeof iobuf);
	}

 	fillbuf (iobuf, sizeof iobuf, 0xFFF);

	close (fd);
	fd = open ("wt1.data", 0666);
	if (fd < 0) {
		perror ("re-open wt1.data");
		return 2;
	}

	wccb->data_conn_fd = fd;

	for (i = 0; i < NBLOCK; i++) {
		/* get the first 100 of each block */
		rc = wrap_reco_seek (wccb, i*sizeof iobuf, 256, 100);
		if (rc) {
			printf ("err=%d '%s'\n", rc, wccb->errmsg);
			break;
		}

		/* then get 512 */
		rc = wrap_reco_must_have (wccb, 512);
		if (rc) {
			printf ("err=%d '%s'\n", rc, wccb->errmsg);
			break;
		}

		/* then get 512 more */
		rc = wrap_reco_must_have (wccb, 1024);
		if (rc) {
			printf ("err=%d '%s'\n", rc, wccb->errmsg);
			break;
		}

		if (chkbuf (wccb->have, 1024, i) != 0) {
			printf ("BOING %d\n", i);
		}
	}

	close (fd);
	remove ("wt1.data");

	return 0;
}
