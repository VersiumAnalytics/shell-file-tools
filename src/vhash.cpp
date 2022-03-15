#include "vhash.h"
#include "vmath.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define __HASH_CRC_32__

#define strcasecmp stricmp
#define strncasecmp strnicmp



int HashTableInit (sHashTbl *HashTbl, int PreserveKeyCase)
{
   (HashTbl -> HashTblSize) = 0;
   (HashTbl -> HashNodes) = NULL;
   (HashTbl -> PreserveKeyCase) = PreserveKeyCase;
   if (ResizeHashTable (HashTbl) != 0) {
      fprintf (stderr, "HashTableInit: Can't resize?!\n");
      return (-1);
   }
   return (0);
}

void HashTableFree (sHashTbl *HashTbl, int FreeData)
{
   int64_t i;

   if (HashTbl != NULL) {

      if ((HashTbl -> HashedKey) != NULL) free (HashTbl -> HashedKey);
      if ((HashTbl -> UniqueKey) != NULL) free (HashTbl -> UniqueKey);

      for (i = 0; i < (HashTbl -> HashTblSize); i++) {
         if ((HashTbl -> HashNodes [i]) != NULL) {
            if ((FreeData) && ((HashTbl -> HashNodes [i] -> Data) != NULL))
               free (HashTbl -> HashNodes [i] -> Data);
            free (HashTbl -> HashNodes [i] -> Key);
            free (HashTbl -> HashNodes [i]);
         }
      }
      if ((HashTbl -> HashNodes) != NULL) free (HashTbl -> HashNodes);
   }
}

int64_t ResizeHashTable (sHashTbl *HashTbl)
{
   sHashTblNode **HashNodes;
   uint64_t *UniqueKey;
   uint64_t *HashedKey;
   int64_t HashIndex;
   int64_t RehashCtr;
   int64_t HashVal;
   int64_t NewSize;
   int64_t i;

   NewSize = (HashTbl -> HashTblSize);
   if (NewSize == 0) NewSize = BLK_HASH_NODES;

   do {
      NewSize = NextPrime (NewSize * 7 / 3);
      HashedKey = (uint64_t *) malloc (sizeof (uint64_t) * NewSize);
      UniqueKey = (uint64_t *) malloc (sizeof (uint64_t) * NewSize);
      HashNodes = (sHashTblNode **) malloc (sizeof (sHashTblNode *) * NewSize);

      for (i = 0; i < NewSize; i++) {
         HashNodes [i] = NULL;
         HashedKey [i] = 0;
         UniqueKey [i] = 0;
      }

      for (i = 0; i < (HashTbl -> HashTblSize); i++) {

         if ((HashTbl -> HashNodes [i]) != NULL) {
            RehashCtr = 0;
            HashIndex = (HashTbl -> HashedKey [i]);
            while (RehashCtr < MAX_REHASH) {
               HashIndex += (RehashCtr + HashIndex);
               if (HashIndex < 0) HashIndex = -(HashIndex);
               HashVal = (HashIndex % NewSize);

               if (HashNodes [HashVal] == NULL) {
                  HashNodes [HashVal] = (HashTbl -> HashNodes [i]);
                  HashedKey [HashVal] = (HashTbl -> HashedKey [i]);
                  UniqueKey [HashVal] = (HashTbl -> UniqueKey [i]);
                  break;
               } else {

               }
               RehashCtr++;
            }
            if (RehashCtr >= MAX_REHASH) break;
         }
      }
      if (i != (HashTbl -> HashTblSize)) {
         free (HashNodes);
         free (HashedKey);
         free (UniqueKey);
      }
   } while (i != (HashTbl -> HashTblSize));

   if ((HashTbl -> HashNodes) != NULL) {
      free (HashTbl -> HashNodes);
      free (HashTbl -> HashedKey);
      free (HashTbl -> UniqueKey);
   }

   (HashTbl -> HashNodes) = HashNodes;
   (HashTbl -> HashedKey) = HashedKey;
   (HashTbl -> UniqueKey) = UniqueKey;
   (HashTbl -> HashTblSize) = NewSize;

   return (0);
}

int64_t FastHashFind (sHashTbl *HashTbl, char *Key, int Config_Verbose)
{
   uint64_t BaseHash;
   uint64_t UniqueKey;
   register int64_t HashVal;
   register int64_t HashIndex;
   register int64_t RehashCtr;
   register sHashTblNode *HN;
   char TmpKey [KEYLEN];
   char *pKey;

   if (!(HashTbl -> PreserveKeyCase)) {
      HashStrLwrCpy (TmpKey, Key, KEYLEN);
      pKey = TmpKey;
   } else pKey = Key;

   FastHash (pKey, &UniqueKey, &BaseHash);
   HashIndex = BaseHash;
   RehashCtr = 0;

   do {
      HashIndex += (RehashCtr + HashIndex);
      if (HashIndex < 0) HashIndex = -(HashIndex);
      HashVal = (HashIndex % (HashTbl -> HashTblSize));

      if (Config_Verbose == 1) {
         fprintf (stderr, "FHF: [%s] [%qd] [%qd] [%qd] hidx=[%qd]\n", pKey, UniqueKey, BaseHash, HashVal, HashIndex);
      }

      HN = (HashTbl -> HashNodes [HashVal]);

      if (HN == NULL) {
         if (Config_Verbose == 1) {
            fprintf (stderr, "FNF: return(A): -1\n");
         }
         return (-1);
      } 

      if (Config_Verbose == 1) {
         fprintf (stderr, "FNF cmp:  HK[%qd] [%qd]\n", (HashTbl -> HashedKey [HashVal]), BaseHash);
         fprintf (stderr, "FNF cmp:  UK[%qd] [%qd]\n", (HashTbl -> UniqueKey [HashVal]), UniqueKey);
         fprintf (stderr, "FNF cmp:  [%s] [%s]\n", (HashTbl -> HashNodes [HashVal] -> Key), pKey);
      }
      
      if (((HashTbl -> HashedKey [HashVal]) == BaseHash) &&
           ((HashTbl -> UniqueKey [HashVal]) == UniqueKey) &&
           (strcmp ((HashTbl -> HashNodes [HashVal] -> Key), pKey) == 0)) {
         if (Config_Verbose == 1) {
            fprintf (stderr, "FNF: return: %qd\n", HashVal);
         }
         return (HashVal);
      }

      RehashCtr++;

   } while (RehashCtr < MAX_REHASH);

   if (Config_Verbose == 1) {
      fprintf (stderr, "FNF: return(B): -1\n");
   }
   return (-1);
}

