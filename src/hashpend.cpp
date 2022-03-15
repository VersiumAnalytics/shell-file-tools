// Copyright 2014 Versium Analytics, Inc.
// License : BSD 3 clause
#include "vstrutils.h"
#include "vhash.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef STRLEN
#define STRLEN (65536 * 8)
#endif

#ifndef MAX_TOKS
#define MAX_TOKS (8192)
#endif


void BuildKeyByIdx (char *d, char **Toks, int NumToks, int *IdxConcatList, int NumIdxConcatList, char *DelimTag);
void QFF_strtolower (char *s);
void QFF_strtoupper (char *s);

int main (int argc, char **argv)
{
   int i;
   int j;
   int k;
   int NumToks;
   int Config_Lowercase = 0;
   int Config_PadLen = 0;
   int Config_PadInlineLen = -1;
   int Config_InjectInline = -1;
   int Config_ReplaceInline = -1;
   int Config_ReplaceInlineIfNull = -1;
   int Config_ExcludeNoMatchOutput = -1;
   int Config_MatchOnlyOutput = -1;
   int Config_Verbose = -1;
   char Config_DelimTag [STRLEN];
   int IdxConcatList [MAX_TOKS];
   int NumIdxConcatList = 0;
   char *Toks [MAX_TOKS];
   char Line [STRLEN];
   char InjectLine [STRLEN];
   char HashTblFileName [STRLEN];
   char Key [STRLEN];
   char *NullStr = "\0";
   char *p;
   char *q;
   char *s;
   sHashTbl HT_Inject;
   FILE *fp;

   Config_DelimTag [0] = '\0';
   HashTblFileName [0] = '\0';
   Line [0] = '\0';

   if (argc < 2) {
      fprintf (stderr, "\nUsage: %s {opts} [tab-delimited-hash-table] {fields ...}\n\n", argv [0]);
      fprintf (stderr, "   tab-delimited-hash-table contains [key]|data|data|data which is appended when\n");
      fprintf (stderr, "   fields (from 0) concatenated match\n\n");
      fprintf (stderr, "   opts:  \n");
      fprintf (stderr, "       -i  x     inject inline at field x, instead of appending\n");
      fprintf (stderr, "       -pi x     pad x fields for unmatched inline injection\n");
      fprintf (stderr, "       -r  x     replace inline at field x, instead of appending\n");
      fprintf (stderr, "       -r0 x     replace field INLINE, ONLY if 0/null\n");
      fprintf (stderr, "       -l        force all keys to lowercase first\n");
      fprintf (stderr, "       -p  x     pad unmatched records with x blank fields.\n");
      fprintf (stderr, "       -d  xyz   delimit each field with xyz from input.\n");
      fprintf (stderr, "       -o0       EXCLUDE matching rows from output.\n");
      fprintf (stderr, "       -o1       INCLUDE ONLY matching rows from output.\n");
      fprintf (stderr, "       -v        verbose - debug info.\n");
      fprintf (stderr, "   data elements will be APPENDED to each input record.\n");
      fprintf (stderr, "\n\nExample w/ delimiter:\n");
      fprintf (stderr, "testme:      33706	St Petersburg	FL\n");
      fprintf (stderr, "testme.ht:   33706:St Petersburg	found\n");
      fprintf (stderr, "%% hashpend -d ':' -r 0 testme.ht 0 1 < testme\n");
      fprintf (stderr, "output: found	St Petersburg	FL\n");

      return (-1);
   }

   for (i = 1; i < argc; i++) {
      if (argv [i] [0] == '-') {
         if (strcasecmp (argv [i], "-i") == 0) {
            if (i + 1 < argc) {
               Config_InjectInline = atoi (argv [i + 1]);
               i++;
            }
         }
         else if (strcasecmp (argv [i], "-pi") == 0) {
            if (i + 1 < argc) {
               Config_PadInlineLen = atoi (argv [i + 1]);
               i++;
            }
         }
         else if (strcasecmp (argv [i], "-r") == 0) {
            if (i + 1 < argc) {
               Config_ReplaceInline = atoi (argv [i + 1]);
               i++;
            }
         }
         else if (strcasecmp (argv [i], "-r0") == 0) {
            if (i + 1 < argc) {
               Config_ReplaceInlineIfNull = atoi (argv [i + 1]);
               i++;
            }
         } 
         else if (strcasecmp (argv [i], "-o0") == 0) Config_ExcludeNoMatchOutput = 1;
         else if (strcasecmp (argv [i], "-o1") == 0) Config_MatchOnlyOutput = 1;
         else if (strcasecmp (argv [i], "-l") == 0) Config_Lowercase = 1;
         else if (strcasecmp (argv [i], "-v") == 0) Config_Verbose = 1;
         else if (strcasecmp (argv [i], "-d") == 0) {
            if (i + 1 < argc) {
               strcpy (Config_DelimTag, argv [i + 1]);
               i++;
            }
         }
         else if (strcasecmp (argv [i], "-p") == 0) {
            if (i + 1 < argc) {
               Config_PadLen = atoi (argv [i + 1]);
               i++;
            }
         }
      } else {
         // first filename, then next ones are indexes
         if (HashTblFileName [0] == '\0') strcpy (HashTblFileName, argv [i]);
         else IdxConcatList [NumIdxConcatList++] = atoi (argv [i]);
      }
   }

   HashTableInit (&HT_Inject, 0);

   fp = fopen (HashTblFileName, "r");
   if (!fp) {
      fprintf (stderr, "Can't open hash table file: [%s]\n", HashTblFileName);
      return (-2);
   } else {
      while (fgets (Line, STRLEN, fp)) {
         p = strpbrk (Line, "\t\r\n");
         if (p) {
            *p++ = '\0';
            q = strpbrk (p, "\r\n");
            if (q) *q = '\0';
            if (Config_Lowercase) QFF_strtolower (Line);
            k = FastHashFind (&HT_Inject, (char *) Line, Config_Verbose);
            if (k < 0) {
               s = strdup (p);
               k = FastHashInsert (&HT_Inject, (char *) Line);
               if (k >= 0) HT_Inject.HashNodes [k] -> Data = (void *) s;

               if (Config_Verbose == 1) {
                  printf ("IN: [%s] => [%s]\n", Line, s);
               }
            }
         }
      }
      fclose (fp);
   }

   if (Config_Verbose == 1) {
      printf ("hash table read complete!\n");
      DumpHashTable (&HT_Inject);
      printf ("----------------------\n");
   }

   while (fgets (Line, STRLEN, stdin)) {
      NumToks = Tokenize (Toks, Line, "\t", MAX_TOKS);

      BuildKeyByIdx (Key, Toks, NumToks, (int *) &IdxConcatList, NumIdxConcatList, Config_DelimTag);
      if (Config_Lowercase) QFF_strtolower (Key);

      if (Config_Verbose == 1) {
         printf ("Key = [%s]\n", Key);
      }

      k = FastHashFind (&HT_Inject, (char *) Key, Config_Verbose);

      if (Config_Verbose == 1) {
         printf (" k=[%d]\n", k);
      }

      if (k >= 0) {
         if (Config_ExcludeNoMatchOutput == -1) {
            for (i = 0; i < NumToks; i++) {
               if ((Config_InjectInline >= 0) && (Config_InjectInline == i)) {
                  fputs (Toks [i], stdout);
                  fputc ('\t', stdout);
                  fputs (((char *) HT_Inject.HashNodes [k] -> Data), stdout);
                  fputc ('\t', stdout);
               } else if ((Config_ReplaceInline >= 0) && (Config_ReplaceInline == i)) {
                  fputs (((char *) HT_Inject.HashNodes [k] -> Data), stdout);
                  fputc ('\t', stdout);
               } else if ((Config_ReplaceInlineIfNull >= 0) && (Config_ReplaceInlineIfNull == i) && ((Toks [i] [0] == '\0') || (strcmp (Toks [i], "0") == 0))) {
                  fputs (((char *) HT_Inject.HashNodes [k] -> Data), stdout);
                  fputc ('\t', stdout);
               } else {
                  fputs (Toks [i], stdout);
                  fputc ('\t', stdout);
               }
            }

            if ((Config_ReplaceInlineIfNull == -1) && 
                  (Config_ReplaceInline == -1) && 
                  (Config_InjectInline == -1)) {
               fputs (((char *) HT_Inject.HashNodes [k] -> Data), stdout);
            }
            fputc ('\n', stdout);
         }
      } else {
         if (Config_MatchOnlyOutput == -1) {
            for (i = 0; i < NumToks; i++) {
               fputs (Toks [i], stdout);
               if (i < NumToks - 1) fputc ('\t', stdout);

               if ((Config_PadInlineLen >= 0) & (Config_InjectInline == i)) {
                  for (j = 0; j < Config_PadInlineLen; j++) {
                     fputc ('\t', stdout);
                  }
               }
            }
            for (i = 0; i < Config_PadLen; i++) fputc ('\t', stdout);
            fputc ('\n', stdout);
         }
      }
   }
   return (0);
}

void BuildKeyByIdx (char *d, char **Toks, int NumToks, int *IdxConcatList, int NumIdxConcatList, char *DelimTag)
{
   int i;

   *d = '\0';

   for (i = 0; i < NumIdxConcatList; i++) {
      if ((NumIdxConcatList >= 0) && (NumIdxConcatList < NumToks)) {
         strcat (d, Toks [IdxConcatList [i]]);
         if ((i < NumIdxConcatList - 1) && (DelimTag)) strcat (d, DelimTag);
      }
   }
}

void QFF_strtolower (char *s)
{
   while (*s) {
      *s = tolower (*s);
      s++;
   }
}

void QFF_strtoupper (char *s)
{
   while (*s) {
      *s = toupper (*s);
      s++;
   }
}
