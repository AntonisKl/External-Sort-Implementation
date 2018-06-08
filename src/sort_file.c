#include "sort_file.h"
#include "bf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRY_SIZE 55*sizeof(char) + sizeof(int) + 1

SR_ErrorCode SR_Init() {
  // Your code goes here

  return SR_OK;
}

SR_ErrorCode SR_CreateFile(const char *fileName) {

  if (BF_CreateFile(filename) != BF_OK)
    return SR_ERROR;

  int fileDesc;
  BF_Block* block = malloc(BF_BLOCK_SIZE);
  char* blockData;

  if (BF_OpenFile(filename, &fileDesc) != BF_OK)
    return SR_ERROR;

  // create the first identifying block of the sort file
  if (BF_AllocateBlock(fileDesc, block) != BF_OK)
    return SR_ERROR;

  blockData = BF_Block_GetData(block);

  strcpy(blockData, "This is a sort file");

  BF_Block_SetDirty(block);

  if (BF_UnpinBlock(block) != BF_OK)
    return SR_ERROR;

  BF_CloseFile(fileDesc);

  // free allocated memory
  BF_Block_Destroy(&block);

  return SR_OK;
}

SR_ErrorCode SR_OpenFile(const char *fileName, int *fileDesc) {

  if (BF_OpenFile(fileName, fileDesc) != BF_OK)
    return SR_ERROR;

  BF_Block* block = malloc(BF_BLOCK_SIZE);
  char* blockData;

  // get the first block and check if the file is an sort file
  if (BF_GetBlock(*fileDesc, 0, block) != BF_OK)
    return SR_ERROR;

  blockData = BF_Block_GetData(block);

  if (BF_UnpinBlock(block) != BF_OK)
    return SR_ERROR;

  if (strcmp(blockData, "This is a sort file") != 0)
  {
    printf("Not a sort file\n");
    return SR_ERROR;
  }

  // free allocated memory
  BF_Block_Destroy(&block);

  return SR_OK;
}

SR_ErrorCode SR_CloseFile(int fileDesc) {

  if (BF_CloseFile(fileDesc) != BF_OK)
    return SR_ERROR;

  return SR_OK;
}

SR_ErrorCode SR_InsertEntry(int fileDesc,	Record record) {

  BF_Block* block = malloc(BF_BLOCK_SIZE);
  char* blockData;
  int blocksNum;
  char* temp = malloc(sizeof(int) + 50*sizeof(char) + 1); // temp: the string that contains the entry to be inserted in the sort file

  char* s = malloc(sizeof(int) + 1);
  sprintf(s, "%d", record.id);

  // create the entry to be inserted in the sort file
  strcpy(temp, s);
  strcat(temp, "$"); // field starting/ending point identifier
  strcat(temp, record.name);
  strcat(temp, "$");
  strcat(temp, record.surname);
  strcat(temp, "$");
  strcat(temp, record.city);
  strcat(temp, "&"); // entry starting/ending point identifier

  if (BF_GetBlockCounter(fileDesc, &blocksNum) != BF_OK)
    return SR_ERROR;


  if (blocksNum > 1) // entries have been added to the file previously
  {
    if (BF_GetBlock(fileDesc, blocksNum - 1 , block) != BF_OK)
      return SR_ERROR;

    blockData = BF_Block_GetData(block);

    if (BF_BLOCK_SIZE - strlen(blockData) >= strlen(temp)) // entry can fit in the block
      strcat(blockData, temp); // add entry to the block
    else
    {
      // unpin the full block
      if (BF_UnpinBlock(block) != BF_OK)
        return SR_ERROR;

      // create a new empty block
      if (BF_AllocateBlock(fileDesc, block) != BF_OK)
        return SR_ERROR;

      blockData = BF_Block_GetData(block);

      strcpy(blockData, temp);
    }
  }
  else // the file does not contain any entries
  {
    if (BF_AllocateBlock(fileDesc, block) != BF_OK)
      return SR_ERROR;

    blockData = BF_Block_GetData(block);

    strcpy(blockData, temp);
  }

  // blockData were modified
  BF_Block_SetDirty(block);

  if (BF_UnpinBlock(block) != BF_OK)
    return SR_ERROR;

  // free allocated memory
  BF_Block_Destroy(&block);
  free(temp);
  free(s);

  return SR_OK;
}

