#include "fdr.h"

int
main()
{
  bitar_t ones, zeros, tmp, large;
  bitar_init(&ones, 3);
  bitar_setall(&ones);

  bitar_init(&zeros, 3);
  bitar_reset(&zeros);

  bitar_init(&tmp, 3);

  bitar_init(&large, 9);
  bitar_reset(&large);
  bitar_putsub(&large, 0, &ones);

  bitar_getsub(&tmp, &large, 0);
  assert(bitar_dist(&tmp, &ones) == 0);
  bitar_getsub(&tmp, &large, 3);
  assert(bitar_dist(&tmp, &zeros) == 0);
  bitar_getsub(&tmp, &large, 6);
  assert(bitar_dist(&tmp, &zeros) == 0);
  bitar_dump(&large);

  bitar_reset(&large);
  bitar_putsub(&large, 3, &ones);

  bitar_getsub(&tmp, &large, 0);
  assert(bitar_dist(&tmp, &zeros) == 0);
  bitar_getsub(&tmp, &large, 3);
  assert(bitar_dist(&tmp, &ones) == 0);
  bitar_getsub(&tmp, &large, 6);
  assert(bitar_dist(&tmp, &zeros) == 0);
  bitar_dump(&large);

  bitar_reset(&large);
  bitar_putsub(&large, 6, &ones);

  bitar_getsub(&tmp, &large, 0);
  assert(bitar_dist(&tmp, &zeros) == 0);
  bitar_getsub(&tmp, &large, 3);
  assert(bitar_dist(&tmp, &zeros) == 0);
  bitar_getsub(&tmp, &large, 6);
  assert(bitar_dist(&tmp, &ones) == 0);
  bitar_dump(&large);

  printf("Survived. OK\n");
}
