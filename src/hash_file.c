#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bf.h"
#include "hash_file.h"
#include "sht_file.h"

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return HT_ERROR;        \
    }                         \
  }


typedef struct
{
  int size;
} HashHeader;

typedef struct
{
  DataHeader header;
  Record record[MAX_RECORDS];
} Entry;

typedef struct
{
  int value;
  int block_num;
} HashNode;

typedef struct
{
  HashHeader header;
  HashNode hashNode[MAX_HNODES];
} HashEntry;


tid getTid(int blockId, int index){
  int num_of_rec_in_block = (BF_BLOCK_SIZE - sizeof(DataHeader)) / sizeof(Record);
  tid temp = (blockId+1)*num_of_rec_in_block + index;
  return temp;
}

void printUpdateArray(UpdateRecordArray *array, int size){
    for(int i = 0; i < MAX_RECORDS; i++)printf("city: %s, surname: %s, oldTid: %i, newTid: %i\n",\
    array[i].city, array[i].surname, array[i].oldTupleId, array[i].newTupleId);
}

void printRecord(Record record)
{
  printf("Entry with id : %i, city : %s, name : %s, and surname : %s\n",
         record.id, record.city, record.name, record.surname);
}

void printHashNode(HashNode node)
{
  printf("HashNode with value : %i, and block_num : %i\n", node.value, node.block_num);
}

unsigned int hashFunction(int id, int depth)
{

  char *p = (char *)&id;
  unsigned int h = 0x811c9dc5;
  int i;

  for (i = 0; i < sizeof(int); i++)
    h = (h ^ p[i]) * 0x01000193;
  h = h >> (32 - depth);

  return h;
}

HT_ErrorCode HT_Init()
{
  if (MAX_OPEN_FILES <= 0)
  {
    printf("Runner.exe needs at least one file to run. Please ensure that MAX_OPEN_FILES is not 0\n");
    return HT_ERROR;
  }
  for (int i = 0; i < MAX_OPEN_FILES; i++)
  {
    indexArray[i].used = 0;
  }
  return HT_OK;
}

HT_ErrorCode checkCreateIndex(const char* file, int depth)
{
  if (file == NULL || strcmp(file, "") == 0)
  {
    printf("Please provide a name for the output file!\n");
    return HT_ERROR;
  }
  if (depth < 0)
  {
    printf("Depth input is wrong! Please give a NON-NEGATIVE number!\n");
    return HT_ERROR;
  }
  return HT_OK;
}

HT_ErrorCode createInfoBlock(int fd, BF_Block *block, int depth)
{
  CALL_BF(BF_AllocateBlock(fd, block));
  char *data = BF_Block_GetData(block);
  memcpy(data, &depth, sizeof(int));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  return HT_OK;
}

HT_ErrorCode createHashTable(int fd, BF_Block *block, int depth)
{
  HashEntry hashEntry;
  int hashN = pow(2.0, (double)depth);
  int blockN;
  char * data;

  //allocate space for the HashTable
  CALL_BF(BF_AllocateBlock(fd, block));
  CALL_BF(BF_UnpinBlock(block));

  //set empty entry header
  Entry empty;
  empty.header.local_depth = depth;
  empty.header.size = 0;

  //Link every hash value an empty data block
  hashEntry.header.size = hashN;
  for (int i = 0; i < hashN; i++)
  {
    hashEntry.hashNode[i].value = i;
    BF_GetBlockCounter(fd, &blockN);
    CALL_BF(BF_AllocateBlock(fd, block));
    data = BF_Block_GetData(block);
    memcpy(data, &empty, sizeof(Entry));
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    hashEntry.hashNode[i].block_num = blockN;
  }

  //Store HashTable
  BF_GetBlock(fd, 1, block);
  data = BF_Block_GetData(block);
  memcpy(data, &hashEntry, sizeof(HashEntry));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth)
{
  CALL_OR_DIE(checkCreateIndex(filename, depth));
  CALL_BF(BF_CreateFile(filename));
  printf("Name given : %s, max depth : %i\n", filename, depth);

  //initialize block
  BF_Block *block;
  BF_Block_Init(&block);

  //open file
  int id;
  HT_OpenIndex(filename, &id);
  int fd = indexArray[id].fd;
  strcpy(indexArray[id].filename, filename);

  //Create Info block and HashTable
  CALL_OR_DIE(createInfoBlock(fd, block, depth));
  CALL_OR_DIE(createHashTable(fd, block, depth));

  //destroy block
  BF_Block_Destroy(&block);
  printf("File was not created before\n");
  HT_CloseFile(id);

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc)
{
  int found = 0; // bool flag.

  // find empty spot
  for (int i = 0; i < MAX_OPEN_FILES; i++)
  {
    if (indexArray[i].used == 0)
    {
      (*indexDesc) = i;
      found = 1;
      break;
    }
  }
  
  // if table is full return error
  if (found == 0)
  {
    printf("The maximum number of open files has been reached.Please close a file and try again!\n");
    return HT_ERROR;
  }

  int fd;
  CALL_BF(BF_OpenFile(fileName, &fd));
  int pos = (*indexDesc);   // Get position
  indexArray[pos].fd = fd;  // Store fileDesc
  indexArray[pos].used = 1; // Set position to used

  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc)
{
  if (indexArray[indexDesc].used == 0)
  {
    printf("Trying to close an already closed file!\n");
    return HT_ERROR;
  }

  int fd = indexArray[indexDesc].fd;
  indexArray[indexDesc].used = 0; // Free up position

  CALL_BF(BF_CloseFile(fd));
  return HT_OK;
}

