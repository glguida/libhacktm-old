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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "fdr.h"
#include "components.h"

void
synapse_init_random(struct synapse *s, size_t wconns, size_t ndest)
{
  size_t i;
  bitar_t tmp;
  s->totcount = 0;
  bitar_init(&tmp, ndest/WORD_BITSIZE + 1);
  s->wconn = malloc(sizeof(struct wconn) * wconns);
  OOMCHK(s->wconn);
  for ( i = 0; i < wconns; i++ ) {
    s->wconn[i].count = 0;
    s->wconn[i].id = bitar_setrandomid_range(&tmp, ndest);
  }
  bitar_fini(&tmp);
}

void
synapse_init_linear(struct synapse *s, size_t wconns)
{
 size_t i;
  s->totcount = 0;
  s->wconn = malloc(sizeof(struct wconn) * wconns);
  OOMCHK(s->wconn);
  for ( i = 0; i < wconns; i++ ) {
    s->wconn[i].count = 0;
    s->wconn[i].id = i;
  }
}

void
synapse_learn(struct synapse *s, size_t wconns, bitar_t *dst)
{
  size_t i;
  int add = 0;
  for ( i = 0; i < wconns; i++ )
    if ( bitar_get(dst, s->wconn[i].id) ) {
      add = 1;
      s->wconn[i].count ++;
    }
  if ( add ) s->totcount++;
}

void
synapse_predict(struct synapse *s, size_t wconns, bitar_t *src, bitar_t *dst)
{
  size_t i;
  if ( s->totcount == 0 )
    return;

  for ( i = 0; i < wconns; i++ ) {
    if ( ((s->wconn[i].count * 10) / s->totcount) > 7 )
      bitar_set(dst, s->wconn[i].id);
  }
}

void
synapse_dump(struct synapse *s, size_t wconns)
{
  size_t i;
  for ( i = 0; i < wconns; i++ ) {
    if ( s->wconn[i].count == 0 )
      continue;
    printf("%lu:%lu/%lu\t", s->wconn[i].id, s->wconn[i].count, s->totcount);
  }
}

/*
 * Array of synapsis (learn and predict based on experience, very basic implementation)
 */

void
synarray_init_sub(struct synar *sa, size_t syns, size_t dest, size_t wconns)
{
  size_t i;
  sa->syns = syns;
  sa->wconns = wconns;

  sa->array = malloc(sizeof(struct synapse) * syns);
  OOMCHK(sa->array);

  for ( i = 0; i < syns; i++ )
    synapse_init_random(sa->array + i, wconns, dest);
}

void
synarray_init(struct synar *sa, size_t syns, size_t wconns)
{
  size_t i;
  sa->syns = syns;
  sa->wconns = wconns;

  sa->array = malloc(sizeof(struct synapse) * syns);
  OOMCHK(sa->array);

  for ( i = 0; i < syns; i++ )
    synapse_init_linear(sa->array + i, wconns);
}

void
synarray_learn(struct synar *sa, bitar_t *prev, bitar_t *cur)
{
  size_t i;
  const size_t syns = (const size_t)sa->syns;
  const size_t wconns = (const size_t)sa->wconns;
  assert ( syns == bitar_bitsize(prev) );
  //  assert ( bitar_bitsize(prev) == bitar_bitsize(cur) );

  for ( i = 0; i < bitar_bitsize(prev); i++ )
    if ( bitar_get(prev, i) )
      synapse_learn(sa->array + i, wconns, cur);
}

void
synarray_predict(struct synar *sa, bitar_t *cur, bitar_t *next)
{
  size_t i;
  const size_t syns = (const size_t)sa->syns;
  const size_t wconns = (const size_t)sa->wconns;
  assert ( syns == bitar_bitsize(cur) );
  //  assert ( bitar_bitsize(next) == bitar_bitsize(cur) );

  bitar_reset(next);
  for ( i = 0; i < bitar_bitsize(cur); i++ ) {
    if ( bitar_get(cur, i) )
      synapse_predict(sa->array + i, wconns, cur, next);
  }
}

void
synarray_dump(struct synar *sa)
{
  size_t i;
  const size_t syns = (const size_t)sa->syns;
  const size_t wconns = (const size_t)sa->wconns;

  for ( i = 0; i < syns; i++ ) {
    if ( sa->array[i].totcount < 2 )
      continue;
    printf("\n %lu\n", i);
    synapse_dump(sa->array + i, wconns);
  }
}

/*
 * Spatial pooler.
 */

