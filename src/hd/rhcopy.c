#ifndef lint
static const char	RCSid[] = "$Id: rhcopy.c,v 3.31 2018/10/05 19:19:16 greg Exp $";
#endif
/*
 * Copy data into a holodeck file
 */

#include "platform.h"
#include "rterror.h"
#include "holo.h"
#include "view.h"

#ifndef BKBSIZE
#define BKBSIZE		256		/* beam clump size (kilobytes) */
#endif

int	checkdepth = 1;		/* check depth (!-d option)? */
int	checkrepeats = 0;	/* check for repeats (-u option)? */
int	frompicz;		/* input from pictures & depth-buffers? */
int	noutsects;		/* number of output sections */
char	obstr, unobstr;		/* flag pointer values */

char	*progname;		/* global argv[0] */

struct phead {
	VIEW	vw;
	double	expos;
	short	gotview;
	short	badfmt;
	short	altprims;
};
static int openholo(char *fname, int append);
static void addray(FVECT ro, FVECT rd, double d, COLR cv);
static gethfunc holheadline;
static int bpcmp(const void *b1p, const void *b2p);
static int addclump(HOLO *hp, int *bq, int nb);
static void addholo(char *hdf);
static gethfunc picheadline;
static void addpicz(char *pcf, char *zbf);



int
main(
	int	argc,
	char	*argv[]
)
{
	int	i;

	progname = argv[0];
	frompicz = -1;
	for (i = 2; i < argc && argv[i][0] == '-'; i++)
		switch (argv[i][1]) {
		case 'u':
			checkrepeats = 1;
			break;
		case 'd':
			checkdepth = 0;
			break;
		case 'h':
			frompicz = 0;
			break;
		case 'p':
			frompicz = 1;
			break;
		default:
			goto userr;
		}
	if (i >= argc || frompicz < 0)
		goto userr;
	if (frompicz && (argc-i)%2)
		goto userr;
	noutsects = openholo(argv[1], 1);
	if (frompicz) {
		for ( ; i < argc; i += 2)
			addpicz(argv[i], argv[i+1]);
	} else {
		if (BKBSIZE*1024*1.5 > hdcachesize)
			hdcachesize = BKBSIZE*1024*1.5;
		for ( ; i < argc; i++)
			addholo(argv[i]);
	}
	quit(0);
userr:
	fprintf(stderr, "Usage: %s output.hdk [-u][-d] -h inp1.hdk ..\n",
			progname);
	fprintf(stderr, "   Or: %s output.hdk [-u][-d] -p inp1.hdr inp1.zbf ..\n",
			progname);
	exit(1);
}


#define H_BADF	01
#define H_OBST	02
#define H_OBSF	04

static int
holheadline(		/* check holodeck header line */
	char	*s,
	void	*vhf
)
{
	char	fmt[MAXFMTLEN];
	int	*hf = vhf;

	if (formatval(fmt, s)) {
		if (strcmp(fmt, HOLOFMT))
			*hf |= H_BADF;
		else
			*hf &= ~H_BADF;
		return(0);
	}
	if (!strncmp(s, "OBSTRUCTIONS=", 13)) {
		s += 13;
		while (*s == ' ') s++;
		if ((*s == 't') | (*s == 'T'))
			*hf |= H_OBST;
		else if ((*s == 'f') | (*s == 'F'))
			*hf |= H_OBSF;
		else
			error(WARNING, "bad OBSTRUCTIONS value in holodeck");
		return(0);
	}
	return(0);
}

int
openholo(		/* open existing holodeck file for i/o */
	char	*fname,
	int	append
)
{
	FILE	*fp;
	int	fd;
	int	hflags = 0;
	off_t	nextloc;
	int	n;
					/* open holodeck file */
	if ((fp = fopen(fname, append ? "r+" : "r")) == NULL) {
		sprintf(errmsg, "cannot open \"%s\" for %s", fname,
				append ? "appending" : "reading");
		error(SYSTEM, errmsg);
	}
					/* check header and magic number */
	if (getheader(fp, holheadline, &hflags) < 0 ||
			hflags&H_BADF || getw(fp) != HOLOMAGIC) {
		sprintf(errmsg, "file \"%s\" not in holodeck format", fname);
		error(USER, errmsg);
	}
	fd = dup(fileno(fp));			/* dup file handle */
	nextloc = ftell(fp);			/* get stdio position */
	fclose(fp);				/* done with stdio */
	for (n = 0; nextloc > 0L; n++) {	/* initialize each section */
		lseek(fd, nextloc, SEEK_SET);
		read(fd, (char *)&nextloc, sizeof(nextloc));
		hdinit(fd, NULL)->priv = hflags&H_OBST ? &obstr :
				hflags&H_OBSF ? &unobstr : (char *)NULL;
	}
	return(n);
}

