#include "vstrutils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>


#define MAXLINE (128 * 1024)



int main (int argc, char **argv)
{
  FILE *fp;
  off_t filelen;
  int ctr = 1000; // good enough for sampling in most cases
  int i;
  int state;
  int ctr_lines = 0;
  int ctr_words = 0;
  int ctr_bytes = 0;
  char *p;
  char Line [MAXLINE + 1];

  if (argc < 2) {
    fprintf (stderr, "Usage: %s [file]\n\n", argv [0]);
    return (-1);
  }

  if (argc == 3) {
    ctr = atoi (argv [2]);
  }

  fp = fopen (argv [1], "rb");
  if (!fp) {
    fprintf (stderr, "Can't open [%s]\n", argv [1]);
    perror (NULL);
    return (-1);
  } 
  
  srandom (time(NULL) ^ getpid());

  fseeko (fp, 0, SEEK_END);
  filelen = ftello (fp);

  for (i = 0; i < ctr; i++) {
    ctr_bytes += SampleLine (Line, MAXLINE, fp, 0, filelen);
    ctr_lines++;

    state = 0;
    p = (char *) Line;
    while (*p) {
      if (state == 0) {
        if (!isspace (*p)) {
            ctr_words++;
            state = 1;
        }
      } else if (state == 1) {
        if (isspace (*p)) state = 0;
      }
      p++;
    }
  }
  fclose (fp);

  if ((ctr != 0) && (ctr_bytes != 0)) {
     printf ("%.0qd\n", (int64_t) filelen / (int64_t) (ctr_bytes / ctr));
  }

}

