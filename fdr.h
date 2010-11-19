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

#ifndef __FDR_H__
#define __FDR_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define WORD_BITSIZE 64
#define WORD_BYTESIZE 8
#define BITSIZE(_n) ((_n) * WORD_BITSIZE)

#include <assert.h>
#define OOMCHK(_ptr) assert((_ptr) != NULL)

#ifdef CONFIG_DETERMINISTIC
#define RANDOM(_max) (random() % (_max))
#else
#define RANDOM(_max) (arc4random() % (_max))
#endif

/*
 * Hamming Distance.
 */

size_t word_hweight(uint64_t x);
#define word_hdiff(_x, _y) ((_x) ^ (_y))
#define word_hdist(_x, _y) word_hweight((_x) ^ (_y))

/*
 * Bit Arrays. 
 */

typedef struct bitar {
  size_t count;
  size_t map_size;
  uint64_t *map;
} bitar_t;

void bitar_init(bitar_t *ba, size_t map_size);
void bitar_fini(bitar_t *ba);
bitar_t *bitar_alloc(void);
void bitar_free(bitar_t *ba);
void bitar_scan(bitar_t *ba, void (*_func)(size_t id, void *priv), void *priv);
void bitar_copy(bitar_t *d, bitar_t *s);
void bitar_iterate(bitar_t *ba, void (*_func)(bitar_t *), size_t l);
void bitar_reset(bitar_t *ba);
void bitar_setall(bitar_t *ba);
void bitar_complement(bitar_t *ba, size_t id);
void bitar_set(bitar_t *ba, size_t id);
int bitar_get(bitar_t *ba, size_t id);
size_t bitar_setrandomid(bitar_t *ba);
size_t bitar_setrandomid_range(bitar_t *ba, size_t range);
void bitar_or(bitar_t *a, bitar_t *b, bitar_t *or);
void bitar_nand(bitar_t *a, bitar_t *b, bitar_t *nand);
void bitar_and(bitar_t *a, bitar_t *b, bitar_t *and);
size_t bitar_dist(bitar_t *a, bitar_t *b);
void bitar_dump(bitar_t *ba);

void bitar_putsub(bitar_t *dst, size_t off, bitar_t *src);
void bitar_getsub(bitar_t *dst, bitar_t *src, size_t off);

#define bitar_count(_ba) ((_ba)->count)
#define bitar_size(_ba) ((_ba)->map_size)
#define bitar_bitsize(_ba) ((_ba)->map_size * WORD_BITSIZE)

/*
 * FDR generator.
 */

struct coinc_array {
  size_t col;
  size_t val;
};

typedef struct fdr {
  size_t input_mapsize;
  size_t output_mapsize;
  size_t conn_bits;
  size_t trunc;
  bitar_t and;
  bitar_t *incols;
  struct coinc_array *coinc_array;
} fdr_t;

void fdr_init(fdr_t *fdr, size_t input_mapsize, 
	      size_t output_mapsize, size_t conn_bits, size_t trunc);
void fdr_init_reverse(fdr_t *reverse, fdr_t *fdr, size_t trunc);
void fdr_groupselect(fdr_t *fdr, size_t group, bitar_t *input, bitar_t *output, bitar_t *predict, bitar_t *discriminant);
void fdr_generate(fdr_t *fdr, bitar_t *input, bitar_t *output);
void fdr_feedback_generate(fdr_t *fdr, bitar_t *input, bitar_t *expected, bitar_t *output);
void vfdr_generate(fdr_t *vdfr, float *input, bitar_t *expected, bitar_t *output);

#define FDR_FEEDBACK_WEIGHT 5

#endif /* FDR_H */
