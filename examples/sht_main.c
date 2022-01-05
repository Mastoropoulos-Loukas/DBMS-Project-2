#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hash_file.h"
#include "sht_file.h"

#define RECORDS_NUM 1000 // you can change it if you want
#define GLOBAL_DEPT 2    // you can change it if you want
#define FILE_NAME "secondary.db"

const char* names[] = {
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
  "Dimitris"
};

const char* surnames[] = {
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
  "Halatsis"
};

const char* cities[] = {
  "Athens",
  "San Francisco",
  "Los Angeles",
  "Amsterdam",
  "London",
  "New York",
  "Tokyo",
  "Hong Kong",
  "Munich",
  "Miami"
};

#define CALL_OR_DIE(call)     \
  {                           \
    HT_ErrorCode code = call; \
    if (code != HT_OK) {      \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }

int main()
{
  BF_Init(LRU);

  CALL_OR_DIE(HT_Init());
  CALL_OR_DIE(HT_CreateIndex("primary.db", GLOBAL_DEPT));

  CALL_OR_DIE(SHT_Init());
  CALL_OR_DIE(SHT_CreateSecondaryIndex(FILE_NAME, "surnames", strlen("surnames"), GLOBAL_DEPT, "primary.db"));
  int sindexDesc;
  CALL_OR_DIE(SHT_OpenSecondaryIndex(FILE_NAME, &sindexDesc));

  SecondaryRecord record;
  srand(12569874);
  int r;
  printf("Insert Entries\n");
  for (int id = 0; id < RECORDS_NUM; ++id)
  {
    // create a record
    record.tupleId = id;
    r = rand() % 12;
    memcpy(record.index_key, surnames[r], strlen(surnames[r]) + 1);

    CALL_OR_DIE(SHT_SecondaryInsertEntry(sindexDesc, record));
  }

  printf("RUN PrintAllEntries\n");
  int id = rand() % RECORDS_NUM;
  char* str = "test";
  CALL_OR_DIE(SHT_PrintAllEntries(sindexDesc, str));
  // CALL_OR_DIE(HT_PrintAllEntries(indexDesc, NULL));
  //CALL_OR_DIE(HashStatistics(FILE_NAME));

  CALL_OR_DIE(SHT_CloseSecondaryIndex(sindexDesc));
  BF_Close();
}
