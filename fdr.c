/*
 * Copyright (C) 2010 Gianluca Guida <glguida@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include "fdr.h"

size_t
word_hweight(uint64_t x)
{
  x = ( x & 0x5555555555555555LL ) + ( (x>>1) & 0x5555555555555555LL );  
  x = ( x & 0x3333333333333333LL ) + ( (x>>2) & 0x3333333333333333LL );  
  x = ( x & 0x0F0F0F0F0F0F0F0FLL ) + ( (x>>4) & 0x0F0F0F0F0F0F0F0FLL );  
  x = ( x & 0x00FF00FF00FF00FFLL ) + ( (x>>8) & 0x00FF00FF00FF00FFLL );  
  x = ( x & 0x0000FFFF0000FFFFLL ) + ( (x>>16) & 0x0000FFFF0000FFFFLL );  
  x = ( x & 0x00000000FFFFFFFFLL ) + ( (x>>32) & 0x00000000FFFFFFFFLL );  
  return (size_t)x;  
}


void
bitar_init(bitar_t *ba, size_t map_size)
{
  ba->count = 0;
  ba->map_size = map_size;
  ba->map = malloc(map_size * WORD_BYTESIZE);
  OOMCHK(ba->map);
  memset(ba->map, 0, map_size * WORD_BYTESIZE);
}

void
bitar_fini(bitar_t *ba)
{
  free(ba->map);
  ba->map = 0;
  ba->count = 0;
  ba->map_size = 0;
}

bitar_t *
bitar_alloc(void)
{
  bitar_t *ba = malloc(sizeof(bitar_t));
  OOMCHK(ba);
  return ba;
}

void
bitar_free(bitar_t *ba)
{
  free(ba);
}

void
bitar_getsub(bitar_t *dst, bitar_t *src, size_t off)
{
  size_t i;
  assert(src->map_size >= off + dst->map_size);
  memcpy(dst->map, src->map + off, dst->map_size * WORD_BYTESIZE);
  dst->count = 0;
  for ( i = 0; i < dst->map_size; i++ )
    dst->count += word_hweight(dst->map[i]);
}

void
bitar_putsub(bitar_t *dst, size_t off, bitar_t *src)
{
  size_t i;
  assert(dst->map_size >= off + src->map_size);
  memcpy(dst->map + off, src->map, src->map_size * WORD_BYTESIZE);
  dst->count = 0;
  for ( i = 0; i < dst->map_size; i++ )
    dst->count += word_hweight(dst->map[i]);
}

void
bitar_scan(bitar_t *ba, void (*_func)(size_t id, void *priv), void *priv)
{
#if 1
  size_t i, j;
  size_t id;
  uint64_t word;

  
  for ( i = 0; i < ba->map_size; i++ ) {
    if ( ba->map[i] == 0 )
      continue;
    word = ba->map[i];
    for ( j = 0; j < sizeof(uint64_t)/sizeof(long); j++ ) {
      long *ptr = (long *)&word + j;
      id = ffsl(*ptr);
      while ( id != 0 ) {
	id--;
	_func(i * WORD_BITSIZE + j * sizeof(long) * 8 + id, priv);
	*ptr &= ~(1L << id);
	id = ffsl(*ptr);
      }
    }
  }

#else
  size_t i;
  /* Slow dumb version. Seriously, Optimize it. */
  for ( i = 0; i < BITSIZE(ba->map_size); i++ )
    if ( bitar_get(ba, i) )
      _func(i, priv);
#endif
}

void
bitar_copy(bitar_t *d, bitar_t *s)
{
  assert(d->map_size == s->map_size);
  d->count = s->count;
  memcpy(d->map, s->map, d->map_size*WORD_BYTESIZE);
}

void
bitar_iterate(bitar_t *ba, void (*_func)(bitar_t *), size_t l)
{
  for ( ba->map[l] = 0; ba->map[l] < UINT64_MAX; ba->map[l]++)
    if ( l == ba->map_size - 1 ) {
      _func(ba);
    } else {
      bitar_iterate(ba, _func, l+1);
    }
}

void
bitar_reset(bitar_t *ba)
{
  ba->count = 0;
  memset(ba->map, 0, ba->map_size * WORD_BYTESIZE);
}

void bitar_setall(bitar_t *ba)
{
  size_t i;
  ba->count = 64 * ba->map_size;
  for ( i = 0; i < ba->map_size; i++ )
    ba->map[i] = UINT64_MAX;
}

void
bitar_complement(bitar_t *ba, size_t id)
{
  size_t mid = id/WORD_BITSIZE;
  assert(mid < ba->map_size);
  if ( ba->map[mid] & (uint64_t)(1LL << (id % WORD_BITSIZE)) )
    ba->count--;
  else
    ba->count++;
  ba->map[mid] ^= (uint64_t)(1LL << (id % WORD_BITSIZE));
}

