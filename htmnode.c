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

#include "htmnode.h"

/*
 * Nodes.
 */

void
node_conn_init(struct node_conn *nc, size_t mapsize)
{
  bitar_init(nc->val, mapsize);
  bitar_init(nc->val + 1, mapsize);
}

void
node_init(node_t *n, size_t input_w, size_t columns_w,
	  size_t sp_in_conn_fct, size_t sp_act_fct,
	  size_t sq_cells_per_col, size_t sq_col_conn_fct)
{
  n->input_w = input_w;
  n->output_w = columns_w * sq_cells_per_col;

  bitar_init(n->col_temp, columns_w);
  bitar_init(n->col_exp, columns_w);
  spool_init_std(n->sp, input_w, columns_w, sp_in_conn_fct, sp_act_fct);
  seqlearn_init_std(n->sq, columns_w, sq_cells_per_col, sq_col_conn_fct);
  tpool_init(n->tp, n->output_w, n->sq->prev2cur);
}

int
node_process(node_t *n, struct node_conn *in, struct node_conn *out)
{
  int r;
  bitar_reset(VAL(out));
  bitar_reset(n->col_temp);
  bitar_reset(n->col_exp);
  bitar_scale(n->sq->expected, n->col_exp);
  spool(n->sp, VAL(in), n->col_exp, n->col_temp);
  r = seqlearn(n->sq, n->col_temp, EXP(out), VAL(out));
  if ( !r )
    return 0;
  r = tpool(n->tp, VAL(out));

#ifdef SEE_FEEDBACK_FAIL
  /* XXX: Feedback is *not* working */
  bitar_reset(n->col_temp);
  bitar_scale(n->sq->expected, n->col_temp);
  spool_reverse(n->sp, n->col_temp, EXP(in));
#endif

  if ( !r )
    return 0;
  return 1;
}

