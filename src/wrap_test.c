#include "ndmos.h"
#include "wraplib.h"


/*
 * Path names are one, two, or three components longs.
 * They are represented by a 32-bit pathcode.
 */

#define PATHCODE_CONS(P0,P1,P2) \
		(  (((P0)&0xFF)<<16) | (((P1)&0xFF)<<8) | (((P2)&0xFF)<<0))
#define PATHCODE_P0(PC)		(((PC)>>16)&0xFF)
#define PATHCODE_P1(PC)		(((PC)>>8)&0xFF)
#define PATHCODE_P2(PC)		(((PC)>>0)&0xFF)
#define PATHCODE_IS_DIR(PC)	(PATHCODE_P2(PC) == 0)
#define PATHCODE_IS_FILE(PC)	(PATHCODE_P2(PC) != 0)

#define PATHCODE_END_MARKER	(-1ul)

/*
 * An entry binds a pathcode to a fileno
 */
struct entry {
	unsigned long	pathcode;
	unsigned long	fileno;
};

#define MAX_ENTRY	100000
struct entry		table[MAX_ENTRY];
int			n_table;

/*
 * fileno allocation is done random()ly to a max of 100 per block.
 */
#define N_FILENO_BLOCK_ALLO	20000
unsigned char	fileno_block_allo[N_FILENO_BLOCK_ALLO];

#define DIRSIZE			5000
#define FILESIZE		123789

/*
 * Format of "header" in image
 */

struct test_header {
	unsigned long	pathcode;
	unsigned long	fileno;
	unsigned long	size;
	unsigned long	mtime;
	char		path[200];
	struct {
		unsigned long	pathcode;
		unsigned long	fileno;
		char		name[24];
	}		dirent[1];		/* end marked name[0]==0 */
};


struct test_ccb {
	struct wrap_ccb *	wccb;
	int			history_mode;
	unsigned long		mtime;
	char			iobuf[FILESIZE+32*1024];
};


/* forward */
extern void		start_table (void);
extern unsigned long	allo_fileno (void);
extern int		pathcode_str (unsigned long pathcode, char *path);
extern int		pathcode_name_str (unsigned long pathcode, char *path);
extern unsigned long	parent_pathcode (unsigned long pathcode);
struct entry *		find_entry_by_pathcode (unsigned long pathcode);
extern void		populate_table (int width);
extern int		cmp_pathcode (const void *a1, const void *a2);
extern int		cmp_fileno (const void *a1, const void *a2);
extern void		sort_table (int by_fileno);
extern void		dump_table (void);
extern void		dump_table_dirs (void);
extern void		dump_table_files (void);

extern void		ent_to_fstat (struct entry *ent,
				struct wrap_fstat *fstat,
				unsigned long mtime);
extern int		test_backup (struct test_ccb *tccb);
extern int		test_recovfh (struct test_ccb *tccb);
extern void		send_fh  (struct test_ccb *tccb, struct entry *ent,
				unsigned long long fhinfo,
				unsigned long mtime);
extern void		send_image  (struct test_ccb *tccb, struct entry *ent,
				unsigned long long fhinfo);
extern void		send_end_marker  (struct test_ccb *tccb);




int
main (int argc, char *argv[])
{
	struct wrap_ccb		_wccb, *wccb = &_wccb;
	struct test_ccb		_tccb, *tccb = &_tccb;
	int			rc;

	NDMOS_MACRO_ZEROFILL (tccb);
	tccb->wccb = wccb;
	tccb->mtime = time(0);

	rc = wrap_main (argc, argv, wccb);
	if (rc) {
		fprintf (stderr, "Error: %s\n", wccb->errmsg);
		return 1;
	}

	if (wccb->hist_enable == 'y') {
		char *		p;

		p = strrchr (argv[0], '_');
		if (p) {
			if (strcmp (p, "_dump") == 0) {
				wccb->hist_enable = 'd';
			}
			if (strcmp (p, "_tar") == 0) {
				wccb->hist_enable = 'f';
			}
		}
	}

	wccb->iobuf = tccb->iobuf;
	wccb->n_iobuf = sizeof tccb->iobuf;

	switch (wccb->op) {
	case WRAP_CCB_OP_BACKUP:
		rc = test_backup (tccb);
		break;

#if 0
	case WRAP_CCB_OP_RECOVER:
#endif

	case WRAP_CCB_OP_RECOVER_FILEHIST:
		rc = test_recovfh (tccb);
		break;

	default:
		fprintf (stderr, "Op unimplemented\n");
		break;
	}

	return 0;
}