void sort(Record **records, int n, int fieldNo)
{
    Record *temp;
    for (int i = 0; i < n; i++)
    {
        for (int j = n - 1; j > i; j--)
        {
            if ((fieldNo == 0 && records[i]->id > records[j]->id) || (fieldNo == 1 && strcmp(records[i]->name, records[j]->name) > 0)
             || (fieldNo == 2 && strcmp(records[i]->surname, records[j]->surname) > 0) || (fieldNo == 3 && strcmp(records[i]->city, records[j]->city) > 0))
             {
                 // swap
                 temp = records[i];
                 records[i] = records[j];
                 records[j] = temp;
             }
        }
    }
}

Record entryToRecord(char* entry)
{
  char* entryCopy = malloc(MAX_ENTRY_SIZE);
  strcpy(entryCopy, entry);

  blockDataTokens[0] = strtok(entry, "$");
  if (blockDataTokens[0] != NULL)
  {
  Record record;
  record.id = atoi(blockDataTokens[0]);
  for (int k = 1; k < 4;k++)
     blockDataTokens[k] = strtok(NULL,"$");

   strcpy(record.name, blockDataTokens[1]);
   strcpy(record.surname, blockDataTokens[2]);
   strcpy(record.city, blockDataTokens[3]);
  }

  free(entryCopy);
  return record;
}

int compareRecords(Record record1, Record record2, int fieldNo)
{
  if (fieldNo == 0 && record1.id > record2.id)
    return 1;
  else if (fieldNo == 0 && record1.id == record2.id)
    return 0;
  else if (fieldNo == 0 && record1.id < record2.id)
    return -1;
  else if (fieldNo == 1)
    return strcmp(record1.name, record2.name);
  else if (fieldNo == 2)
    return strcmp(record1.surname, record2.surname);
  else
    return strcmp(record1.city, record2.city);
}

int allEntriesNull(char** currEntries, int start, int end)
{
  int count = 0;
  for (int i = start; i < end; i++)
  {
    if (currEntries[i] == NULL)
      count++;
    else
      break;
  }
  if (count == n)
    return 1;
  else
    return 0;
}

char* findMinCurrEntry(char** currEntries, int start, int currEntriesNum, BF_Block** blocks, int* currBlockIds /*for multiple files only*/, int* inputFileDescs /*for multiple files only*/, int* bytesForNextEntry, int singleFileCallFlag)
{
  Record records[currEntriesNum];

  for (int i = 0; i < currEntriesNum; i++)
    records[i] = entryToRecord(currEntries[i + start]);

  Record* minRecord = &records[0];
  int minRecordIndex = 0;

  for (int i = 1; i < currEntriesNum; i++)
  {
    if (compareRecords(*minRecord, records[i]) > 0)
    {
      minRecord = &records[i];
      minRecordIndex = i;
    }
  }

  char* minEntry = currEntries[minRecordIndex + start];

  if (singleFileCallFlag == 1)
  {
    // if singleFileCallFlag == 1, then start = 0 so there is no need to include it
    char* blockData = BF_Block_GetData(blocks[minRecordIndex]);
    char* blockDataCopy = malloc(BF_BLOCK_SIZE);
    strcpy(blockDataCopy, blockData);

    currEntries[minRecordIndex] = strtok(blockDataCopy + bytesForNextEntry[minRecordIndex], "&");
    bytesForNextEntry[minRecordIndex] += strlen(minEntry) + 1;

    free(blockDataCopy);
  }
  else /*singleFileCallFlag == 0*/
  {
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// HERE ADD CODE FOR MULTIPLE FILES
  }

  return minEntry;
}

