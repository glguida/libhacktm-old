#include "fdr.h"

/* Test #0. Do random modification to an input bit array and print
   gnuplot-plottable data points for a distance-to-input vs
   distance-to-output graph. */

#define INPUT_MAPSIZE 64 
#define CONN_BITS 200
#define OUTPUT_MAPSIZE (INPUT_MAPSIZE/64)
#define OUTPUT_TRUNC 50

int
main()
{
  uint64_t i, j;
  fdr_t fdr, sparsify;
  bitar_t in, ref, out, outref;


  bitar_init(&in, INPUT_MAPSIZE);
  bitar_init(&ref, INPUT_MAPSIZE);

  bitar_init(&out, OUTPUT_MAPSIZE);
  bitar_init(&outref, OUTPUT_MAPSIZE);

  for ( i = 0; i < INPUT_MAPSIZE; i++ )
    ref.map[i] = arc4random();

  bitar_copy(&in, &ref);

  fdr_init(&fdr, INPUT_MAPSIZE, OUTPUT_MAPSIZE, CONN_BITS, OUTPUT_TRUNC);
  fdr_generate(&fdr, &ref, &outref);

  fprintf(stderr, "Iterating...");

  for ( i = 0; i < 100000; i++)
    {
      size_t bs = bitar_bitsize(&in);
      size_t id = (size_t)(arc4random() % bs);
      fdr_generate(&fdr, &in, &out);
      printf("%lu %lu\n", bitar_dist(&in, &ref), bitar_dist(&out, &outref));
      bitar_complement(&in, id);
      bitar_reset(&out);
    }
  fprintf(stderr, "done.\n");
}

