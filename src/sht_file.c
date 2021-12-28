#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bf.h"
#include "sht_file.h"
#define MAX_OPEN_FILES 20

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return HT_ERROR;        \
    }                         \
  }

#ifndef HASH_FILE_H
#define HASH_FILE_H

typedef enum HT_ErrorCode
{
  HT_OK,
  HT_ERROR
} HT_ErrorCode;

typedef struct Record
{
  int id;
  char name[15];
  char surname[20];
  char city[20];
} Record;

typedef struct
{
  char index_key[20];
  int tupleId; /*Ακέραιος που προσδιορίζει το block και τη θέση μέσα στο block στην οποία     έγινε η εισαγωγή της εγγραφής στο πρωτεύον ευρετήριο.*/
} SecondaryRecord;

#endif // HASH_FILE_H

typedef struct
{
  int size;
  int local_depth;
} SecHeader;

typedef struct
{
  SecHeader header;
  SecondaryRecord record[(BF_BLOCK_SIZE - sizeof(SecHeader)) / sizeof(SecondaryRecord)];
} SecEntry;

typedef struct
{
  int fd;
  int used;
} IndexNode;

typedef struct
{
  int size;
} SecHashHeader;

typedef struct
{
  int value;
  int block_num;
} SecHashNode;

typedef struct
{
  SecHashHeader header;
  SecHashNode hashNode[(BF_BLOCK_SIZE - sizeof(SecHashHeader)) / sizeof(SecHashNode)];
} SecHashEntry;

IndexNode indexArray[MAX_OPEN_FILES];

HT_ErrorCode SHT_Init()
{
  if (MAX_OPEN_FILES <= 0)
  {
    printf("Runner.exe needs at least one file to run. Please ensure that MAX_OPEN_FILES is not 0\n");
    return HT_ERROR;
  }

  for (int i = 0; i < MAX_OPEN_FILES; i++)
    indexArray[i].used = 0;

  return HT_OK;
}

HT_ErrorCode SHT_CreateSecondaryIndex(const char *sfileName, char *attrName, int attrLength, int depth, char *fileName)
{
  printf("This is SHT_CreateSecondaryIndex\n");
  printf("sfileName: %s, attrName: %s, attrLength: %d, depth: %d, fileName: %s\n", sfileName, attrName, attrLength, depth, fileName);
  if (sfileName == NULL || strcmp(sfileName, "") == 0)
  {
    printf("Please provide a name for the output file!\n");
    return HT_ERROR;
  }

  if (depth < 0)
  {
    printf("Depth input is wrong! Please give a NON-NEGATIVE number!\n");
    return HT_ERROR;
  }

  CALL_BF(BF_CreateFile(sfileName));
  printf("Name given : %s, max depth : %i\n", sfileName, depth);

  BF_Block *block;
  BF_Block_Init(&block);

  SecEntry empty;
  empty.header.local_depth = depth;
  empty.header.size = 0;

  int id;
  SHT_OpenSecondaryIndex(sfileName, &id);
  int sfd = indexArray[id].fd;

  SecHashEntry secHashEntry;
  int hashN = pow(2.0, (double)depth);
  secHashEntry.header.size = hashN;
  for (int i = 0; i < hashN; i++)
    secHashEntry.hashNode[i].value = i;
  // create first block for info
  printf("sfd=%d\n", sfd);
  CALL_BF(BF_AllocateBlock(sfd, block));
  printf("YOOO\n");
  char *data = BF_Block_GetData(block);
  memcpy(data, &depth, sizeof(int));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  int blockN;

  CALL_BF(BF_AllocateBlock(sfd, block));
  CALL_BF(BF_UnpinBlock(block));

  for (int i = 0; i < hashN; i++)
  {
    BF_GetBlockCounter(sfd, &blockN);
    CALL_BF(BF_AllocateBlock(sfd, block));
    data = BF_Block_GetData(block);
    memcpy(data, &empty, sizeof(SecEntry));
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));

    secHashEntry.hashNode[i].block_num = blockN;
  }

  // create second block for hashing
  BF_GetBlock(sfd, 1, block);
  data = BF_Block_GetData(block);
  memcpy(data, &secHashEntry, sizeof(SecHashEntry));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));

  BF_Block_Destroy(&block);
  printf("File was not created before\n");

  SHT_CloseSecondaryIndex(id);

  return HT_OK;
  printf("Secondary index fully created!\n");
}

HT_ErrorCode SHT_OpenSecondaryIndex(const char *sfileName, int *indexDesc)
{
  int found = 0; // bool flag.

  // find empty spot
  for (int i = 0; i < MAX_OPEN_FILES; i++)
    if (indexArray[i].used == 0)
    {
      (*indexDesc) = i;
      found = 1;
      break;
    }

  // if table is full return error
  if (found == 0)
  {
    printf("The maximum number of open files has been reached.Please close a file and try again!\n");
    return HT_ERROR;
  }
  int fd;
  CALL_BF(BF_OpenFile(sfileName, &fd));
  int pos = (*indexDesc);   // Return position
  indexArray[pos].fd = fd;  // Save fileDesc
  indexArray[pos].used = 1; // Set position to used

  printf("Secondary index opened!\n");

  return HT_OK;
}

HT_ErrorCode SHT_CloseSecondaryIndex(int indexDesc)
{
  if (indexArray[indexDesc].used == 0)
  {
    printf("Trying to close an already closed file!\n");
    return HT_ERROR;
  }

  int fd = indexArray[indexDesc].fd;

  indexArray[indexDesc].used = 0; // Free up position

  CALL_BF(BF_CloseFile(fd));

  printf("Secondary index closed!\n");

  return HT_OK;
}

HT_ErrorCode SHT_SecondaryInsertEntry(int indexDesc, SecondaryRecord record)
{
  // insert code here
  return HT_OK;
}

HT_ErrorCode SHT_SecondaryUpdateEntry(int indexDesc, UpdateRecordArray *updateArray)
{
  // insert code here
  return HT_OK;
}

HT_ErrorCode SHT_PrintAllEntries(int sindexDesc, char *index_key)
{
  // insert code here
  return HT_OK;
}

HT_ErrorCode SHT_HashStatistics(char *filename)
{
  // insert code here
  return HT_OK;
}

HT_ErrorCode SHT_InnerJoin(int sindexDesc1, int sindexDesc2, char *index_key)
{
  // insert code here
  return HT_OK;
}