SR_ErrorCode SR_SortedFile(
  const char* input_filename,
  const char* output_filename,
  int fieldNo,
  int bufferSize
) {

  int fileDesc, blocksNum;

  if (BF_OpenFile(input_filename, &fileDesc) != BF_OK)
    return SR_ERROR;

  if (BF_GetBlockCounter(fileDesc, &blocksNum) != BF_OK)
    return SR_ERROR;

  char* blockDataEntries[40], blockDataTokens[4], blockData;
  char* blockDataCopy = malloc(BF_BLOCK_SIZE);


  BF_Block* block;
  BF_Block_Init(&block);

  // sort every block individually
  for (int i = 0; i < blocksNum; i++)
  {
    Record* records[40];

    if (BF_GetBlock(fileDesc, i, block) != BF_OK)
      return SR_ERROR;

    blockData = BF_Block_GetData(block);

    strcpy(blockDataCopy, blockData);

    blockDataEntries[0] = strtok(blockDataCopy, "&");

    int j = 1;
    while (blockDataEntries[j-1] != NULL && j < 40)
    {
      blockDataEntries[j] = strtok(NULL, "&");
      j++;
    }

    j = 0;
    while (blockDataEntries[j] != NULL && j < 40)
    {
      blockDataTokens[0] = strtok(blockDataEntries[j], "$");
      if (blockDataTokens[0] != NULL)
      {
        records[j] = malloc(MAX_ENTRY_SIZE);
        records[j]->id = atoi(blockDataTokens[0]);
        for (int k = 1; k < 4;k++)
           blockDataTokens[k] = strtok(NULL,"$");

        strcpy(records[j]->name, blockDataTokens[1]);
        strcpy(records[j]->surname, blockDataTokens[2]);
        strcpy(records[j]->city, blockDataTokens[3]);
      }
      j++;
    }

    int recordsNum = 0;

    j = 0;
    while (records[i] != NULL)
    {
      recordsNum++;
      j++;
    }

    sort(records, recordsNum, fieldNo);

    strcpy(blockData, ""); // empty block
    char* temp = malloc(sizeof(int) + 50*sizeof(char) + 1); // temp: the string that contains the entry to be inserted in the sort file
    char* s = malloc(sizeof(int) + 1);

    for (j = 0; j < recordsNum; j ++)
    {
      sprintf(s, "%d", records[j]->id);

      // create the entry to be inserted in the sort file
      strcpy(temp, s);
      strcat(temp, "$"); // field starting/ending point identif6:45 / 14:03ier
      strcat(temp, records[j]->name);
      strcat(temp, "$");
      strcat(temp, records[j]->surname);
      strcat(temp, "$");
      strcat(temp, records[j]->city);
      strcat(temp, "&"); // entry starting/ending point identifier

      strcat(blockData, temp);
    }
    free(temp);
    free(s);
    for (j = 0; j < recordsNum; j++)
      free(records[j]);


    BF_Block_SetDirty(block);
    if (BF_UnpinBlock(block) != BF_OK)
      return SR_ERROR;

      BF_Block_Destroy(&block);
  }


  // here sort groups of 63 blocks and store them in distinct files
  // block starting for variable scoping
  int size = blocksNum/63;
  if (blocksNum%63 > 0)
    size = (blocksNum/63) + 1
  int fileDescs[size];
  {
    //int groupSize = 63;
    int currEntriesNum;
    BF_Block* blocks[64]; // one block is for output
    for (int i = 0; i < 64; i++)
      BF_Block_Init(&blocks[i]);

    char* currEntries[63];
    int bytesForNextEntry[63];
    for (int i = 0; i < 63; i++)
      currEntries[i] = malloc(MAX_ENTRY_SIZE); // max size of entry

    int fileIndex = 0;
    int i = 0; // first block
    int j, n;
    while (i < blocksNum)
    {
      for (j = 0; j < 63; j++)
        bytesForNextEntry[j] = 0;
      // get 63 blocks (the 64th is for output)
      j = 0;
      //n = i + 63;
      currEntriesNum = 0;
      while (j < 63 && i < blocksNum)
      {

        if (BF_GetBlock(fileDesc, i, blocks[j]) != BF_OK)
          return SR_ERROR;

        blockData = BF_Block_GetData(blocks[j]);
        strcpy(blockDataCopy, blockData);
        //blockDataCopy = blockDataCopy + lengthOf(currEntries[j]) + 1;      // FOR NEXT ENTRY
        // get the next entry of each block (of each file later)

        strcpy(currEntries[j], strtok(blockDataCopy, "&")); // first entry of current block

        bytesForNextEntry[j] += strlen(currEntries[j]) + 1;

        currEntriesNum++;
        j++;
        i++;
      }

      // put the entries of the 63 blocks in an output file sorted
      char* fileName = malloc(sizeof(int) + 7*sizeof(char) + 1);
      sprintf(fileName,"output_%d", fileIndex);
      BF_CreateFile(fileName);
      if (BF_OpenFile(fileName, &fileDescs[fileIndex]) != BF_OK)
        return SR_ERROR;
      free(fileName);

      // allocate the first output block
      if (BF_AllocateBlock(fileDescs[fileIndex], blocks[63]) != BF_OK)
        return SR_ERROR;

      blockData = BF_Block_GetData(blocks[63]);

      char* minEntry;
      while (!allEntriesNull(currEntries, 0, currEntriesNum))
      {
        minEntry = findMinCurrEntry(currEntries, 0, currEntriesNum, blocks, NULL/*int* currBlockIds /*for multiple files only*/, NULL, bytesForNextEntry, 1);
        if (strlen(blockData) + strlen(minEntry) <= BF_BLOCK_SIZE)
          strcat(blockData, entry);
        else
        {
          BF_Block_SetDirty(blocks[63]);
          if (BF_UnpinBlock(blocks[63]) != BF_OK)
            return SR_ERROR;

          // allocate the first output block
          if (BF_AllocateBlock(fileDescs[fileIndex], blocks[63]) != BF_OK)
            return SR_ERROR;

          blockData = BF_Block_GetData(blocks[63]);

          strcat(blockData, minEntry);
        }
      }

      BF_Block_SetDirty(blocks[63]);
      if (BF_UnpinBlock(blocks[63]) != BF_OK)
        return SR_ERROR;

      // unpin all blocks to make room for the next ones
      for (j = 0; j < 63; j++)
      {
        BF_Block_SetDirty(blocks[j]);
        if (BF_UnpinBlock(blocks[j]) != BF_OK)
          return SR_ERROR;
      }

      // find the min entry among all non-NULL currEntries and insert it in the output block which is the last non-full block of the output file with name "filename"
      // if the output block is full: set it dirty, unpin it and allocate a new block for the output file
      // stop the procedure if all of the 63 blocks have no other entries (until all currEntries are NULL)


      fileIndex++;
    }
    for (int i = 0; i < 64; i++)
      BF_Block_Destroy(&blocks[i]);
  }

  // then sort groups of 63*63 blocks and store them in distinct files
  // ...
  // until the output file is only one

  void mergeAndSortFiles(int* inputFileDescs, int inputFilesNum, int outputFilesNum)
  {
    int outputFileDescs[outputFilesNum];
    int currBlockIds[inputFilesNum];
    BF_Block* blocks[inputFilesNum];
    BF_Block* outputBlock;
    BF_Block_Init(&outputBlock);
    char* currEntries[inputFilesNum];
    int currEntriesNum;

    int bytesForNextEntry[inputFilesNum];
    for (int i = 0; i < 63; i++)
    {
      currBlockIds[i] = 0;
      bytesForNextEntry[i] = 0;
    }
    char* blockData, blockDataCopy;
    blockDataCopy = malloc(BF_BLOCK_SIZE);

    int outputFilesIndex = 0;
    int i = 0;
    int j;
    int runNum = 0;
    while (i < inputFilesNum)
    {
      int n = i + 63;

      // allocate memory for new currEntries
      for (j = i; j < n; i++)
        currEntries[j] = malloc(MAX_ENTRY_SIZE); // max size of entry

      currEntriesNum = 0;
      while (i < n && i < inputFilesNum)
      {
        BF_Block_Init(&blocks[i]);

        if (BF_GetBlock(inputFileDescs[i], 0, blocks[i]) != BF_OK)
          return SR_ERROR;

        blockData = BF_Block_GetData(blocks[i]);
        strcpy(blockDataCopy, blockData);
        //blockDataCopy = blockDataCopy + lengthOf(currEntries[j]) + 1;      // FOR NEXT ENTRY
        // get the next entry of each block (of each file later)

        strcpy(currEntries[i], strtok(blockDataCopy, "&")); // first entry of current block

        bytesForNextEntry[i] += strlen(currEntries[i]) + 1;

        currEntriesNum++;
        i++;
      }
      free(blockDataCopy);

      // put the entries of the 63 files in a single output file sorted
      char* fileName = malloc(sizeof(int) + 7*sizeof(char) + 1);
      sprintf(fileName,"output_%d", outputFilesIndex);
      BF_CreateFile(fileName);
      if (BF_OpenFile(fileName, &fileDescs[outputFilesIndex]) != BF_OK)
        return SR_ERROR;
      free(fileName);

      // allocate the first output block
      if (BF_AllocateBlock(fileDescs[outputFilesIndex], outputBlock) != BF_OK)
        return SR_ERROR;

      blockData = BF_Block_GetData(outputBlock);


      char* minEntry;
      int start = i - currEntriesNum;
      int end = i;
      while (!allEntriesNull(currEntries, start, end))
      {
        minEntry = findMinCurrEntry(currEntries, start, currEntriesNum, blocks, currBlockIds, inputFileDescs, bytesForNextEntry, 0);

        if (strlen(blockData) + strlen(minEntry) <= BF_BLOCK_SIZE) // if entry fits in block
          strcat(blockData, entry);
        else
        {
          BF_Block_SetDirty(outputBlock);
          if (BF_UnpinBlock(outputBlock) != BF_OK)
            return SR_ERROR;

          // allocate the first output block
          if (BF_AllocateBlock(fileDescs[fileIndex], outputBlock) != BF_OK)
            return SR_ERROR;

          blockData = BF_Block_GetData(outputBlock);

          strcat(blockData, minEntry);
        }
      }

      BF_Block_SetDirty(outputBlock);
      if (BF_UnpinBlock(outputBlock) != BF_OK)
        return SR_ERROR;

      // unpin all blocks to make room for the next ones

      for (j = start; j < end; j++)
      {
        BF_Block_SetDirty(blocks[j]);
        if (BF_UnpinBlock(blocks[j]) != BF_OK)
          return SR_ERROR;
        BF_Block_Destroy(&blocks[j]);
        // free previous currEntries
        free(currEntries[j]);
      }

      outputFilesIndex++;
    }

  }

  free(blockDataCopy);
  BF_Block_Destroy(outputBlock);
  return SR_OK;
}

