#include "vstrutils.h"

#include <stdlib.h>
#include <string.h>

int Tokenize (char **Toks, char *Str, char *DelimChrs, int MaxToks)
{
    int i = 0;
    char *p;
    char *q;

    p = strpbrk(Str, "\n\r");
    if (p != NULL)
        *p = '\0';

    p = (char *)Str;

    if (*p)
    {
        do
        {
            q = strpbrk(p, DelimChrs);
            if (q != NULL)
                *q++ = '\0';

            Toks[i++] = p;
            p = q;
        } while ((p != NULL) && (i < MaxToks));
    }

    return (i);
}

int SampleLine (char * Line, int MaxLine, FILE *fp, off_t offset,  off_t len)
{
    off_t filelen;
    off_t seekpos;
    int ctr;

    seekpos = (random() << 32) ^ random(); // Makes a 64 bit random. seekpos is 64 bit, random returns 32 bit.
    seekpos %= len;
    fseeko(fp, offset + seekpos, SEEK_SET);
    fgets(Line, MaxLine, fp);
    if (feof(fp))
        fseeko(fp, seekpos, SEEK_SET);
    fgets(Line, MaxLine, fp);
    ctr = strlen(Line);
    return ctr;
}