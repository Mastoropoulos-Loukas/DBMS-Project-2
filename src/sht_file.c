#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_file.h"
#define MAX_OPEN_FILES 20

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HT_ERROR;        \
  }                         \
}

#ifndef HASH_FILE_H
#define HASH_FILE_H


typedef enum HT_ErrorCode {
  HT_OK,
  HT_ERROR
} HT_ErrorCode;

typedef struct Record {
	int id;
	char name[15];
	char surname[20];
	char city[20];
} Record;



typedef struct{
char index_key[20];
int tupleId;  /*Ακέραιος που προσδιορίζει το block και τη θέση μέσα στο block στην οποία     έγινε η εισαγωγή της εγγραφής στο πρωτεύον ευρετήριο.*/ 
}SecondaryRecord;

#endif // HASH_FILE_H

typedef struct
{
  int fd;
  int used;
} SecIndexNode;


typedef struct
{
  int size;
  int local_depth;
} SecHeader;

typedef struct 
{
  SecHeader secHeader;
  SecondaryRecord secRecord[(BF_BLOCK_SIZE - sizeof(SecHeader)) / sizeof(SecondaryRecord)];
} SecEntry;


typedef struct
{
  int size;
} SecHashHeader;

typedef struct
{
  int h_value;
  int block_num;
} SecHashNode;

typedef struct
{
  SecHashHeader secHeader;
  SecHashNode secHashNode[(BF_BLOCK_SIZE - sizeof(SecHashHeader)) / sizeof(SecHashNode)];

} SecHashEntry;

SecIndexNode secIndex[MAX_OPEN_FILES];  // πινακας μεα τα ανοικτα αρχεια δευτερευοντος ευρετηριου

UpdateRecordArray updateArray[((BF_BLOCK_SIZE - sizeof(SecHeader)) / sizeof(SecondaryRecord)) + 1];

int max_secNodes;     //max_secNodes = BF_BLOCK_SIZE / sizeof(secHashNode)




HT_ErrorCode SHT_Init() {
  //insert code here
  return HT_OK;
}

HT_ErrorCode SHT_CreateSecondaryIndex(const char *sfileName, char *attrName, int attrLength, int depth,char *fileName ) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode SHT_OpenSecondaryIndex(const char *sfileName, int *indexDesc  ) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode SHT_CloseSecondaryIndex(int indexDesc) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode SHT_SecondaryInsertEntry (int indexDesc,SecondaryRecord record  ) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode SHT_SecondaryUpdateEntry (int indexDesc, UpdateRecordArray *updateArray ) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode SHT_PrintAllEntries(int sindexDesc, char *index_key ) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode SHT_HashStatistics(char *filename ) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode SHT_InnerJoin(int sindexDesc1, int sindexDesc2,  char *index_key ) {
  //insert code here
  return HT_OK;
}


