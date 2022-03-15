// Copyright 2014 Versium Analytics, Inc.
// License : BSD 3 clause
#include "vstrutils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAXLINE (128 * 1024)

int main (int argc, char **argv)
{
  FILE *fp;
  off_t filelen;
  int i;
  int ctr;
  char Line [MAXLINE + 1];

  if (argc < 3) {
    fprintf (stderr, "Usage: %s [file] [#lines]\n\n", argv [0]);
    return (-1);
  }

  ctr = atoi (argv [2]);

  fp = fopen (argv [1], "rb");
  if (!fp) {
    fprintf (stderr, "Can't open [%s]\n", argv [1]);
    perror (NULL);
    return (-1);
  }

  srandom (time(NULL) ^ getpid());

  fseeko (fp, 0, SEEK_END);
  filelen = ftello (fp);
  if (filelen == 0) return 0;
  for (i = 0; i < ctr; i++) {
    SampleLine(Line, MAXLINE, fp, 0, filelen);
    printf ("%s", Line);
  }
  
  fclose (fp);
  return 0;
}