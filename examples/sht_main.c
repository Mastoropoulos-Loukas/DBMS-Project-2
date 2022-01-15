#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hash_file.h"
#include "sht_file.h"

#define RECORDS_NUM 10 // you can change it if you want
#define GLOBAL_DEPT 2  // you can change it if you want
#define PRIME_FILE_NAME "primary.db"
#define FILE_NAME "secondary.db"

const char *names[] = {
    "Yannis",
    "Christofos",
    "Sofia",
    "Marianna",
    "Vagelis",
    "Maria",
    "Iosif",
    "Dionisis",
    "Konstantina",
    "Theofilos",
    "Giorgos",
    "Dimitris"};

const char *surnames[] = {
    "Ioannidis",
    "Svingos",
    "Karvounari",
    "Rezkalla",
    "Nikolopoulos",
    "Berreta",
    "Koronis",
    "Gaitanis",
    "Oikonomou",
    "Mailis",
    "Michas",
    "Halatsis"};

const char *cities[] = {
    "Athens",
    "San Francisco",
    "Los Angeles",
    "Amsterdam",
    "London",
    "New York",
    "Tokyo",
    "Hong Kong",
    "Munich",
    "Miami"};

#define CALL_OR_DIE(call)     \
  {                           \
    HT_ErrorCode code = call; \
    if (code != HT_OK)        \
    {                         \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }

int main(int argc, char **argv)
{
  BF_Init(LRU);
  int indexDesc;

  CALL_OR_DIE(HT_Init());
  CALL_OR_DIE(HT_CreateIndex(PRIME_FILE_NAME, GLOBAL_DEPT));
  CALL_OR_DIE(HT_OpenIndex(PRIME_FILE_NAME, &indexDesc));

  CALL_OR_DIE(SHT_Init());
  CALL_OR_DIE(SHT_CreateSecondaryIndex(FILE_NAME, "cities", strlen("cities"), GLOBAL_DEPT, PRIME_FILE_NAME));
  int sindexDesc;
  CALL_OR_DIE(SHT_OpenSecondaryIndex(FILE_NAME, &sindexDesc));

  SecondaryRecord secr;
  Record record;
  srand(12569874);
  UpdateRecordArray update[MAX_RECORDS];
  int r1, r2, r3;
  int sum = 0;
  printf("Insert Entries\n");
  for (int id = 0; id < atoi(argv[1]); ++id)
  {
    printf("id = %i\n", id);
    tid tupleId;

    // create a record
    record.id = id;
    r1 = rand() % 12;
    memcpy(record.name, names[r1], strlen(names[r1]) + 1);
    r2 = rand() % 12;
    memcpy(record.surname, surnames[r2], strlen(surnames[r2]) + 1);
    r3 = rand() % 10;
    memcpy(record.city, cities[r3], strlen(cities[r3]) + 1);

    CALL_OR_DIE(HT_InsertEntry(indexDesc, record, &tupleId, update));
    printf("%s %s %s\n", names[r1], surnames[r2], cities[r3]);

    // create a record
    secr.tupleId = tupleId;
    memcpy(secr.index_key, cities[r3], strlen(cities[r3]) + 1);
    printUpdateArray(update);

    if (id == (atoi(argv[1]) - 2))
      SHT_PrintAllEntries(sindexDesc, "cities");

    CALL_OR_DIE(SHT_SecondaryInsertEntry(sindexDesc, secr));
    CALL_OR_DIE(SHT_SecondaryUpdateEntry(sindexDesc, update));
  }

  CALL_OR_DIE(SHT_PrintAllEntries(indexDesc, NULL));
  // SHT_CloseSecondaryIndex(sindexDesc);
  CALL_OR_DIE(SHT_HashStatistics(FILE_NAME));

  printf("---------------------------------\n");
  printf("Inner join on Athens:\n");
  CALL_OR_DIE(SHT_InnerJoin(sindexDesc, sindexDesc, "Athens"));
  printf("---------------------------------\n");
  printf("Inner join on NULL:\n");
  CALL_OR_DIE(SHT_InnerJoin(sindexDesc, sindexDesc, NULL));
  printf("---------------------------------\n");
  CALL_OR_DIE(SHT_CloseSecondaryIndex(sindexDesc));

  CALL_OR_DIE(HT_CloseFile(indexDesc));
  BF_Close();
}
