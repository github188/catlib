#include <cat/cat.h>
#include <cat/dbgmem.h>
#include <cat/err.h>
#include <cat/hash.h>

/* TODO:  Make thread safe */

#if CAT_USE_STDLIB
#include <stdlib.h>
#else  /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */

#ifndef CAT_DBGMEM_NBUCKETS
#define CAT_DBGMEM_NBUCKETS	4096
#endif /* CAT_DBGMEM_NBUCKETS */

struct memmgr dbgmem = { dbg_mem_get, dbg_mem_resize, dbg_mem_free, NULL };
static unsigned long dbg_alloc_amt = 0;
static unsigned long dbg_num_alloc = 0;

static struct hashsys dbg_hashsys = { 
  cmp_ptr, 
  ht_phash,
  NULL
};
static struct list dbg_ht_buckets[CAT_DBGMEM_NBUCKETS];
static struct htab dbg_htab = { 
  dbg_ht_buckets, 
  CAT_DBGMEM_NBUCKETS, 
  CAT_DBGMEM_NBUCKETS - 1,
  { cmp_ptr, ht_phash, NULL }
};

struct dbg_header {
  struct hnode	dh_hnode;
  size_t	dh_amt;
};

union dbg_header_u {
  cat_align_t	dhu_align;
  struct hnode	dhu_hnode;
};

#define ROUNDUP(amt) ((((amt) + sizeof(union dbg_header_u)-1) / \
           sizeof(union dbg_header_u)) * sizeof(union dbg_header_u))
#define TOMEM(dh) ((void *)((union dbg_header_u *)(dh) + 1))

static int initialized = 0;


static void dbg_initialize(void)
{
  ht_init(&dbg_htab, dbg_ht_buckets, CAT_DBGMEM_NBUCKETS, &dbg_hashsys);
  initialized = 1;
}



void *dbg_mem_get(struct memmgr *mm, size_t amt)
{
  size_t ramt;
  struct dbg_header *dh;
  void *p;

  abort_unless(mm == &dbgmem);
  if ( !initialized )
    dbg_initialize();

  ramt = ROUNDUP(amt);
  abort_unless(ramt >= amt);

  dh = malloc(ramt);
  if ( dh == NULL )
    return NULL;

  ++dbg_num_alloc;
  dbg_alloc_amt += amt;
  dh->dh_amt = amt;
  
  p = TOMEM(dh);
  ht_ninit(&dh->dh_hnode, p, dh, ht_phash(p, NULL));
  ht_ins(&dbg_htab, &dh->dh_hnode);

  return p;
}


void *dbg_mem_resize(struct memmgr *mm, void *p, size_t newsize)
{
  size_t ramt;
  struct dbg_header *dh;
  struct hnode *hn;
  void *p2;
  size_t osize;

  abort_unless(mm == &dbgmem);
  if ( !initialized )
    dbg_initialize();

  hn = ht_lkup(&dbg_htab, p, NULL); 
  if ( hn == NULL )
    errsys("dbg_mem_resize: attempt to reallocate non dynamic"
           "pointer: %p", p);
  dh = container(hn, struct dbg_header, dh_hnode);
  osize = dh->dh_amt;

  if ( newsize > 0 ) {
    ramt = ROUNDUP(newsize);
    abort_unless(ramt >= newsize);
  } else {
    ramt = 0;
  }

  p2 = realloc(dh, ramt);
  abort_unless(ramt > 0 || p2 == NULL);
  if ( p == NULL ) {
    if ( ramt == 0 ) {
      dbg_alloc_amt -= osize;
      --dbg_num_alloc;
    }
    return NULL;
  }

  dbg_alloc_amt = dbg_alloc_amt - osize + newsize;
  dh = p2;
  dh->dh_amt = newsize;
  p = TOMEM(dh);
  ht_ninit(&dh->dh_hnode, p, dh, ht_phash(p, NULL));
  ht_ins(&dbg_htab, &dh->dh_hnode);

  return p;
}


void dbg_mem_free(struct memmgr *mm, void *p)
{
  struct hnode *hn;
  struct dbg_header *dh;

  abort_unless(mm == &dbgmem);
  if ( !initialized )
    dbg_initialize();

  hn = ht_lkup(&dbg_htab, p, NULL); 
  if ( hn == NULL )
    errsys("dbg_mem_free: attempt to free non-dynamic pointer: %p",
           p);
  dh = container(hn, struct dbg_header, dh_hnode);
  dbg_alloc_amt -= dh->dh_amt;
  --dbg_num_alloc;
  free(dh);
}


unsigned long dbg_get_num_alloc(void)
{
  return dbg_num_alloc;
}


unsigned long dbg_get_alloc_amt(void)
{
  return dbg_alloc_amt;
}


int dbg_is_dyn_mem(void *p)
{
  if ( !initialized )
    dbg_initialize();
  return ht_lkup(&dbg_htab, p, NULL) != NULL;
}

void dbg_mem_reset(void)
{
  struct list *le, *end;

  if ( !initialized ) {
    dbg_initialize();
    return;
  }
  dbg_alloc_amt = 0;
  dbg_num_alloc = 0;
  for ( le = dbg_htab.tab, end = le + dbg_htab.size; le < end ; ++le )
    while ( !l_isempty(le) )
      ht_rem(container(le->next, struct hnode, le));
}