int64_t FastHashInsert (sHashTbl *HashTbl, char *Key)
{
   uint64_t BaseHash;
   uint64_t UniqueKey;
   register int64_t HashIndex;
   register int64_t HashVal;
   register int64_t RehashCtr;
   register sHashTblNode *HN;
   char TmpKey [KEYLEN];
   char *pKey;

   if (!(HashTbl -> PreserveKeyCase)) {
      HashStrLwrCpy (TmpKey, Key, KEYLEN);
      pKey = TmpKey;
   } else pKey = Key;

   FastHash (pKey, &UniqueKey, &BaseHash);
   HashIndex = BaseHash;
   RehashCtr = 0;

   do {
      do {
         HashIndex += (RehashCtr + HashIndex);
         if (HashIndex < 0) HashIndex = -(HashIndex);
         HashVal = (HashIndex % (HashTbl -> HashTblSize));

         if ((HashTbl -> HashNodes [HashVal]) == NULL)  {
            (HashTbl -> HashedKey [HashVal]) = BaseHash;
            (HashTbl -> UniqueKey [HashVal]) = UniqueKey;

            HN = (sHashTblNode *) malloc (sizeof (sHashTblNode));

            (HashTbl -> HashNodes [HashVal]) = HN;
            (HN -> Key) = strdup (pKey);
            (HN -> Data) = NULL;

            return (HashVal);

         } else if (((HashTbl -> HashedKey [HashVal]) == BaseHash) &&
                    ((HashTbl -> UniqueKey [HashVal]) == UniqueKey) &&
                     (strcmp ((HashTbl -> HashNodes [HashVal] -> Key), pKey) == 0)) {
               return (HashVal);
            }

         RehashCtr++;
      } while (RehashCtr < MAX_REHASH);

      HashIndex = BaseHash;
      RehashCtr = 0;

   } while (ResizeHashTable (HashTbl) == 0);

   fprintf (stderr, "!!!\n");

   return (-1);
}

void FastHash (char *Key, uint64_t *Hash1, uint64_t *Hash2)
{
  register uint64_t H1 = 0;
  register uint64_t H2;
  register uint64_t Tmp;

  while (*Key != '\0') {
      H1 += *Key;
      H1 += H1 << 10;
      H1 ^= H1 >> 6;
      Key++;
  }

  H2 = H1;

  H1 += H1 << 3;
  H1 ^= H1 >> 11; 
  H1 += H1 << 15;

  H2 += H2 << 15;
  H2 ^= H2 >> 7;
  H2 += H2 << 4;

  *Hash1 = H1;
  *Hash2 = H2;


}

/*
void FastHash (char *Key, uint64_t *Hash1, uint64_t *Hash2)
{
  register uint64_t H1;
  register uint64_t H2;
  register uint64_t Tmp;

  H1 = 0xffffffff;
  H2 = 0x00000000;

  while (*Key != '\0') {
    H1 = UPDC32 (*Key, H1);
    H2 <<= 4;
    H2 += (*Key);
    Tmp = (H2 & 0xf000000000000000);
    if (Tmp) {
      H2 ^= (Tmp >> 24);
      H2 ^= Tmp;
    }
    Key++;    
	}

  *Hash1 = ~H1; 
  *Hash2 = H2;
}
*/

void HashStrLwrCpy (char *d, char *s, int maxlen)
{
  while ((*s) && (--maxlen > 0)) {
	  if (isupper (*s)) *d++ = tolower (*s);
		else *d++ = *s;
		s++;
	}
	*d++ = '\0';
}

void DumpHashTable (sHashTbl *HashTbl)
{
   int64_t i;

   fprintf (stderr, "DUMP HashTbl\n");
   fprintf (stderr, "   HashTbl->HashTblSize       = %qd\n", HashTbl -> HashTblSize);
   fprintf (stderr, "   HashTbl->PreserveKeyCase   = %d\n", HashTbl -> PreserveKeyCase);
   fprintf (stderr, "  ---- NODES ----\n");

   for (i = 0; i < HashTbl -> HashTblSize; i++) {
      if ((HashTbl -> HashNodes [i]) &&
          (HashTbl -> HashNodes [i] -> Key)) {
         fprintf (stderr, "%qd: hk=[%qd] uk=[%qd] [%s]\n", 
               i,
               HashTbl -> HashedKey [i],
               HashTbl -> UniqueKey [i],
               HashTbl -> HashNodes [i] -> Key);
      }
   }
}
