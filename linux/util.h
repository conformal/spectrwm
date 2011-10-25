#define FPARSELN_UNESCESC	0x01
#define FPARSELN_UNESCCONT	0x02
#define FPARSELN_UNESCCOMM	0x04
#define FPARSELN_UNESCREST	0x08
#define FPARSELN_UNESCALL	0x0f

size_t	strlcpy(char *, const char *, size_t);
size_t	strlcat(char *, const char *, size_t);

char   *fgetln(FILE *, size_t *);
char   *fparseln(FILE *, size_t *, size_t *, const char [3], int);

long long strtonum(const char *, long long, long long, const char **);

#ifndef WAIT_ANY
#define WAIT_ANY		(-1)
#endif

/* there is no limit to ulrich drepper's crap */
#ifndef TAILQ_END
#define	TAILQ_END(head)			NULL
#endif
