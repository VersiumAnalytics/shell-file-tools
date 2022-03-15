// Copyright 2014 Versium Analytics, Inc.
// License : BSD 3 clause
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define STRLEN (131072)

struct ProgramArgs {
  int NumSlices;
  int SliceToDump;
  int DupeHeader;
  int HeaderFile;
  char Header[STRLEN];
  char HeaderFileContents[STRLEN];
  char *InfileName;
};

int ParseArgs (struct ProgramArgs *Args, int argc, char **argv);
void GetSliceOffsets (FILE *fp, off_t SliceSize, int NumSlices, int SliceToDump, off_t *Head, off_t *Tail);
void GetSliceOffset (FILE *fp, off_t SliceSize, int NumSlices, int SliceToDump, off_t *Boundary);



int main (int argc, char **argv)
{
  FILE *fp;
  off_t SliceSize;
  off_t Head;
  off_t Tail;
  off_t ReadLen;
  off_t i;
  int c;
  int ParseStatus;
  char *p;
  char Header[STRLEN];
  struct ProgramArgs Args;
  struct stat statbuf;
  
  ParseStatus = ParseArgs (&Args, argc, argv);
  if (ParseStatus < 1)
    return (ParseStatus);

  stat(Args.InfileName, &statbuf);

  fp = fopen (Args.InfileName, "r");
  if (!fp) {
    fprintf (stderr, "Can't open [%s]\n", Args.InfileName);
	 return (0);
  }

  fgets (Header, STRLEN, fp);
  fseeko (fp, 0, SEEK_SET);

  SliceSize = statbuf.st_size / Args.NumSlices;

  GetSliceOffsets (fp, SliceSize, Args.NumSlices, Args.SliceToDump, &Head, &Tail);

  if (Args.SliceToDump == Args.NumSlices - 1) Tail = statbuf.st_size;

  if ((Args.SliceToDump != 0) && (Args.DupeHeader)) fputs (Header, stdout);

  if (Args.HeaderFile) fputs (Args.HeaderFileContents, stdout);

  ReadLen = Tail - Head;
  fseeko (fp, Head, SEEK_SET);
  for (i = 0; i < ReadLen; i++) {
    c = fgetc (fp);
	 fputc (c, stdout);
  }
}

int ParseArgs (struct ProgramArgs *Args, int argc, char **argv)
{
  FILE *fp;
  int j;
  int nr;
  char *p;

  Args -> DupeHeader = 0;
  Args -> HeaderFile = 0;

  for (j = 1; j < argc; j++)
  {
    if (strncasecmp(argv[j], "--", 2) == 0)
    {
      if (strcasecmp(argv[j], "--header") == 0)
        Args->DupeHeader = 1;
      else if (strncasecmp(argv[j], "--headerfile=", 13) == 0)
      {
        Args -> HeaderFile = 1;
        p = (char *) argv[j] + 13;
        fp = fopen(p, "r");
        if (fp)
        {
          nr = fread(&Args->HeaderFileContents, sizeof(char), (STRLEN - 1), fp);
          Args -> HeaderFileContents[nr] = '\0';
          fclose(fp);
        }
        else
        {
          fprintf(stderr, "error opening: [%s]\n", p);
          return (-2);
        }
      }
      else
      {
        printf("Unknown option: [%s]\n", argv[j]);
        return (-1);
      }
    }
    else
      break;
  }

  if (j + 3 != argc)
  {
    fprintf(stderr, "Usage: %s {opts} [infile] [num-slices] [slice-to-dump]\n", argv[0]);
    fprintf(stderr, "  {opts} :: --header - show header each slice.\n");
    fprintf(stderr, "            --headerfile=xxxx - replicate the contents of headerfile on top of each slice. max 128k\n\n");
    fprintf(stderr, "  num-slices      how many slices to make of the file\n");
    fprintf(stderr, "  slice-to-dump   which slice number to dump (0 .. num-slices - 1)\n");
    return (0);
  }

  Args -> InfileName = argv[j];
  Args -> NumSlices = atoi(argv[j + 1]);
  Args -> SliceToDump = atoi(argv[j + 2]);

  if (Args -> NumSlices <= 0)
  {
    fprintf(stderr, "num-slices must be larger than 0.\n");
    return (0);
  }

  if (Args -> SliceToDump < 0)
  {
    fprintf(stderr, "slice-to-dump must be larger than 0.\n");
    return (0);
  }

  if (Args -> SliceToDump >= Args -> NumSlices)
  {
    fprintf(stderr, "slice-to-dump must be smaller than num-slices.\n");
    return (0);
  }

  return (1);
}

void GetSliceOffsets(FILE *fp, off_t SliceSize, int NumSlices, int SliceToDump, off_t *Head, off_t *Tail)
{
  GetSliceOffset(fp, SliceSize, NumSlices, SliceToDump, Head);
  GetSliceOffset(fp, SliceSize, NumSlices, SliceToDump + 1, Tail);
}

void GetSliceOffset(FILE *fp, off_t SliceSize, int NumSlices, int SliceToDump, off_t *Boundary)
{
  int c;

  off_t SliceOffset = 0;
  off_t SliceBlockStart;

  SliceBlockStart = SliceSize * SliceToDump;

  fseeko(fp, SliceBlockStart, SEEK_SET);
  if (SliceBlockStart > 0)
  {
    while (!feof(fp))
    {
      c = fgetc(fp);
      SliceOffset++;
      if (c == '\n')
        break;
    }
  }

  *Boundary = (SliceBlockStart + SliceOffset);
}