SR_ErrorCode SR_PrintAllEntries(int fileDesc) {

  BF_Block* block = malloc(BF_BLOCK_SIZE);
  char* blockData;
  int blocksNum;
  char* blockDataEntries[40]; // 40 is the average number of entries that fit in a block
  char* blockDataTokens[4];
  char* temp = malloc(20*(sizeof(int) + 50*sizeof(char) + 1));

  if (BF_GetBlockCounter(fileDesc, &blocksNum) != BF_OK)
    return SR_ERROR;

  if (blocksNum > 1) // file contains entries
  {
    // access all data blocks of the sort file and print all entries in a user-friendly format
    for (int i = 1; i < blocksNum; i++)
    {
      if (BF_GetBlock(fileDesc, i, block) != BF_OK)
        return SR_ERROR;

      blockData = BF_Block_GetData(block);
      strcpy(temp, blockData);

      if (BF_UnpinBlock(block) != BF_OK)
        return SR_ERROR;

      blockDataEntries[0] = strtok(temp, "&");

      int j = 1;
      while (blockDataEntries[j - 1] != NULL && j < 40) // while the previous entry is valid
      {
        blockDataEntries[j] = strtok(NULL, "&");
      	j++;
      }

      j = 0;
      while (blockDataEntries[j] != NULL && j < 40)
      {
        blockDataTokens[0] = strtok(blockDataEntries[j], "$");
        if (blockDataTokens[0] != NULL)
        {
          for (int k = 1; k < 4;k++)
             blockDataTokens[k] = strtok(NULL,"$");

          printf("%d, \"%s\", \"%s\", \"%s\"\n", atoi(blockDataTokens[0]), blockDataTokens[1], blockDataTokens[2], blockDataTokens[3]);
        }
        else
          break;

      	j++;
      }
    }
  }
  else // file empty
  {
    printf("File is empty");
    return SR_ERROR;
  }

  // free allocated memory
  BF_Block_Destroy(&block);
  free(temp);

  return SR_OK;
}
