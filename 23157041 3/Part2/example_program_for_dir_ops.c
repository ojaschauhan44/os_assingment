#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#define MAX_FILES 100

// Struct to store information about a file
struct File {
  char name[256];
  int size;
  char *contents;
};

// Function to create tar file
void createTar(char *dirPath, char *tarFilePath) {
  // Array to store information about all files in the input directory
  struct File files[MAX_FILES];
  int numFiles = 0;


  // Open the input directory
  DIR *dir = opendir(dirPath);
  if (dir == NULL) {
    perror("Error opening directory");
    exit(1);
  }

  // Read the contents of the input directory
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
  while ((entry = readdir(lddir)) != NULL) {
    // Skip the current and parent directories
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // Get the path of the file
    char filePath[512];
    sprintf(filePath, "%s/%s", dirPath, entry->d_name);

    // Get the information about the file
    struct stat info;
    if (stat(filePath, &info) == -1) {
      perror("Error getting file information");
      exit(1);
    }

    // Store the information about the file in the struct
    strcpy(files[numFiles].name, entry->d_name);
    files[numFiles].size = info.st_size;

    // Allocate memory for the contents of the file
    files[numFiles].contents = malloc(info.st_size);
    if (files[numFiles].contents == NULL) {
      perror("Error allocating memory");
      exit(1);
    }

    // Read the contents of the file
    FILE *file = fopen(filePath, "rb");
    if (file == NULL) {
      perror("Error opening file");
      exit(1);
    }
    fread(files[numFiles].contents, info.st_size, 1, file);
    fclose(file);

    numFiles++;
  }

  // Close the input directory
  closedir(dir);

  // Create the output tar file
  FILE *tarFile = fopen(tarFilePath, "wb");
  if (tarFile == NULL) {
    perror("Error creating tar file");
    exit(1);
  }

  // Write the information about each file to the tar file
  for (int i = 0; i < numFiles; i++) {
    // Write the name of the file
    fwrite(files[i].name, strlen(files[i].name) + 1, 1, tarFile);

    // Write the
}}