void
bitar_set(bitar_t *ba, size_t id)
{
  size_t mid = id/WORD_BITSIZE;
  assert(mid < ba->map_size);
  ba->map[mid] |= (uint64_t)(1LL << (id % WORD_BITSIZE));
  ba->count++;
}

int
bitar_get(bitar_t *ba, size_t id)
{
  size_t mid = id/WORD_BITSIZE;
  assert(mid < ba->map_size);
  return !!(ba->map[mid] & (uint64_t)(1LL << (id % WORD_BITSIZE)));
}

size_t
bitar_setrandomid_range(bitar_t *ba, size_t range)
{
  size_t id;
  do {
    id = (size_t)(arc4random() % range);
  } while ( bitar_get(ba, id) );

  bitar_set(ba, id);
  return id;
}

size_t 
bitar_setrandomid(bitar_t *ba)
{
  size_t bs = bitar_bitsize(ba);
  return bitar_setrandomid_range(ba, bs);
}

void
bitar_or(bitar_t *a, bitar_t *b, bitar_t *or)
{
  int i;
  size_t r = 0, ms = a->map_size;
  assert(a->map_size == b->map_size);
  assert(a->map_size == or->map_size);
  for ( i = 0; i < ms; i++ ) {
    or->map[i] = a->map[i] | b->map[i];
    r += word_hweight(or->map[i]);
  }
  or->count = r;
}

void
bitar_nand(bitar_t *a, bitar_t *b, bitar_t *nand)
{
  int i;
  size_t r = 0, ms = a->map_size;
  assert(a->map_size == b->map_size);
  assert(a->map_size == nand->map_size);
  for ( i = 0; i < ms; i++ ) {
    nand->map[i] = a->map[i] & ~b->map[i];
    r += word_hweight(nand->map[i]);
  }
  nand->count = r;
}

void
bitar_and(bitar_t *a, bitar_t *b, bitar_t *and)
{
  int i;
  size_t r = 0, ms = a->map_size;
  assert(a->map_size == b->map_size);
  assert(a->map_size == and->map_size);
  for ( i = 0; i < ms; i++ ) {
    and->map[i] = a->map[i] & b->map[i];
    r += word_hweight(and->map[i]);
  }
  and->count = r;
}

/* FIXME: I don't check for integer overflow. */
struct _bitar_vmult {
  float *v;
  size_t cnt;
};

static void _vector_mult(size_t i, void *priv)
{
  struct _bitar_vmult *vmul = (struct _bitar_vmult *)priv;
  vmul->cnt += (size_t)vmul->v[i];
}

size_t
bitar_vector_mult(float *v, bitar_t *b)
{
  struct _bitar_vmult vmul = { .v = v, .cnt = 0 };
  bitar_scan(b, _vector_mult, &vmul);
  return vmul.cnt;
}

/* Hamming Distance. */
size_t 
bitar_dist(bitar_t *a, bitar_t *b)
{
  int i;
  size_t r = 0, ms = a->map_size;
  assert(a->map_size == b->map_size);
  for ( i = 0; i < ms; i++ ) {
    r += word_hdist(a->map[i], b->map[i]);
  }
  return r;
}

struct _bitar_scale {
  bitar_t *out;
  int scale;
};

static void
_scale(size_t i, void *priv)
{
  struct _bitar_scale *_bs = (struct _bitar_scale *)priv;

  if (_bs->scale <= 0 )
    bitar_set(NULL, i * -(_bs->scale));
  else
    bitar_set(_bs->out, i / _bs->scale);
}

void
bitar_scale(bitar_t *in, bitar_t *out)
{
  size_t is = in->map_size, os = out->map_size;
  struct _bitar_scale bs = { .out = out, .scale = (is > os ? is/os : -(os/is)) };
  bitar_reset(out);
  bitar_scan(in, _scale, &bs);
}

void
_bitar_dump(bitar_t *ba, int (*print)(void *priv, const char *, ...), void *priv)
{
#define PRINT(fmt, ...) do { if ( print == NULL ) printf(fmt, __VA_ARGS__); else print(priv, fmt, __VA_ARGS__); } while (0)
  int i, j;
  size_t ms = ba->map_size;
  for ( i = ms -1; i >= 0; i-- )
    {
      uint64_t map = ba->map[i];
      PRINT("%016llx", map);
    }
  PRINT("\nCount is %lu\n", ba->count);
}

void
bitar_dump(bitar_t *ba)
{
  _bitar_dump(ba, NULL, NULL);
}

/*
 * FDR generator.
 */


#define COLUMNS(_fdr) BITSIZE((_fdr)->output_mapsize)

/* Coincidence Detection helpers */