int
test_backup (struct test_ccb *tccb)
{
	struct wrap_ccb *	wccb = tccb->wccb;
	unsigned long long	fhinfo = 0;
	int			ix;
	struct entry *		ent;

	start_table();
	populate_table (10);

	switch (wccb->hist_enable) {
	default:
		/* Hmmm. */
		sort_table (0);
		for (ix = 0; ix < n_table; ix++) {
			ent = &table[ix];
			send_image (tccb, ent, fhinfo);
			fhinfo += FILESIZE;
		}
		break;

	case 'f':
		sort_table (0);
		for (ix = 0; ix < n_table; ix++) {
			ent = &table[ix];
			send_fh (tccb, ent, fhinfo, tccb->mtime);
			send_image (tccb, ent, fhinfo);
			if (PATHCODE_IS_FILE (ent->pathcode))
				fhinfo += FILESIZE;
			else
				fhinfo += DIRSIZE;
		}
		break;

	case 'y':
	case 'd':
		sort_table (1);
		/* directories */
		for (ix = 0; ix < n_table; ix++) {
			ent = &table[ix];
			if (PATHCODE_P2(ent->pathcode) != 0)
				continue;
			send_fh (tccb, ent, fhinfo, tccb->mtime);
			send_image (tccb, ent, fhinfo);
			fhinfo += DIRSIZE;
		}
		/* nodes */
		for (ix = 0; ix < n_table; ix++) {
			ent = &table[ix];
			if (PATHCODE_P2(ent->pathcode) == 0)
				continue;
			send_fh (tccb, ent, fhinfo, tccb->mtime);
			send_image (tccb, ent, fhinfo);
			fhinfo += FILESIZE;
		}
		break;
	}

	send_end_marker (tccb);

	return 0;
}

int
test_recovfh (struct test_ccb *tccb)
{
	struct wrap_ccb *	wccb = tccb->wccb;
	unsigned long long	fhinfo = 0;
	int			rc;
	struct test_header *	th;
	struct entry		_ent, *ent = &_ent;
	struct wrap_fstat	fstat;

	switch (wccb->hist_enable) {
	default:
		wrap_send_log_message (wccb->index_fp,
					"Recov FH invalid HIST=");
		break;

	case 0:
	case 'f':
	case 'y':
	case 'd':
		break;
	}


	wccb->hist_enable = 'f';	/* temporary */

	for (;;) {
		rc = wrap_reco_seek (wccb, fhinfo, sizeof *th, sizeof *th);
		if (rc) {
			wrap_send_log_message (wccb->index_fp,
					"Failed seek/read of header");
			break;
		}

		th = (struct test_header *) wccb->have;

		if (th->pathcode == PATHCODE_END_MARKER) {
			/* Victory! */
			break;
		}

		ent->pathcode = th->pathcode;
		ent->fileno = th->fileno;
		ent_to_fstat (ent, &fstat, th->mtime);
		fstat.size = th->size;

		wrap_send_add_file (wccb->index_fp, th->path, fhinfo, &fstat);

		fhinfo += th->size;
	}

	return 0;
}

int
oldmain (int argc, char *argv[])
{
	start_table();
	populate_table (10);

	sort_table (0);
	printf ("By pathcode\n");
	dump_table();

	sort_table (1);
	printf ("By fileno\n");
	printf ("Dirs\n");
	dump_table_dirs();
	printf ("Files\n");
	dump_table_files();

	return 0;
}