void
spool_init(struct spool *sp, size_t input_w, size_t columns_w, size_t conns, size_t maxbits)
{
  sp->input_w = input_w;
  sp->columns_w = columns_w;
  sp->conns = conns;
  sp->maxbits = maxbits;
#ifdef SEE_FEEDBACK_FAIL
  synarray_init_sub(sp->synar, BITSIZE(columns_w), BITSIZE(input_w), BITSIZE(input_w)/2);
#endif
  fdr_init(sp->in2col, input_w, columns_w, conns, maxbits);
}

void
spool_init_std(struct spool *sp, size_t input_w, size_t columns_w, size_t conn_factor, size_t act_factor)
{
  size_t conns = BITSIZE(input_w)/conn_factor;
  size_t maxbits = BITSIZE(columns_w)/act_factor;
  spool_init(sp, input_w, columns_w, conns, maxbits);
  
}

void spool(struct spool *sp, bitar_t *in, bitar_t *expected, bitar_t *out)
{
  fdr_feedback_generate(sp->in2col, in, expected, out);
#ifdef SEE_FEEDBACK_FAIL
  synarray_learn(sp->synar, out, in);
#endif
}
#ifdef SEE_FEEDBACK_FAIL
void spool_reverse(struct spool *sp, bitar_t *columns, bitar_t *input)
{
  synarray_predict(sp->synar, columns, input);
}
#endif

/*
 * Sequence learner.
 */

void
seqlearn_init(struct seqlearn *sq, size_t columns_w, size_t cells_per_col, size_t col2cur_conns, size_t prev2cur_conns)
{
  size_t i;
  sq->columns_w = columns_w;
  sq->output_w = columns_w * cells_per_col;
  
  fdr_init(sq->col2cur, sq->columns_w, sq->output_w, col2cur_conns, 1);

  synarray_init_sub(sq->prev2cur, BITSIZE(sq->output_w), BITSIZE(sq->output_w), prev2cur_conns);
  
  bitar_init(sq->col_prev, sq->columns_w);
  bitar_init(sq->prev, sq->output_w);
  bitar_init(sq->expected, sq->output_w);

  bitar_setall(sq->prev);
}

void
seqlearn_init_std(struct seqlearn *sq, size_t columns_w, size_t cells_per_col, size_t conn_factor)
{
  size_t col2cur_conns = BITSIZE(columns_w)/conn_factor;
  size_t prev2cur_conns = (BITSIZE(columns_w) * cells_per_col) / conn_factor;
  seqlearn_init(sq, columns_w, cells_per_col, col2cur_conns, prev2cur_conns);
}

int
seqlearn(struct seqlearn *sq, bitar_t *col_in, bitar_t *feedback, bitar_t *out)
{
  size_t i;
  size_t cells_per_col = sq->output_w/sq->columns_w;
  assert(bitar_size(col_in) == sq->columns_w);

  if ( bitar_dist(sq->col_prev, col_in) == 0 ) {
    bitar_copy(out, sq->prev);
    synarray_predict(sq->prev2cur, out, sq->expected);
    return 0;
  }

  /* Select cells. */
  fdr_groupselect(sq->col2cur, cells_per_col, sq->col_prev, out, feedback, col_in);

  i = bitar_dist(out, sq->expected);

  synarray_learn(sq->prev2cur, sq->prev, out);
  synarray_predict(sq->prev2cur, out, sq->expected);

  bitar_copy(sq->prev, out);
  bitar_copy(sq->col_prev, col_in);
  return 1 + i;
}

/*
 * Temporal pooler.
 */

void
tpool_init(struct tpool *tp, size_t output_w, struct synar *synar)
{
  tp->synar = synar;
  bitar_init(tp->and, output_w);
  bitar_init(tp->in_prev, output_w);
  bitar_init(tp->out_prev, output_w);
  bitar_init(tp->tmp, output_w);
  bitar_init(tp->tmp + 1, output_w);
}

int
tpool(struct tpool *tp, bitar_t *out)
{
  int i;
  bitar_and(out, tp->out_prev, tp->and);
  if ( tp->and->count == out->count ) { 
    bitar_copy(tp->in_prev, out);
    /* No change: output the previous value. */
    bitar_copy(out, tp->out_prev);
    return 0;
  }

  bitar_copy(tp->in_prev, out);
  bitar_copy(tp->tmp, out);
  for ( i = 0; i < 5; i++ ) {
    bitar_reset(tp->tmp + ((i + 1) % 2));
    synarray_predict(tp->synar, tp->tmp + (i % 2),
		     tp->tmp + ((i + 1) % 2));
    bitar_or(tp->tmp + ((i + 1) % 2), out, out);
  }

  bitar_copy(tp->out_prev, out);
  return 1;
}