#undef H_BADF
#undef H_OBST
#undef H_OBSF


void
addray(		/* add a ray to our output holodeck */
	FVECT	ro,
	FVECT	rd,
	double	d,
	COLR	cv
)
{
	int	sn, bi, n;
	HOLO	*hp;
	GCOORD	gc[2];
	uby8	rr[2][2];
	BEAM	*bp;
	double	d0, d1;
	unsigned	dc;
	RAYVAL	*rv;
				/* check each output section */
	for (sn = noutsects; sn--; ) {
		hp = hdlist[sn];
		d0 = hdinter(gc, rr, &d1, hp, ro, rd);
		if (d <= d0 || d1 < -0.001)
			continue;	/* missed section */
		if (checkdepth) {		/* check depth */
			if (hp->priv == &obstr && d0 < -0.001)
				continue;	/* ray starts too late */
			if (hp->priv == &unobstr && d < 0.999*d1)
				continue;	/* ray ends too soon */
		}
		dc = hdcode(hp, d-d0);
		bi = hdbindex(hp, gc);		/* check for duplicates */
		if (checkrepeats && (bp = hdgetbeam(hp, bi)) != NULL) {
			for (n = bp->nrm, rv = hdbray(bp); n--; rv++)
				if ((hp->priv != NULL || rv->d == dc) &&
						rv->r[0][0] == rr[0][0] &&
						rv->r[0][1] == rr[0][1] &&
						rv->r[1][0] == rr[1][0] &&
						rv->r[1][1] == rr[1][1])
					break;
			if (n >= 0)
				continue;	/* found a matching ray */
		}
		rv = hdnewrays(hp, bi, 1);
		rv->d = dc;
		rv->r[0][0] = rr[0][0]; rv->r[0][1] = rr[0][1];
		rv->r[1][0] = rr[1][0]; rv->r[1][1] = rr[1][1];
		copycolr(rv->v, cv);
	}
}


static BEAMI	*beamdir;

static int
bpcmp(			/* compare beam positions on disk */
	const void	*b1p,
	const void	*b2p
)
{
	off_t	pdif = beamdir[*(int*)b1p].fo - beamdir[*(int*)b2p].fo;

	if (pdif > 0L) return(1);
	if (pdif < 0L) return(-1);
	return(0);
}

static int
addclump(		/* transfer the given clump and free */
	HOLO	*hp,
	int	*bq,
	int	nb
)
{
	GCOORD	gc[2];
	FVECT	ro, rd;
	double	d;
	int	i;
	int	k;
	BEAM	*bp;
					/* sort based on file position */
	beamdir = hp->bi;
	qsort((char *)bq, nb, sizeof(*bq), bpcmp);
					/* transfer each beam */
	for (i = 0; i < nb; i++) {
		bp = hdgetbeam(hp, bq[i]);
		hdbcoord(gc, hp, bq[i]);
						/* add each ray to output */
		for (k = bp->nrm; k--; ) {
			d = hdray(ro, rd, hp, gc, hdbray(bp)[k].r);
			if (hp->priv == &unobstr)
				VSUM(ro, ro, rd, d);
			else
				d = 0.;
			d = hddepth(hp, hdbray(bp)[k].d) - d;
			addray(ro, rd, d, hdbray(bp)[k].v);
		}
		hdfreebeam(hp, bq[i]);		/* free the beam */
	}
	return(0);
}


void
addholo(			/* add a holodeck file */
	char	*hdf
)
{
	int	fd;
					/* open the holodeck for reading */
	openholo(hdf, 0);
	fd = hdlist[noutsects]->fd;	/* remember the file handle */
	while (hdlist[noutsects] != NULL) {	/* load each section */
							/* clump the beams */
		clumpbeams(hdlist[noutsects], 0, BKBSIZE*1024, addclump);
		hddone(hdlist[noutsects]);		/* free the section */
	}
	close(fd);			/* close input file */
	hdflush(NULL);			/* flush output */
}



static int
picheadline(		/* process picture header line */
	char	*s,
	void	*vph
)
{
	char	fmt[32];
	struct phead *ph = vph;

	if (formatval(fmt, s)) {
		ph->badfmt = strcmp(fmt, COLRFMT);
		return(0);
	}
	if (isprims(s)) {
		ph->altprims++;		/* don't want to deal with this */
		return(0);
	}
	if (isexpos(s)) {
		ph->expos *= exposval(s);
		return(0);
	}
	if (isview(s)) {
		ph->gotview += sscanview(&ph->vw, s);
		return(0);
	}
	return(0);
}


