#ifndef __HASH_H__
#define __HASH_H__

#include <stdint.h>

#define BLK_HASH_NODES  (1048576)
#define MAX_REHASH      (12)

#define HASH_FIND       (0x01)
#define HASH_INSERT     (0x02)

#ifndef KEYLEN
#define KEYLEN	(1024)
#endif /* KEYLEN */


#define UPDC32(octet, crc) (crc_32_tab[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))

typedef struct __HashTbl sHashTbl;
typedef struct __HashTblNode sHashTblNode;

struct __HashTbl {
  int64_t HashTblSize;
  int64_t PreserveKeyCase;
  uint64_t *HashedKey;
  uint64_t *UniqueKey;
  sHashTblNode **HashNodes;
};

struct __HashTblNode {
  char *Key;
  void *Data;
};

int HashTableInit (sHashTbl *HashTbl, int PreserveKeyCase);
void HashTableFree (sHashTbl *HashTbl, int FreeData);
int64_t ResizeHashTable (sHashTbl *HashTbl);
uint64_t Hash (char *Key);
int64_t FastHashFind (sHashTbl *HashTbl, char *Key, int Config_Verbose);
int64_t FastHashInsert (sHashTbl *HashTbl, char *Key);
uint64_t GenUniqueKey (char *Key);
void FastHash (char *Key, uint64_t *Hash1, uint64_t *Hash2);
void HashStrLwrCpy (char *d, char *s, int maxlen);
void DumpHashTable (sHashTbl *HashTbl);

#endif /* __HASH_H__ */