HT_ErrorCode getDepth(int fd, BF_Block *block, int *depth)
{
  CALL_BF(BF_GetBlock(fd, 0, block));
  char *data = BF_Block_GetData(block);
  memcpy(depth, data, sizeof(int));
  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

HT_ErrorCode getHashTable(int fd, BF_Block *block, HashEntry *hashEntry)
{
  CALL_BF(BF_GetBlock(fd, 1, block));
  char *data = BF_Block_GetData(block);
  memcpy(hashEntry, data, sizeof(HashEntry));
  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

int getBucket(int value, HashEntry hashEntry)
{
  int pos = -1;
  for (pos = 0; pos < hashEntry.header.size; pos++)
    if (hashEntry.hashNode[pos].value == value)
      return hashEntry.hashNode[pos].block_num;
}

HT_ErrorCode checkInsertEntry(int indexDesc, UpdateRecordArray *updateArray)
{
  if (indexArray[indexDesc].used == 0)
  {
    printf("Trying to insert into a closed file!\n");
    return HT_ERROR;
  }
  if (updateArray == NULL)
  {
    printf("updateArray is NULL\n");
    return HT_ERROR;
  }

  return HT_OK;
}

HT_ErrorCode getEntry(int fd, BF_Block *block, int bucket, Entry *entry)
{
  BF_GetBlock(fd, bucket, block);
  char *data = BF_Block_GetData(block);
  memcpy(entry, data, sizeof(Entry));
  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

HT_ErrorCode doubleHashTable(int fd, BF_Block *block, HashEntry *hashEntry)
{
  //double table
  HashEntry new = (*hashEntry);
  new.header.size = (*hashEntry).header.size * 2;
  for (unsigned int i = 0; i < new.header.size; i++)
  {
    new.hashNode[i].block_num = (*hashEntry).hashNode[i >> 1].block_num;
    new.hashNode[i].value = i;
  }

  //store changes
  CALL_BF(BF_GetBlock(fd, 1, block));
  char *data = BF_Block_GetData(block);
  memcpy(data, &new, sizeof(HashEntry));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));

  //update changes to memory
  (*hashEntry) = new;
  return HT_OK;
}

HT_ErrorCode setDepth(int fd, BF_Block *block, int depth)
{
  CALL_BF(BF_GetBlock(fd, 0, block));
  char *data = BF_Block_GetData(block);
  memcpy(data, &depth, sizeof(int));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

HT_ErrorCode setHashTable(int fd, BF_Block *block, HashEntry *hashEntry)
{
  CALL_BF(BF_GetBlock(fd, 1, block));
  char *data = BF_Block_GetData(block);
  memcpy(data, hashEntry, sizeof(HashEntry));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  return HT_OK;
}

HT_ErrorCode getEndPoints(int *first, int *half, int *end, int local_depth, int depth, int blockN, HashEntry hashEntry)
{
  //int local_depth = entry.header.local_depth;
  int dif = depth - local_depth;
  int numOfHashes = pow(2.0, (double)dif);
  for (int pos = 0; pos < hashEntry.header.size; pos++)
    if (hashEntry.hashNode[pos].block_num == blockN)
    {
      *first = pos;
      break;
    }

  *half = (*first) + numOfHashes / 2 - 1;
  *end = (*first) + numOfHashes - 1;
  return HT_OK;
}

HT_ErrorCode getNewBlock(int fd, BF_Block *block, int *block_num)
{
  BF_GetBlockCounter(fd, block_num);
  CALL_BF(BF_AllocateBlock(fd, block));
  CALL_BF(BF_UnpinBlock(block));
  
  return HT_OK;
}

HT_ErrorCode splitHashTable(int fd, BF_Block *block, int depth, int blockN, Record record, tid *tupleId, UpdateRecordArray *updateArray , Entry entry)
{
  //get HashTable
  HashEntry hashEntry;
  CALL_OR_DIE(getHashTable(fd, block, &hashEntry));

  //get end points
  int local_depth = entry.header.local_depth;
  int first, half, end;
  CALL_OR_DIE(getEndPoints(&first, &half, &end, local_depth, depth, blockN, hashEntry));

  //get a new block
  int blockNew;
  CALL_OR_DIE(getNewBlock(fd, block, &blockNew));

  Entry old, new;
  new.header.local_depth = local_depth + 1;
  new.header.size = 0;

  old = entry;
  old.header.local_depth++;
  old.header.size = 0;

  for (int i = half + 1; i <= end; i++)
    hashEntry.hashNode[i].block_num = blockNew;

  CALL_OR_DIE(setHashTable(fd, block, &hashEntry));

  for (int i = 0; i < entry.header.size; i++)
  {
    if (hashFunction(entry.record[i].id, depth) <= half)
    {
      //reassign to new position in old block
      old.record[old.header.size] = entry.record[i];
      
      //update array
      strcpy(updateArray[i].city, entry.record[i].city);
      strcpy(updateArray[i].surname, entry.record[i].surname);
      updateArray[i].oldTupleId = getTid(blockN, i);
      updateArray[i].newTupleId = getTid(blockN, old.header.size);
      
      old.header.size++;  
    }
    else
    {
      // assign to new block
      new.record[new.header.size] = entry.record[i];

      //update array
      strcpy(updateArray[i].city, entry.record[i].city);
      strcpy(updateArray[i].surname, entry.record[i].surname);
      updateArray[i].oldTupleId = getTid(blockN, i);
      updateArray[i].newTupleId = getTid(blockNew, new.header.size);

      new.header.size++;
    }
  }

  //store given record
  if (hashFunction(record.id, depth) <= half)
  {
    old.record[old.header.size] = record;
    *tupleId = getTid(blockN, old.header.size);
    old.header.size++;
  }
  else
  {
    new.record[new.header.size] = record;
    *tupleId = getTid(blockNew, new.header.size);
    new.header.size++;
  }

  // write old
  CALL_BF(BF_GetBlock(fd, blockN, block));
  char *data = BF_Block_GetData(block);
  memcpy(data, &old, sizeof(Entry));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));

  // write new
  CALL_BF(BF_GetBlock(fd, blockNew, block));
  data = BF_Block_GetData(block);
  memcpy(data, &new, sizeof(Entry));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record, tid* tupleId, UpdateRecordArray* updateArray)
{
  printRecord(record);
  CALL_OR_DIE(checkInsertEntry(indexDesc, updateArray));

  //Initialize block
  BF_Block *block;
  BF_Block_Init(&block);
  
  int new = 0; // bool flag

  // get depth
  int depth;
  int fd = indexArray[indexDesc].fd;
  CALL_OR_DIE(getDepth(fd, block, &depth));

  //get HashTable
  HashEntry hashEntry;  
  CALL_OR_DIE(getHashTable(fd, block, &hashEntry));

  //get bucket
  int value = hashFunction(record.id, depth);
  int blockN = getBucket(value, hashEntry);

  //get bucket's entry
  Entry entry;
  CALL_OR_DIE(getEntry(fd, block, blockN, &entry));

  char *data;

  // number of Records in the block
  int size = entry.header.size;
  int local_depth = entry.header.local_depth;

  // check for available space
  if (size >= MAX_RECORDS)
  {
    // space needs to be allocated
    if (local_depth == depth)
    {
      CALL_OR_DIE(doubleHashTable(fd, block, &hashEntry));
      depth++;
      CALL_OR_DIE(setDepth(fd, block, depth));
    }
    
    CALL_OR_DIE(splitHashTable(fd, block, depth, blockN, record, tupleId, updateArray, entry));
    return HT_OK;
  }

  CALL_BF(BF_GetBlock(fd, blockN, block));

  // write new info
  data = BF_Block_GetData(block);
  if (new == 0)
    memcpy(&entry, data, sizeof(Entry));    // if space was previously allocated, get previous data
  entry.record[entry.header.size] = record; // add record
  *tupleId  = getTid(blockN, entry.header.size);
  (entry.header.size)++;                    // update header size

  memcpy(data, &entry, sizeof(Entry));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id)
{

  BF_Block *block;
  BF_Block_Init(&block);

  if (indexArray[indexDesc].used == 0)
  {
    printf("Can't print from a closed file!\n");
    return HT_ERROR;
  }

  int fd = indexArray[indexDesc].fd;
  int blockN;

  Entry entry;
  HashEntry hashEntry;

  // get depth
  int depth;

  CALL_BF(BF_GetBlock(fd, 0, block));
  char *data = BF_Block_GetData(block);
  memcpy(&depth, data, sizeof(int));
  CALL_BF(BF_UnpinBlock(block));

  // number of hash values
  int hashN = pow(2.0, (double)depth);

  // get hashNodes
  CALL_BF(BF_GetBlock(fd, 1, block));
  data = BF_Block_GetData(block);
  memcpy(&hashEntry, data, sizeof(HashEntry));
  CALL_BF(BF_UnpinBlock(block));

  // if id == NULL -> print all entries
  if (id == NULL)
  {

    printf("NULL id was given. Printing all entries\n");
    // for every hash value
    for (int i = 0; i < hashEntry.header.size; i++)
    {

      int blockN = hashEntry.hashNode[i].block_num;
      printf("Records with hash value %i (block_num = %i)\n", i, blockN);
      // get data block

      CALL_BF(BF_GetBlock(fd, blockN, block));
      data = BF_Block_GetData(block);
      memcpy(&entry, data, sizeof(Entry));

      // print all records
      for (int i = 0; i < entry.header.size; i++)
        printRecord(entry.record[i]);

      CALL_BF(BF_UnpinBlock(block));

      int dif = depth - entry.header.local_depth;
      i += pow(2.0, (double)dif) - 1;
    }

    BF_Block_Destroy(&block);
    return HT_OK;
  }

  // id != NULL. Print specific entry
  int value = hashFunction((*id), depth);

  // find data block_num
  int pos;
  for (pos = 0; pos < hashN; pos++)
    if (hashEntry.hashNode[pos].value == value)
      break;
  blockN = hashEntry.hashNode[pos].block_num;

  if (blockN == 0)
  {
    printf("Data block was not allocated\n");
    return HT_ERROR;
  }

  CALL_BF(BF_GetBlock(fd, blockN, block));
  data = BF_Block_GetData(block);
  memcpy(&entry, data, sizeof(Entry));

  // print record with that id
  for (int i = 0; i < entry.header.size; i++)
    if (entry.record[i].id == (*id))
      printRecord(entry.record[i]);

  CALL_BF(BF_UnpinBlock(block));

  BF_Block_Destroy(&block);

  return HT_OK;
}

