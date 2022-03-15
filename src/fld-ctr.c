#include "vstrutils.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_TOKS	(8192)
#define STRLEN		(262144 * 8)

int main (int argc, char **argv)
{
  int i;
  int NumToks;
  char *Toks [MAX_TOKS];
  char Line [STRLEN];
  char *p1;
  int FieldCtrStart = 0;

  Line [0] = '\0';

  if (argc < 2) {
    fprintf (stderr, "Usage: %s [delim-chars] {start-idx} < infile > output\n", argv [0]);
    return (0);
  }

  if (argc == 3) {
     FieldCtrStart = atoi (argv [2]);
  }

  while (fgets (Line, STRLEN, stdin)) {
    NumToks = Tokenize (Toks, Line, argv [1], (int) MAX_TOKS);
    printf ("%d:", NumToks);
    for (i = 0; i < NumToks; i++) {
       printf ("(%d:%s)\t", FieldCtrStart + i, Toks [i]);
    }
    printf ("\n");
  }
  return (0);
}

