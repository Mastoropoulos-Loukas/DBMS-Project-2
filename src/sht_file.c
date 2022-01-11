#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bf.h"
#include "sht_file.h"
#include "hash_file.h"

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
  SecondaryRecord secRecord[SEC_MAX_RECORDS];
} SecEntry;

typedef struct
{
  int size;
  int next_hblock;    // το επόμενο block στο ευρετήριο
  char attribute[20]; // ο τύπος τιμών που κάνουμε hash (city or surname)
} SecHashHeader;

typedef struct
{
  int h_value;
  int block_num;
} SecHashNode;

typedef struct
{
  SecHashHeader secHeader;
  SecHashNode secHashNode[SEC_MAX_NODES];
} SecHashEntry;

SecIndexNode secIndexArray[MAX_OPEN_FILES]; // πινακας μεα τα ανοικτα αρχεια δευτερευοντος ευρετηριου

HT_ErrorCode SHT_Init()
{
  if (MAX_OPEN_FILES <= 0)
  {
    printf("Runner.exe needs at least one file to run. Please ensure that MAX_OPEN_FILES is not 0\n");
    return HT_ERROR;
  }

  for (int i = 0; i < MAX_OPEN_FILES; i++)
    secIndexArray[i].used = 0;

  return HT_OK;
}

void printSecRecord(SecondaryRecord record)
{
  printf("tupleId = %i, index_key = %s\n", record.tupleId, record.index_key);
}

/*
  checks the input for SHT_CreateSecondaryIndex
*/
HT_ErrorCode checkShtCreate(const char *sfileName, char *attrName, int attrLength, int depth, char *fileName)
{
  // printf("This is SHT_CreateSecondaryIndex\n");
  // printf("sfileName: %s, attrName: %s, attrLength: %d, depth: %d, fileName: %s\n", sfileName, attrName, attrLength, depth, fileName);
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

  if (fileName == NULL || strcmp(fileName, "") == 0)
  {
    printf("Please provide a name for the primary file!\n");
    return HT_ERROR;
  }

  if (attrName == NULL || strcmp(attrName, "") == 0)
  {
    printf("Please provide a name for the attribute!\n");
    return HT_ERROR;
  }

  if (attrLength < 1)
  {
    printf("Please provide a valid length for the attribute name!\n");
    return HT_ERROR;
  }

  return HT_OK;
}

/*
  Checks if the primary index exists.
  fileName: the name of the primary index file.
*/
HT_ErrorCode primaryExists(char *fileName)
{
  for (int i = 0; i < MAX_OPEN_FILES; i++)
    if (strcmp(indexArray[i].filename, fileName) == 0) return HT_OK;

  return HT_ERROR;
}

/*
  block: previously initialized BF_Block pointer (does not get destroyed)
  allocates and stores to the first block of the file with fileDesc 'fd', the global 'depht'
*/
HT_ErrorCode createSecInfoBlock(int sfd, BF_Block *block, int depth)
{
  CALL_BF(BF_AllocateBlock(sfd, block));
  char *data = BF_Block_GetData(block);
  memcpy(data, &depth, sizeof(int));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  return HT_OK;
}

