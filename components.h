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

#ifndef __COMPONENTS_H__
#define __COMPONENTS_H__

#include "fdr.h"

struct synapse {
    size_t totcount;
    struct wconn {
      size_t id;
      size_t count;
    } *wconn;
};

void synapse_init_random(struct synapse *s, size_t wconns, size_t ndest);
void synapse_init_linear(struct synapse *s, size_t wconns);
void synapse_learn(struct synapse *s, size_t wconns, bitar_t *dst);
void synapse_predict(struct synapse *s, size_t wconns, bitar_t *src, bitar_t *dst);
void synapse_dump(struct synapse *s, size_t wconns);

struct synar {
  size_t syns;
  size_t wconns;
  struct synapse *array;
};

void synarray_init_sub(struct synar *sa, size_t syns, size_t dest, size_t wconns);
void synarray_init(struct synar *sa, size_t syns, size_t wconns);
void synarray_learn(struct synar *sa, bitar_t *prev, bitar_t *cur);
void synarray_predict(struct synar *sa, bitar_t *cur, bitar_t *next);
void synarray_dump(struct synar *sa);

struct spool {
  size_t input_w;
  size_t columns_w;
  size_t conns;
  size_t maxbits;

  bitar_t input[1];
  fdr_t in2col[1];
  struct synar synar[1];
};

void spool_init(struct spool *sp, size_t input_w, size_t columns_w, size_t conns, size_t maxbits);
void spool_init_std(struct spool *sp, size_t input_w, size_t columns_w, size_t conn_factor, size_t act_factor);
void spool_merge(struct spool *sp, bitar_t *in, size_t ins, bitar_t *expected, bitar_t *out);
void spool(struct spool *sp, bitar_t *in, bitar_t *expected, bitar_t *out);
void spool_reverse(struct spool *sp, bitar_t *columns, bitar_t *input);

struct seqlearn {
  size_t columns_w;
  size_t output_w;

  fdr_t col2cur[1];
  size_t col2cur_conns;

  struct synar prev2cur[1];

  bitar_t col_prev[1]; /* Previous input value */
  bitar_t prev[1]; /* Previous output value. */
  bitar_t expected[1]; /* Expected next output value. */
};

void seqlearn_init(struct seqlearn *sq, size_t columns_w, size_t cells_per_col, size_t col2cur_conns, size_t prev2cur_conns);
void seqlearn_init_std(struct seqlearn *sq, size_t columns_w, size_t cells_per_col, size_t conn_factor);
int seqlearn(struct seqlearn *sq, bitar_t *col_in, bitar_t *feedback, bitar_t *out);

struct tpool {
  struct synar *synar;
  bitar_t and[1];
  bitar_t out_prev[1];
  bitar_t in_prev[1];
  bitar_t tmp[2];
};

void tpool_init(struct tpool *tp, size_t output_w, struct synar *synar);
int tpool(struct tpool *tp, bitar_t *out);

#endif /* COMPONENTS_H */