void
addpicz(		/* add a picture + depth-buffer */
	char	*pcf,
	char	*zbf
)
{
	FILE	*pfp;
	int	zfd;
	COLR	*cscn;
	float	*zscn;
	struct phead	phd;
	int	eshft;
	double	emult;
	RESOLU	prs;
	RREAL	vl[2];
	FVECT	ro, rd;
	double	aftd;
	COLOR	ctmp;
	int	j;
	int	i;
				/* open files */
	if ((pfp = fopen(pcf, "r")) == NULL) {
		sprintf(errmsg, "cannot open picture file \"%s\"", pcf);
		error(SYSTEM, pcf);
	}
	if ((zfd = open(zbf, O_RDONLY)) < 0) {
		sprintf(errmsg, "cannot open depth file \"%s\"", zbf);
		error(SYSTEM, pcf);
	}
				/* load picture header */
	phd.vw = stdview;
	phd.expos = 1.0;
	phd.badfmt = phd.gotview = phd.altprims = 0;
	if (getheader(pfp, picheadline, &phd) < 0 ||
			phd.badfmt || !fgetsresolu(&prs, pfp)) {
		sprintf(errmsg, "bad format for picture file \"%s\"", pcf);
		error(USER, errmsg);
	}
	if (!phd.gotview || setview(&phd.vw) != NULL) {
		sprintf(errmsg, "missing/illegal view in picture \"%s\"",
				pcf);
		error(USER, errmsg);
	}
	if (phd.altprims) {
		sprintf(errmsg, "ignoring color primaries in picture \"%s\"",
				pcf);
		error(WARNING, errmsg);
	}
				/* figure out what to do about exposure */
	if ((phd.expos < 0.99) | (phd.expos > 1.01)) {
		emult = -log(phd.expos)/log(2.);
		eshft = emult >= 0. ? emult+.5 : emult-.5;
		emult -= (double)eshft;
		if ((emult <= 0.01) & (emult >= -0.01))
			emult = -1.;
		else {
			emult = 1./phd.expos;
			eshft = 0;
		}
	} else {
		emult = -1.;
		eshft = 0;
	}
				/* allocate buffers */
	cscn = (COLR *)malloc(scanlen(&prs)*sizeof(COLR));
	zscn = (float *)malloc(scanlen(&prs)*sizeof(float));
	if ((cscn == NULL) | (zscn == NULL))
		error(SYSTEM, "out of memory in addpicz");
				/* read and process each scanline */
	for (j = 0; j < numscans(&prs); j++) {
		i = scanlen(&prs);			/* read colrs */
		if (freadcolrs(cscn, i, pfp) < 0) {
			sprintf(errmsg, "error reading picture \"%s\"", pcf);
			error(USER, errmsg);
		}
		if (eshft)				/* shift exposure */
			shiftcolrs(cscn, i, eshft);
		i *= sizeof(float);			/* read depth */
		if (read(zfd, (char *)zscn, i) != i) {
			sprintf(errmsg, "error reading depth file \"%s\"", zbf);
			error(USER, errmsg);
		}
		for (i = scanlen(&prs); i--; ) {	/* do each pixel */
			if (zscn[i] <= 0.0)
				continue;		/* illegal depth */
			pix2loc(vl, &prs, i, j);
			aftd = viewray(ro, rd, &phd.vw, vl[0], vl[1]);
			if (aftd < -FTINY)
				continue;		/* off view */
			if (aftd > FTINY && zscn[i] > aftd)
				continue;		/* aft clipped */
			if (emult > 0.) {		/* whatta pain */
				colr_color(ctmp, cscn[i]);
				scalecolor(ctmp, emult);
				setcolr(cscn[i], colval(ctmp,RED),
					colval(ctmp,GRN), colval(ctmp,BLU));
			}
			addray(ro, rd, (double)zscn[i], cscn[i]);
		}
	}
				/* write output and free beams */
	hdflush(NULL);
				/* clean up */
	free((void *)cscn);
	free((void *)zscn);
	fclose(pfp);
	close(zfd);
}


void
eputs(			/* put error message to stderr */
	char  *s
)
{
	static int  midline = 0;

	if (!*s)
		return;
	if (!midline++) {	/* prepend line with program name */
		fputs(progname, stderr);
		fputs(": ", stderr);
	}
	fputs(s, stderr);
	if (s[strlen(s)-1] == '\n') {
		fflush(stderr);
		midline = 0;
	}
}


void
quit(			/* exit the program gracefully */
	int	code
)
{
	hdsync(NULL, 1);	/* write out any buffered data */
	exit(code);
}