/*
  block: previously initialized BF_Block pointer (does not get destroyed)
  allocates and stores to the second block of the file with fileDesc 'sfd', a HashTable with 2^'depht' values.
*/
HT_ErrorCode createSecHashTable(int sfd, BF_Block *block, int depth, char* attrName)
{
  SecHashEntry secHashEntry;
  int hashN = pow(2.0, (double)depth);
  secHashEntry.secHeader.next_hblock = -1;
  strcpy(secHashEntry.secHeader.attribute, attrName);
  int blockN;
  char * data;

  //allocate space for the HashTable
  CALL_BF(BF_AllocateBlock(sfd, block));
  CALL_BF(BF_UnpinBlock(block));

  //set empty entry header
  SecEntry empty;
  empty.secHeader.local_depth = depth;
  empty.secHeader.size = 0;

  //Link every hash value an empty data block
  secHashEntry.secHeader.size = hashN;
  for (int i = 0; i < hashN; i++)
  {
    secHashEntry.secHashNode[i].h_value = i;
    BF_GetBlockCounter(sfd, &blockN);
    CALL_BF(BF_AllocateBlock(sfd, block));
    data = BF_Block_GetData(block);
    memcpy(data, &empty, sizeof(SecEntry));
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    secHashEntry.secHashNode[i].block_num = blockN;
  }

  //Store HashTable
  BF_GetBlock(sfd, 1, block);
  data = BF_Block_GetData(block);
  memcpy(data, &secHashEntry, sizeof(SecHashEntry));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

HT_ErrorCode SHT_CreateSecondaryIndex(const char *sfileName, char *attrName, int attrLength, int depth, char *fileName)
{
  CALL_OR_DIE(checkShtCreate(sfileName, attrName, attrLength, depth, fileName));
  CALL_OR_DIE(primaryExists(fileName));

  CALL_BF(BF_CreateFile(sfileName));
  // printf("Name given : %s, max depth : %i\n", sfileName, depth);

  BF_Block *block;
  BF_Block_Init(&block);

  int id;
  SHT_OpenSecondaryIndex(sfileName, &id);
  int sfd = secIndexArray[id].fd;

  //create info block and sec hash table
  CALL_OR_DIE(createSecInfoBlock(sfd, block, depth));
  CALL_OR_DIE(createSecHashTable(sfd, block, depth, attrName));

  BF_Block_Destroy(&block);
  SHT_CloseSecondaryIndex(id);
  return HT_OK;
}

HT_ErrorCode SHT_OpenSecondaryIndex(const char *sfileName, int *indexDesc)
{
  // find empty spot
  int found = 0;
  for (int i = 0; i < MAX_OPEN_FILES; i++)
    if (secIndexArray[i].used == 0)
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
  int pos = (*indexDesc);      // Return position
  secIndexArray[pos].fd = fd;  // Save fileDesc
  secIndexArray[pos].used = 1; // Set position to used

  // printf("Secondary index opened!\n");

  return HT_OK;
}

HT_ErrorCode SHT_CloseSecondaryIndex(int indexDesc)
{
  if (secIndexArray[indexDesc].used == 0)
  {
    printf("Trying to close an already closed file!\n");
    return HT_ERROR;
  }

  int fd = secIndexArray[indexDesc].fd;
  secIndexArray[indexDesc].used = 0;
  CALL_BF(BF_CloseFile(fd));
  //printf("Secondary index closed!\n");
  return HT_OK;
}

/*
  block: previously initialized BF_Block pointer (does not get destroyed)
  stores the HashTable from file with fileDesc 'fd', at 'block_num' to 'hashEntry' variable.
*/
HT_ErrorCode getSecHashTable(int fd, BF_Block *block,int block_num, SecHashEntry *hashEntry)
{
  CALL_BF(BF_GetBlock(fd, block_num, block));
  char *data = BF_Block_GetData(block);
  memcpy(hashEntry, data, sizeof(SecHashEntry));
  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

/*
  Returns the block_num of the data block that hash 'value' from HashTable 'hashEntry' points to.
*/
int getSecBucket(int value, SecHashEntry hashEntry)
{
  int pos = -1;
  for (pos = 0; pos < hashEntry.secHeader.size; pos++)
    if (hashEntry.secHashNode[pos].h_value == value)
      return hashEntry.secHashNode[pos].block_num;
}

/*
  checks the input of HT_InsertEntry
*/
HT_ErrorCode checkSecInsertEntry(int indexDesc, SecondaryRecord record)
{
  if (secIndexArray[indexDesc].used == 0)
  {
    printf("Trying to insert into a closed file!\n");
    return HT_ERROR;
  }
  
  return HT_OK;
}

/*
  block: previously initialized BF_Block pointer (does not get destroyed)
  Stores the Entry from block with block_num 'bucket' at file with fileDesc 'fd', at 'entry' variable.
*/
HT_ErrorCode getSecEntry(int fd, BF_Block *block, int bucket, SecEntry *entry)
{
  BF_GetBlock(fd, bucket, block);
  char *data = BF_Block_GetData(block);
  memcpy(entry, data, sizeof(SecEntry));
  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

// /*
//   block: previously initialized BF_Block pointer (does not get destroyed).
//   fd: fileDesc of file we want.
//   Updates the value of the global depth at the disk. (calling function must update memory, if need to)
// */
// HT_ErrorCode setDepth(int fd, BF_Block *block, int depth)
// {
//   CALL_BF(BF_GetBlock(fd, 0, block));
//   char *data = BF_Block_GetData(block);
//   memcpy(data, &depth, sizeof(int));
//   BF_Block_SetDirty(block);
//   CALL_BF(BF_UnpinBlock(block));

//   return HT_OK;
// }

/*
  block: previously initialized BF_Block pointer (does not get destroyed).
  fd: fileDesc of file we want.
  Saves the HashTable 'hashEntry' at the disk at 'block_num'.
*/
HT_ErrorCode setSecHashTable(int fd, BF_Block *block, int block_num, SecHashEntry *hashEntry)
{
  CALL_BF(BF_GetBlock(fd, block_num, block));
  char *data = BF_Block_GetData(block);
  memcpy(data, hashEntry, sizeof(SecHashEntry));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  return HT_OK;
}

/*
  'hashEntry' : hash table.
  'bucket' : block we are interested in.
  'depth' : global depth of the hash table.
  'local_depth' : local depth of bucket.
   Stores at first and end the first and last index of the hash table, that points to the bucket. Stores at half the medium of [first, last].
*/
HT_ErrorCode getSecEndPoints(int *first, int *half, int *end, int local_depth, int depth, int bucket, SecHashEntry hashEntry)
{
  //int local_depth = entry.header.local_depth;
  int dif = depth - local_depth;
  int numOfHashes = pow(2.0, (double)dif);
  for (int pos = 0; pos < hashEntry.secHeader.size; pos++)
    if (hashEntry.secHashNode[pos].block_num == bucket)
    {
      *first = pos;
      break;
    }

  *half = (*first) + numOfHashes / 2 - 1;
  *end = (*first) + numOfHashes - 1;
  return HT_OK;
}

/*
  block: previously initialized BF_Block pointer (does not get destroyed).
  fd: fileDesc of file we want.
  Allocates a new block at the end of the file, and stores it's 'block_num'.
*/
// HT_ErrorCode getNewBlock(int fd, BF_Block *block, int *block_num)
// {
//   BF_GetBlockCounter(fd, block_num);
//   CALL_BF(BF_AllocateBlock(fd, block));
//   CALL_BF(BF_UnpinBlock(block));
  
//   return HT_OK;
// }

/*
  Reassigns records from one block to two. Used when need to split. !It doesn't split, it reassigns!
  The two new blocks consist of the old block and a new one that has been allocated.
  block: previously initialized BF_Block pointer (does not get destroyed).
  fd: fileDesc of file we want.
  entry: the Entry that was in the block we are reassigning from.
  old: address of the entry that will remain in the old block.
  new: address of the entry that will be in the new block.
  blockOld: block_num of old block.
  blockNew: block_num of new block.
  depth: global depth.
  half: medium of [first, end]. first is the first index of the hash table that points to the old block and end is the last.
  updateArray: the array we stores record updates.
*/
HT_ErrorCode reassignSecRecords(int fd, BF_Block *block, SecEntry entry, int blockOld, int blockNew, int half, int depth, SecEntry *old, SecEntry *new)
{
  for (int i = 0; i < entry.secHeader.size; i++)
  {
    if (hashFunction(entry.secRecord[i].tupleId, depth) <= half)
    {
      //reassign to new position in old block
      old->secRecord[old->secHeader.size] = entry.secRecord[i];
      
      //update array
      // strcpy(updateArray[i].city, entry.secRecord[i].city);
      // strcpy(updateArray[i].surname, entry.secRecord[i].surname);
      // updateArray[i].oldTupleId = getTid(blockOld, i);
      // updateArray[i].newTupleId = getTid(blockOld, old->secHeader.size);
      
      old->secHeader.size++;  
    }
    else
    {
      // assign to new block
      new->secRecord[new->secHeader.size] = entry.secRecord[i];

      //update array
      // strcpy(updateArray[i].city, entry.secRecord[i].city);
      // strcpy(updateArray[i].surname, entry.secRecord[i].surname);
      // updateArray[i].oldTupleId = getTid(blockOld, i);
      // updateArray[i].newTupleId = getTid(blockNew, new->secHeader.size);

      new->secHeader.size++;
    }
  }

  return HT_OK;
}

/*
  Inserts a record after the block it should go, was splitted.
  The two new blocks (after spliting) consist of the old block and a new one that has been allocated.
  record: the record we're inserting.
  depth: the global depth.
  half: medium of [first, end]. first is the first index of the hash table that points to the old block and end is the last.
  tupleId: the tupleId of the record after insertion.
  old: address of the entry that will remain in the old block.
  new: address of the entry that will be in the new block.
  blockOld: block_num of old block.
  blockNew: block_num of new block.
*/
HT_ErrorCode insertSecRecordAfterSplit(SecondaryRecord secondaryRecord, int depth, int half, int blockOld, int blockNew, SecEntry *old, SecEntry *new)
{
  //store given record
  if (hashFunction(secondaryRecord.tupleId, depth) <= half)
  {
    old->secRecord[old->secHeader.size] = secondaryRecord;
    // *tupleId = getTid(blockOld, old->header.size);
    old->secHeader.size++;
  }
  else
  {
    new->secRecord[new->secHeader.size] = secondaryRecord;
    // *tupleId = getTid(blockNew, new->header.size);
    new->secHeader.size++;
  }

  return HT_OK;
}

/*
  Stores given Entry at a block.
  fd: fileDesc of file we are interested in.
  block: previously initialized BF_Block pointer (does not get destroyed).
  dest_block_num: block_num of the block we are storing Entry at.
  entry: the Entry.
*/
HT_ErrorCode setSecEntry(int fd, BF_Block *block, int dest_block_num, SecEntry *entry){
  CALL_BF(BF_GetBlock(fd, dest_block_num, block));
  char *data = BF_Block_GetData(block);
  memcpy(data, entry, sizeof(SecEntry));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

/*
  block: previously initialized BF_Block pointer (does not get destroyed).
  fd: fileDesc of file we want.
  Doubles the HashTable 'hashEntry', updates memory and disk data at the end.
*/
HT_ErrorCode doubleSecHashTable(int fd, BF_Block *block, SecHashEntry *hashEntry)
{
  //double table
  SecHashEntry new = (*hashEntry);
  new.secHeader.size = (*hashEntry).secHeader.size * 2;
  for (unsigned int i = 0; i < new.secHeader.size; i++)
  {
    new.secHashNode[i].block_num = (*hashEntry).secHashNode[i >> 1].block_num;
    new.secHashNode[i].h_value = i;
  }

  //update changes in disk and memory
  CALL_OR_DIE(setSecHashTable(fd, block, 1, &new));
  (*hashEntry) = new;
  return HT_OK;
}

/*
  Splits a HashTable's block, reassigns records, and stores updated data.
  fd: fileDesc of file we are interested in.
  block: previously initialized BF_Block pointer (does not get destroyed).
  depth: global depth.
  bucket: the block_num of the block we are spliting.
  record: the record that when added caused the spliting. Inserted at the end.
  tupleId: the tupleId of the record after it is inserted.
  updateArray: the array we are storing records' updates.
  entry: the Entry of the block before it splitted.
*/
HT_ErrorCode splitSecHashTable(int fd, BF_Block *block, int depth, int bucket, SecondaryRecord record, SecEntry entry)
{
  //get HashTable
  SecHashEntry hashEntry;
  CALL_OR_DIE(getSecHashTable(fd, block, 1, &hashEntry));

  //get end points
  int local_depth = entry.secHeader.local_depth;
  int first, half, end;
  CALL_OR_DIE(getSecEndPoints(&first, &half, &end, local_depth, depth, bucket, hashEntry));

  //get a new block
  int blockNew;
  CALL_OR_DIE(getNewBlock(fd, block, &blockNew));

  SecEntry old, new;
  new.secHeader.local_depth = local_depth + 1;
  new.secHeader.size = 0;

  old = entry;
  old.secHeader.local_depth++;
  old.secHeader.size = 0;

  for (int i = half + 1; i <= end; i++)
    hashEntry.secHashNode[i].block_num = blockNew;

  //update HashTable and re-assing records
  CALL_OR_DIE(setSecHashTable(fd, block, 1, &hashEntry));
  CALL_OR_DIE(reassignSecRecords(fd, block, entry, bucket, blockNew, half, depth, &old, &new));

  //insert new record (after splitting)
  CALL_OR_DIE(insertSecRecordAfterSplit(record, depth, half, bucket, blockNew, &old, &new));

  //store created/modified entries
  CALL_OR_DIE(setSecEntry(fd, block, bucket, &old));
  CALL_OR_DIE(setSecEntry(fd, block, blockNew, &new));

  return HT_OK;
}

HT_ErrorCode SHT_SecondaryInsertEntry(int indexDesc, SecondaryRecord record)
{
  // insert code here
  // printSecRecord(record);
  CALL_OR_DIE(checkSecInsertEntry(indexDesc, record));

  //Initialize block
  BF_Block *block;
  BF_Block_Init(&block);
  
  // get depth
  int depth;
  int fd = secIndexArray[indexDesc].fd;
  CALL_OR_DIE(getDepth(fd, block, &depth));

  //get HashTable
  SecHashEntry hashEntry;  
  CALL_OR_DIE(getSecHashTable(fd, block, 1, &hashEntry));

  //get bucket
  int value = hashFunction(record.tupleId, depth);
  int blockN = getSecBucket(value, hashEntry);

  //get bucket's entry
  SecEntry entry;
  CALL_OR_DIE(getSecEntry(fd, block, blockN, &entry));

  char *data;

  // number of Records in the block
  int size = entry.secHeader.size;
  int local_depth = entry.secHeader.local_depth;

  // check for available space
  if (size >= SEC_MAX_RECORDS)
  {
    // check local depth
    if (local_depth == depth)
    {
      //double HashTable
      CALL_OR_DIE(doubleSecHashTable(fd, block, &hashEntry));
      depth++;
      CALL_OR_DIE(setDepth(fd, block, depth));
    }
    //spit hashTable's pointers
    CALL_OR_DIE(splitSecHashTable(fd, block, depth, blockN, record, entry));
    return HT_OK;
  }

  //space available
  CALL_OR_DIE(getSecEntry(fd, block, blockN, &entry));

  //insert new record (whithout splitting)
  entry.secRecord[entry.secHeader.size] = record;
  // *tupleId  = getTid(blockN, entry.header.size);
  (entry.secHeader.size)++;

  CALL_OR_DIE(setSecEntry(fd, block, blockN, &entry));
  return HT_OK;
}

HT_ErrorCode SHT_SecondaryUpdateEntry(int indexDesc, UpdateRecordArray *updateArray)
{
  // insert code here
  return HT_OK;
}


/*
  prints SecHashNodes values
*/
void SHT_PrintHashNode(SecHashNode node)
{
  printf("SecHashNode: block_num = %i, h_value=%i\n", node.block_num, node.h_value);
}

/*
  prints the contents of a SecondaryRecord
*/
void SHT_PrintSecondaryRecord(SecondaryRecord record)
{
  printf("inde_key: %s, tupleId: %i\n", record.index_key, record.tupleId);
}

/*
  Prints the contents of a SecEntry
*/
void SHT_PrintSecEntry(SecEntry entry)
{
  printf("local_depth = %i\n", entry.secHeader.local_depth);
  for(int i = 0; i < entry.secHeader.size; i++)SHT_PrintSecondaryRecord(entry.secRecord[i]);
}

/*
  Prints SecHashEntry's SecHashNodes.
  If full is given as 1, it prints the block each SecHashNode points to, also
  fd: file's fileDesc
  block: previously initialized BF_Block stracture (must be destroyed by calling function)
*/
void SHT_PrintSecHashEntry(SecHashEntry hEntry, int full, int fd, BF_Block *block)
{
  SecEntry entry;

  for(int i = 0; i < hEntry.secHeader.size; i++)
  {
    if(full)printf("\n");
    SHT_PrintHashNode(hEntry.secHashNode[i]);
    if(full == 1)
    {
      int bn = hEntry.secHashNode[i].block_num;
      printf("Secondary Entry with block_num = %i\n", bn);
      CALL_OR_DIE(getSecEntry(fd, block, bn, &entry));
      SHT_PrintSecEntry(entry);
    }
  }
}

/*
  Prints for every SecHashEntry in the Hash Table, it's SecHashNodes.
  If full is given as 1, it prints the block each SecHashNode points to, also
  fd: file's fileDesc
  block: previously initialized BF_Block stracture (must be destroyed by calling function)
*/
void SHT_PrintSecHashTable(int fd, BF_Block *block, int full)
{
  SecHashEntry table;
  int block_num;

  block_num = 1;  //get start of hash table first

  do
  {
    CALL_OR_DIE(getSecHashTable(fd, block, block_num, &table));
    block_num = table.secHeader.next_hblock;

    SHT_PrintSecHashEntry(table, full, fd, block);
  } while (block_num != -1);
}


/*
  Prints all records in a file.
  fd: the fileDesc of the file.
  block: previously initialized BF_Block pointer (does not get destroyed).
  depth: the global depth.
  hashEntry: the hash table.
*/
HT_ErrorCode printAllSecRecords(int fd, BF_Block *block, int depth, SecHashEntry hashEntry)
{
  // for (int i = 0; i < hashEntry.secHeader.size; i++)
  // {
  //   int blockN = hashEntry.secHashNode[i].block_num;
  //   printf("Records with hash value %i (block_num = %i)\n", i, blockN);
    
  //   // print all records
  //   SecEntry entry;
  //   CALL_OR_DIE(getSecEntry(fd, block, blockN, &entry));
  //   for (int i = 0; i < entry.secHeader.size; i++)
  //     printSecRecord(entry.secRecord[i]);

  //   //skip hash values that point to the same block
  //   int dif = depth - entry.secHeader.local_depth;
  //   i += pow(2.0, (double)dif) - 1;
  // }

  SHT_PrintSecHashTable(fd, block, 1);

  return HT_OK;
}

HT_ErrorCode SHT_PrintAllEntries(int sindexDesc, char *index_key)
{
  BF_Block *block;
  BF_Block_Init(&block);

  int fd = secIndexArray[sindexDesc].fd;

  // get depth
  int depth;
  CALL_OR_DIE(getDepth(fd, block, &depth));

  // get HashTable
  SecHashEntry hashEntry;
  CALL_OR_DIE(getSecHashTable(fd, block, 1, &hashEntry));

  HT_ErrorCode htCode;
  //if (id == NULL) 
    htCode = printAllSecRecords(fd, block, depth, hashEntry);
  // else htCode = printSepcificRecord(fd, block, (*id), depth, hashEntry);

  BF_Block_Destroy(&block);
  return htCode;
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

