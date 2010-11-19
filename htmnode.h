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

#ifndef __HTMNODE_H__
#define __HTMNODE_H__

#include "fdr.h"
#include "components.h"

#define VAL(nc) ((nc)->val)
#define EXP(nc) ((nc)->val + 1)

struct node_conn {
  bitar_t val[2];
};

void node_conn_init(struct node_conn *nc, size_t mapsize);

typedef struct node {
  size_t input_w;
  size_t output_w;

  bitar_t col_temp[1];
  bitar_t col_exp[1];
  struct spool sp[1];
  struct seqlearn sq[1];
  struct tpool tp[1];
} node_t;

void node_init(node_t *n, size_t input_w, size_t columns_w,
	       size_t sp_in_conn_fct, size_t sp_act_fct,
	       size_t sq_cells_per_col, size_t sq_col_conn_fct);
int node_process(node_t *n, struct node_conn *in, struct node_conn *out);

#endif /* HTMNODE_H */
