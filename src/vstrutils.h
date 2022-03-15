#include <stdio.h>

#ifdef __cplusplus //inform the compiler that these are C functions if we are using a c++ compiler
extern "C"
{
#endif

    int Tokenize(char **Toks, char *Str, char *DelimChrs, int MaxToks);
    int SampleLine(char *Line, int MaxLine, FILE *fp, off_t offset, off_t len);

#ifdef __cplusplus
}
#endif