#include <cat/cat.h>

#if CAT_USE_STDLIB

#include <cat/stdcsv.h>
#include <cat/stduse.h>
#include <cat/grow.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static int local_getchar(void *fp)
{
	int c;
	FILE *f = fp;
	errno = 0;
	c = fgetc(f);
	if ( c == EOF ) {
		if ( ferror(f) )
			return CSV_GETC_ERR;
		else
			return CSV_GETC_EOF;
	}
	return c;
}


int csv_fopen(struct csv_state *csv, const char *filename)
{
	FILE *f;
	if ( (f = fopen(filename, "r")) == NULL )
		return CSV_ERR;
	csv_init(csv, local_getchar, f);
	return CSV_OK;
}


int csv_fclose(struct csv_state *csv)
{
	if ( fclose(csv->csv_gcctx) < 0 )
		return CSV_ERR;
	return CSV_OK;
}


int csv_read_field(struct csv_state *csv, char **res)
{
	char *s = NULL;
	size_t slen = 32, soff = 0, t;
	int code;

	s = emalloc(slen);
	do {
		abort_unless(slen + 2 > slen);
		if ( soff > slen - 2 )
			mem_grow(&estdmem, &s, &slen, slen + 2);
		code = csv_next(csv, &s[soff], slen - soff, &t);
		soff += t;
	} while  ( code == CSV_CNT );

	if ( code != CSV_ERR )
		*res = s;
	else
		free(s);
	return code;
}


int csv_read_rec(struct csv_state *csv, struct csv_record *cr)
{
	unsigned long nf, i;
	int code = CSV_OK;
	struct list *list;
	char *field, **record;

	abort_unless(csv != NULL);
	abort_unless(cr != NULL);

	list = clist_newlist();

	for ( nf = 0 ; (code != CSV_REC) && (code != CSV_EOF) ; ++nf ) {
		abort_unless(nf <= (unsigned long)~0);
		code = csv_read_field(csv, &field);
		if ( code == CSV_ERR )
			goto err;
		else if ( code != CSV_EOF )
			break;
		clist_enq(list, char *, field);
	}

	if ( code == CSV_EOF ) {
		abort_unless(nf == 0);
		clist_freelist(list);
		return CSV_EOF;
	}

	record = emalloc(sizeof(char **) * nf);
	for ( i = 0 ; i < nf ; ++i ) {
		record[i] = clist_qnext(list, char *);
		clist_deq(list);
	}

	cr->cr_nfields = nf;
	cr->cr_fields = record;

	clist_freelist(list);
	return CSV_REC;

err:
	clist_freelist(list);
	return CSV_ERR;
}


void csv_free_rec(struct csv_record *cr)
{
	int i;
	for ( i = 0 ; i < cr->cr_nfields ; ++i )
		free(cr->cr_fields[i]);
	free(cr->cr_fields);
	memset(cr, 0, sizeof(*cr));
}

#endif /* CAT_USE_STDLIB */