void
send_fh  (struct test_ccb *tccb, struct entry *ent,
  unsigned long long fhinfo, unsigned long mtime)
{
	struct wrap_ccb *	wccb = tccb->wccb;
	struct wrap_fstat	fstat;
	char			path[1024];
	char *			p;
	unsigned long		par_pc;
	int			ix;
	struct entry *		dent;

	ent_to_fstat (ent, &fstat, mtime);

	switch (wccb->hist_enable) {
	case 0:
		return;

	case 'f':
		p = wccb->backup_root;
		if (!p)
			p = "/BACKUP-ROOT-GUESS";
		strcpy (path, p);
		for (p = path; *p; p++) {
			continue;
		}
		while (p > path && p[-1] == '/') p--;
		*p++ = '/';
		pathcode_str (ent->pathcode, p);
		if (strcmp (p, ".") == 0) {
			*p = 0;
		}
		if (strcmp (path, "") == 0) {
			strcpy (path, ".");
		}
		wrap_send_add_file (wccb->index_fp, path, fhinfo, &fstat);
		break;

	case 'y':
	case 'd':
		if (PATHCODE_P2(ent->pathcode)) {
			/* file */
		} else {
			/* directory */
			wrap_send_add_dirent (wccb->index_fp, ".",
				fhinfo, ent->fileno, ent->fileno);
			par_pc = parent_pathcode (ent->pathcode);
			dent = find_entry_by_pathcode (par_pc);
			if (dent) {
				wrap_send_add_dirent (wccb->index_fp, "..",
					fhinfo, ent->fileno, dent->fileno);
			}
			for (ix = 0; ix < n_table; ix++) {
				dent = &table[ix];
				if (dent == ent)
					continue;	/* happens on root */
				par_pc = parent_pathcode (dent->pathcode);
				if (par_pc != ent->pathcode)
					continue;
				pathcode_name_str (dent->pathcode, path);
				wrap_send_add_dirent (wccb->index_fp, path,
					fhinfo, ent->fileno, dent->fileno);
			}
		}
		wrap_send_add_node (wccb->index_fp, fhinfo, &fstat);
		break;
	}
}

void
send_image  (struct test_ccb *tccb, struct entry *ent,
  unsigned long long fhinfo)
{
	struct wrap_ccb *	wccb = tccb->wccb;
	struct test_header *	th = (struct test_header *) tccb->iobuf;

	NDMOS_MACRO_ZEROFILL (th);

	th->pathcode = ent->pathcode;
	th->fileno = ent->fileno;
	th->mtime = tccb->mtime;
	if (PATHCODE_IS_FILE(ent->pathcode))
		th->size = FILESIZE;
	else
		th->size = DIRSIZE;
	pathcode_str (ent->pathcode, th->path);

	write (wccb->data_conn_fd, tccb->iobuf, th->size);
}

void
send_end_marker  (struct test_ccb *tccb)
{
	struct wrap_ccb *	wccb = tccb->wccb;
	struct test_header *	th = (struct test_header *) tccb->iobuf;

	NDMOS_MACRO_ZEROFILL (th);
	th->pathcode = PATHCODE_END_MARKER;
	th->size = sizeof *th;

	write (wccb->data_conn_fd, tccb->iobuf, th->size);
}


void
ent_to_fstat (struct entry *ent, struct wrap_fstat *fstat, unsigned long mtime)
{
	NDMOS_MACRO_ZEROFILL (fstat);

	if (PATHCODE_IS_FILE(ent->pathcode)) {
		fstat->ftype = WRAP_FTYPE_REG;
		fstat->mode = 0664;
		fstat->size = FILESIZE;
	} else {
		fstat->ftype = WRAP_FTYPE_DIR;
		fstat->mode = 0775;
		fstat->size = DIRSIZE;
	}
	fstat->valid |= WRAP_FSTAT_VALID_FTYPE;
	fstat->valid |= WRAP_FSTAT_VALID_MODE;
	fstat->valid |= WRAP_FSTAT_VALID_SIZE;

	fstat->uid = 0;
	fstat->valid |= WRAP_FSTAT_VALID_UID;

	fstat->gid = 0;
	fstat->valid |= WRAP_FSTAT_VALID_GID;

	fstat->atime = mtime;
	fstat->valid |= WRAP_FSTAT_VALID_ATIME;

	fstat->mtime = mtime;
	fstat->valid |= WRAP_FSTAT_VALID_MTIME;

	fstat->ctime = mtime;
	fstat->valid |= WRAP_FSTAT_VALID_CTIME;

	fstat->fileno = ent->fileno;
	fstat->valid |= WRAP_FSTAT_VALID_FILENO;
}






void
start_table (void)
{
	/* reserve chunk, install "root" */
	fileno_block_allo[0] = 100;
	table[0].pathcode = 0;
	table[0].fileno = 2;
	n_table = 1;
}