HT_ErrorCode HashStatistics(char *filename)
{
  int id;
  HT_OpenIndex(filename, &id);
  int fd = indexArray[id].fd;
  HashEntry hashEntry;
  BF_Block *block;
  BF_Block_Init(&block);

  // get number of blocks
  int nblocks;
  CALL_BF(BF_GetBlockCounter(fd, &nblocks));
  printf("File %s has %d blocks.\n", filename, nblocks);

  int depth;
  CALL_BF(BF_GetBlock(fd, 0, block));
  char *data = BF_Block_GetData(block);
  memcpy(&depth, data, sizeof(int));
  CALL_BF(BF_UnpinBlock(block));

  CALL_BF(BF_GetBlock(fd, 1, block));
  data = BF_Block_GetData(block);
  memcpy(&hashEntry, data, sizeof(HashEntry));
  CALL_BF(BF_UnpinBlock(block));

  int iter = hashEntry.header.size;
  int dataN = iter;
  int min, max, total;
  max = total = 0;
  min = -1;
  for (int i = 0; i < iter; i++)
  {

    Entry entry;

    int blockN = hashEntry.hashNode[i].block_num;

    CALL_BF(BF_GetBlock(fd, blockN, block));
    data = BF_Block_GetData(block);
    memcpy(&entry, data, sizeof(Entry));
    CALL_BF(BF_UnpinBlock(block));

    int num = entry.header.size;
    // add to total
    total += num;

    // check for max
    if (num > max)
      max = num;

    // check for min
    if (num < min || min == -1)
      min = num;

    int dif = depth - entry.header.local_depth;
    i += pow(2.0, (double)dif) - 1;
    dataN -= pow(2.0, (double)dif) - 1;
  }

  printf("Max number of records in bucket is %i\n", max);
  printf("Min number of records in bucket is %i\n", min);
  printf("Mean number of records in bucket is %f\n", (double)total / (double)dataN);

  BF_Block_Destroy(&block);
  HT_CloseFile(id);

  return HT_OK;
}