static void
coinc_reset(fdr_t *fdr)
{
  size_t i;
  for ( i = 0; i < fdr->trunc; i++ ) {
    fdr->coinc_array[i].val = 0;
    fdr->coinc_array[i].col = 0;
  }
}

static void
coinc_add(fdr_t *fdr, size_t col, size_t val)
{
  size_t i, imin = (size_t)-1, min = (size_t)-1; 
  for ( i = 0; i < fdr->trunc; i++ )
    if ( fdr->coinc_array[i].val < val 
	 && fdr->coinc_array[i].val < min ) {
      min = fdr->coinc_array[i].val;
      imin = i;
    }
  if ( imin != (size_t)-1 ) {
    fdr->coinc_array[imin].col = col;
    fdr->coinc_array[imin].val = val;
  }
}

static void
coinc_get_fdr(fdr_t *fdr, bitar_t *bitar, size_t trunc)
{
  size_t i;
  for ( i = 0; i < fdr->trunc; i++)
    bitar_set(bitar, fdr->coinc_array[i].col);
}

void fdr_init(fdr_t *fdr, size_t input_mapsize, 
	      size_t output_mapsize, size_t conn_bits, size_t trunc)
{
  int i, j;
  fdr->input_mapsize = input_mapsize;
  fdr->output_mapsize = output_mapsize;
  fdr->conn_bits = conn_bits;
  fdr->trunc = trunc;

  bitar_init(&fdr->and, input_mapsize);

  fdr->incols = malloc(sizeof(bitar_t) * BITSIZE(output_mapsize));
  assert(fdr->incols);
  for ( i = 0; i < BITSIZE(output_mapsize); i++ ) {
      bitar_init(&fdr->incols[i], input_mapsize);
      for ( j = 0; j < conn_bits; j++ )
	bitar_setrandomid(&fdr->incols[i]);
  }

  fdr->coinc_array = malloc(sizeof(struct coinc_array) * trunc);
  coinc_reset(fdr);
}

struct _gs_priv {
  fdr_t *fdr;
  size_t groupsz;
  bitar_t *input;
  bitar_t *output;
  bitar_t *predict;
};

static void
_groupselect(size_t in_id, void *priv)
{
  struct _gs_priv *gsp = (struct _gs_priv *)priv;
  size_t i, toadd;
  size_t id = in_id * gsp->groupsz;
  for ( i = 0; i < gsp->groupsz; i++ ) {
    bitar_and(gsp->input, &gsp->fdr->incols[id + i], &gsp->fdr->and);
    toadd = gsp->fdr->and.count;
    if ( gsp->predict != NULL )
      if ( bitar_get(gsp->predict, id + i) )
	toadd += 3; /* XXX: FEEDBACK WEIGHT */
    coinc_add(gsp->fdr, id + i, toadd);
  }
  coinc_get_fdr(gsp->fdr, gsp->output, 1);
  coinc_reset(gsp->fdr);
}

void
fdr_groupselect(fdr_t *fdr, size_t group, bitar_t *input, bitar_t *output, bitar_t *predict, bitar_t *discriminant)
{
  bitar_reset(output);
  //  assert(input->map_size == output->map_size);
  assert(discriminant->map_size * group == output->map_size);
  struct _gs_priv gsp = { .fdr = fdr, .groupsz = group, .input = input, .output = output, .predict = predict };
  bitar_scan(discriminant, _groupselect, &gsp);
}

/* Floating Point Vector fdr */
/* TODO: pass a float vector (i.e., know the size!) instead of a pointer. */
void
vfdr_generate(fdr_t *fdr, float *input, bitar_t *expected, bitar_t *output)
{
  size_t i;
  size_t count;
  for ( i = 0; i < COLUMNS(fdr); i++ ) {
    count = bitar_vector_mult(input, &fdr->incols[i]);
    if ( expected != NULL )
      if ( bitar_get(expected, i) )
	count += 3;
    coinc_add(fdr, i, count);
  }
  coinc_get_fdr(fdr, output, fdr->trunc);
  coinc_reset(fdr);

}

void
fdr_feedback_generate(fdr_t *fdr, bitar_t *input, bitar_t *expected, bitar_t *output)
{
  size_t i;
  bitar_t *and = &fdr->and;

  for ( i = 0; i < COLUMNS(fdr); i++ ) {
    bitar_and(input, &fdr->incols[i], and);
    if ( expected != NULL )
      if ( bitar_get(expected, i) )
	and->count += 3;
    coinc_add(fdr, i, and->count);
  }
  coinc_get_fdr(fdr, output, fdr->trunc);
  coinc_reset(fdr);
}

void
fdr_generate(fdr_t *fdr, bitar_t *input, bitar_t *output)
{
  fdr_feedback_generate(fdr, input, NULL, output);
}

