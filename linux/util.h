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

#ifndef TAILQ_FOREACH_SAFE
#define	TAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = TAILQ_FIRST(head);					\
	    (var) != TAILQ_END(head) &&					\
	    ((tvar) = TAILQ_NEXT(var, field), 1);			\
	    (var) = (tvar))
#endif

#ifndef SIMPLEQ_HEAD
#define SIMPLEQ_HEAD                    STAILQ_HEAD
#define SIMPLEQ_HEAD_INITIALIZER        STAILQ_HEAD_INITIALIZER
#define SIMPLEQ_ENTRY                   STAILQ_ENTRY
#define SIMPLEQ_INIT                    STAILQ_INIT
#define SIMPLEQ_INSERT_AFTER            STAILQ_INSERT_AFTER
#define SIMPLEQ_INSERT_HEAD             STAILQ_INSERT_HEAD
#define SIMPLEQ_INSERT_TAIL             STAILQ_INSERT_TAIL
#define SIMPLEQ_EMPTY                   STAILQ_EMPTY
#define SIMPLEQ_FIRST                   STAILQ_FIRST
#define SIMPLEQ_REMOVE_AFTER            STAILQ_REMOVE_AFTER
#define SIMPLEQ_REMOVE_HEAD             STAILQ_REMOVE_HEAD
#define SIMPLEQ_FOREACH                 STAILQ_FOREACH
#define SIMPLEQ_END(head)               NULL
#endif