void
populate_table (int width)
{
	int		p0, p1, p2;
	int		ix = n_table;
	unsigned long	pathcode;

	for (p0 = 0; p0 < width; p0++) {
	    for (p1 = 0; p1 < width; p1++) {
		for (p2 = 0; p2 < width; p2++) {
			pathcode = PATHCODE_CONS(p0,p1,p2);
			if (pathcode == 0)
				continue;

			if (ix >= MAX_ENTRY) {
				goto out;
			}

			table[ix].pathcode = pathcode;
			table[ix].fileno = allo_fileno();
			ix++;
		}
	    }
	}
  out:
	n_table = ix;
}


unsigned long
allo_fileno (void)
{
	unsigned long		fileno;
	unsigned long		ix;

	for (;;) {
		ix = fileno = random() % N_FILENO_BLOCK_ALLO;
		if (fileno_block_allo[ix] >= 100) {
			continue;
		}
		fileno <<= 8;
		fileno += fileno_block_allo[ix]++;
		return fileno;
	}
}

unsigned long
parent_pathcode (unsigned long pathcode)
{
	if (PATHCODE_P2(pathcode)) {
		return pathcode & PATHCODE_CONS(-1,-1,0);
	}

	if (PATHCODE_P1(pathcode)) {
		return pathcode & PATHCODE_CONS(-1,0,0);
	}

	return 0;	/* root */
}

struct entry *
find_entry_by_pathcode (unsigned long pathcode)
{
	int		ix;
	struct entry *	ent;

	for (ix = 0; ix < n_table; ix++) {
		ent = &table[ix];
		if (ent->pathcode == pathcode)
			return ent;
	}

	return 0;
}


int
pathcode_str (unsigned long pathcode, char *path)
{
	char *		p = path;
	int		comp;

	*p = 0;
	comp = PATHCODE_P0(pathcode);
	if (comp) {
		sprintf (p, "/usr%03d", comp);
		while (*p) p++;
	}

	comp = PATHCODE_P1(pathcode);
	if (comp) {
		sprintf (p, "/dir%03d", comp);
		while (*p) p++;
	}

	comp = PATHCODE_P2(pathcode);
	if (comp) {
		sprintf (p, "/file%03d", comp);
		while (*p) p++;
	}

	if (p == path) {
		strcpy (p, "/.");
	}

	strcpy (path, path+1);

	return 0;
}

int
pathcode_name_str (unsigned long pathcode, char *path)
{
	char *		p = path;
	int		comp;

	comp = PATHCODE_P2(pathcode);
	if (comp) {
		sprintf (p, "file%03d", comp);
		return 0;
	}

	comp = PATHCODE_P1(pathcode);
	if (comp) {
		sprintf (p, "dir%03d", comp);
		return 0;
	}

	comp = PATHCODE_P0(pathcode);
	if (comp) {
		sprintf (p, "usr%03d", comp);
		return 0;
	}

	*p = 0;

	return 0;
}


void
sort_table (int by_fileno)
{
	qsort (table, n_table, sizeof table[0],
		by_fileno ? cmp_fileno : cmp_pathcode);
}

int
cmp_pathcode (const void *a1, const void *a2)
{
	struct entry *	e1 = (struct entry *) a1;
	struct entry *	e2 = (struct entry *) a2;

	return e1->pathcode - e2->pathcode;
}

int
cmp_fileno (const void *a1, const void *a2)
{
	struct entry *	e1 = (struct entry *) a1;
	struct entry *	e2 = (struct entry *) a2;

	return e1->fileno - e2->fileno;
}

void
dump_table (void)
{
	int		i;
	char		path[100];

	for (i = 0; i < n_table; i++) {
		pathcode_str (table[i].pathcode, path);
		printf ("%4d: %8lu %s\n", i, table[i].fileno, path);
	}
}

void
dump_table_dirs (void)
{
	int		i;
	char		path[100];

	for (i = 0; i < n_table; i++) {
		if (PATHCODE_P2(table[i].pathcode) != 0)
			continue;
		pathcode_str (table[i].pathcode, path);
		printf ("%4d: %8lu %s\n", i, table[i].fileno, path);
	}
}

void
dump_table_files (void)
{
	int		i;
	char		path[100];

	for (i = 0; i < n_table; i++) {
		if (PATHCODE_P2(table[i].pathcode) == 0)
			continue;
		pathcode_str (table[i].pathcode, path);
		printf ("%4d: %8lu %s\n", i, table[i].fileno, path);
	}
}
