/* Copyright (c) 1990 Regents of the University of California */

/* SCCSid "$SunId$ LBL" */

/*
 * Translator definitions
 *
 *	Greg Ward
 */

#define MAXSTR		128	/* maximum input string length */

#define VOIDID		"void"	/* null modifier */

				/* qualifier list */
typedef struct {
	int	nquals;			/* number of qualifiers */
	char	**qual;			/* qualifier array */
} QLIST;
				/* identifier */
typedef struct {
	char	*name;			/* string, NULL if number */
	int	number;
} ID;
				/* identifier list */
typedef struct {
	int	nids;			/* number of ids */
	ID	*id;			/* id array */
} IDLIST;
				/* identifier range */
typedef struct {
	char	*nam;			/* string, NULL if range */
	int	min, max;		/* accepted range */
} IDMATCH;
				/* mapping rule */
typedef struct rule {
	char	*mnam;			/* material name */
	long	qflg;			/* qualifier condition flags */
	struct rule	*next;		/* next rule in mapping */
	/* followed by the IDMATCH array */
} RULEHD;
				/* useful macros */
#define doneid(idp)	if ((idp)->name != NULL) freestr((idp)->name)
#define FL(qn)		(1L<<(qn))
#define rulsiz(nq)	(sizeof(RULEHD)+(nq)*sizeof(IDMATCH))
#define idm(rp)		((IDMATCH *)((rp)+1))

char	*malloc(), *calloc(), *realloc();
char	*savestr();
RULEHD	*getmapping();
