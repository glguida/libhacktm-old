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

/*
 * Test #1. Try to learn the patterns behind a very dense bit-field
 * long sequence.
 * Be sure to have a *HUGE* window.
 */

#include "htmnode.h"

#define INPUT_W 40

#include <ncurses.h>
WINDOW *win_n3_out;
WINDOW *win_n2_out;
WINDOW *win_n1_out;
WINDOW *win_input;
WINDOW *win_stat_out;


#define bitardisp_init(_num, _win)			\
  (_win) = newwin(6, COLS - 4, 2 + (_num)*4, 2);	\
	 box((_win), 0, 0);				\
	 wrefresh((_win));

void
bitar_dumpw(WINDOW *win, bitar_t *ba)
{
  static int k;
  int i, j;
  size_t ms = ba->map_size;
  wmove(win, 1, 1);
  k = 0;
  for ( i = ms -1; i >= 0; i-- )
    {
      uint64_t map = ba->map[i];
#if 1
      wprintw(win, " %016llx ", map);
      if ( (++k % ((COLS - 6)/18) == 0) )
	wmove(win, getcury(win) + 1, 1);
#else
      for ( j = WORD_BITSIZE; j > 0; j-- )
	{
	  if ( map & (1LL << (WORD_BITSIZE - 1)) )
	    {
	      putchar('1');
	    }
	  else
	    putchar('0');
	  map <<= 1;
	  } 
#endif
    }
  wrefresh(win);
}

void
display_init(void)
{
  initscr();
  noecho();
  bitardisp_init(0, win_input);
  bitardisp_init(2, win_n1_out);
  bitardisp_init(4, win_n2_out);
  bitardisp_init(6, win_n3_out);
  bitardisp_init(8, win_stat_out);

  wmove(win_input, 0, 0);
  wprintw(win_input, "Input: ");
  wrefresh(win_input);
  wmove(win_n1_out, 0, 0);
  wprintw(win_n1_out, "L1:   ");
  wrefresh(win_n1_out);
  wmove(win_n2_out, 0, 0);
  wprintw(win_n2_out, "L2:   ");
  wrefresh(win_n2_out);
  wmove(win_n3_out, -2, -1);
  wprintw(win_n3_out, "L3:   ");
  wrefresh(win_n3_out);
}

int
main()
{
  size_t i, j;
  node_t n1, n2, n3;
  struct node_conn l0, l1, l2, l3;
  bitar_t in[16];
  bitar_t inn[16];

  display_init();

  node_init(&n1, INPUT_W, 2, 5, 10, 8, 2);
  node_init(&n2, 16, 2, 5, 10, 8, 2);
  node_init(&n3, 16, 2, 5, 10, 8, 2);
  
  node_conn_init(&l0, INPUT_W);
  node_conn_init(&l1, 16);
  node_conn_init(&l2, 16);
  node_conn_init(&l3, 16);
  

  for ( j = 0; j < 16; j++ ) {
    bitar_init(in+j, INPUT_W);
    for ( i = 0; i < INPUT_W; i++ ) {
      in[j].map[i] = (uint32_t)arc4random() | ((uint64_t)arc4random() << 31);
      in[j].count += word_hweight(in[j].map[i]);
    }
  }

  for ( j = 0; j < 16; j++ ) {
    bitar_init(inn+j, INPUT_W);
    for ( i = 0; i < INPUT_W; i++ ) {
      inn[j].map[i] = in[15 - j].map[i];
      inn[j].count += word_hweight(inn[j].map[i]);
    }
  }

  {
    size_t ins = 0, out1s = 0, out2s = 0, out3s = 0;
    bitar_t *input;
    for ( i = 0; ; i++ ) {
      if ( (i % 2000) == 0 )
	input = (input == in) ? inn : in;
      
      wmove(win_stat_out, 1, 1);
      wprintw(win_stat_out, "Ins: %d", ins);
      wmove(win_stat_out, getcury(win_stat_out) + 1, 1);
      wprintw(win_stat_out, "L1:  %02.02f", (float)out1s/ins);
      wmove(win_stat_out, getcury(win_stat_out) + 1, 1);
      wprintw(win_stat_out, "L2:  %02.02f", (float)out2s/out1s);
      wmove(win_stat_out, getcury(win_stat_out) + 1, 1);
      wprintw(win_stat_out, "L3:  %02.02f", (float)out3s/out2s);
      wrefresh(win_stat_out);

      ins++;
      bitar_copy(l0.val, input + (i % 16));
      bitar_dumpw(win_input, VAL(&l0));

      if ( node_process(&n1, &l0, &l1) ) {
	bitar_dumpw(win_n1_out, VAL(&l1));
	out1s++;
      } else continue;

      if ( node_process(&n2, &l1, &l2) ) {
	bitar_dumpw(win_n2_out, VAL(&l2));
	out2s++;
      } else continue;

      if ( node_process(&n3, &l2, &l3) ) {
	bitar_dumpw(win_n3_out, VAL(&l3));
	out3s++;
      } else continue;
    }
  }
}
