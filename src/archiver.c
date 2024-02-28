#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#define MAX_D 2048
#define PATH_LEN 1024 // max path length
#define MAX_FILES 1024 // max number of files

// write the headers to the archive
void writeHeaders(const char* directoryPath, FILE* archive, int *counter, const char* emptyPath)
{
    // open the directory
    DIR* dir = opendir(directoryPath);
    if (!dir)
    {
        perror("Error opening directory");
        exit(1);
    }

    struct dirent* ent;
    struct stat info;

    // read the directory
    while ((ent = readdir(dir))!= NULL)
    {
        char filePath[PATH_LEN] = { 0 };

        sprintf(filePath, "%s/%s", directoryPath, ent->d_name);
        char relatedDirPath[PATH_LEN] = {0};
        sprintf(relatedDirPath, "%s/%s", emptyPath, ent->d_name);
        stat(filePath, &info);
        if (S_ISDIR(info.st_mode)) // if it's a directory
        {
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) // ignore. and..
            {
                fprintf(archive, "%d /%s\n", 0, ent->d_name);
                (*counter)++;
                writeHeaders(filePath, archive, counter, relatedDirPath);
            }
        }
        else // if it's a file
        {
            // write size and path to the archive
            fprintf(archive, "%d %s\n", (int)info.st_size, relatedDirPath);
            // increment the counter
            (*counter)++;
        }
    }
    // close the directory
    closedir(dir);
}


// archive a directory recursively
void writeContent(const char *directoryPath, FILE* archive) 
{
    // open the directory
    DIR *dir = opendir(directoryPath);
    // if not found
    if (!dir)
    {
        perror("Error opening directory");
        exit(1);
    }
    
    struct stat info;
    struct dirent *entry;


    // read the directory
    while ((entry = readdir(dir)) != NULL)
    {
        char filePath[PATH_LEN] = { 0 };

        snprintf(filePath, sizeof(filePath), "%s/%s", directoryPath, entry->d_name);
        if (stat(filePath, &info) == -1)
        {
            perror("Error getting file info");
            exit(1);
        }
        // if it's a directory, recursively archive it
        if (S_ISDIR(info.st_mode))
        {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                writeContent(filePath, archive);
            }
        }
        // else if it's a regular file, archive it
        else
        {
            FILE *file = fopen(filePath, "rb");
            if (!file)
            {
                perror("Error opening file");
                exit(1);
            }
            int c;

            while ((c = fgetc(file))!= EOF)
            {
                fputc(c, archive);
            }
            fclose(file);
        }
    }
    closedir(dir);
}

// archive a directory
FILE* archiver(const char* directoryPath, const char* archiveName, int* counter)
{
    // open the archive
    FILE *archive = fopen(archiveName, "wb+");
    if (!archive)
    {
        perror("Error opening archive");
        exit(1);
    }

    // write the headers
    writeHeaders(directoryPath, archive, counter, "");

    // write the content
    writeContent(directoryPath, archive);

    return archive;
}

// char *getIsolatedName(char *str) {
//     char *isolated = str;
//     char *tmp = strchr(isolated, '/');
//     while (tmp) {
//         if (tmp[1])
//         {
//             isolated = tmp + 1;
//             tmp = strchr(isolated, '/');
//         }
//         else
//         {
//             tmp = NULL;
//         }
        
//     }
//     return isolated;
// }

// unpack an archive
void unpacker(FILE* archive, const int filesCount, const char* destinationPath, char* directoryPath)
{
    rewind(archive);
    int *fileSizes = malloc(sizeof(int) * filesCount);
    char **filePaths = malloc(sizeof(char *) * filesCount);
    {
        char *pathsValues = malloc(sizeof(char) * filesCount * PATH_LEN);
        for (int i = 0; i < filesCount; ++i) {
            filePaths[i] = pathsValues + PATH_LEN * i;
        }
    }

    char *line = NULL;
    unsigned long line_size = 0;
    for (int i = 0; i < filesCount; i++)
    {
        getline(&line, &line_size, archive);
        sscanf(line, "%d %s", &fileSizes[i], filePaths[i]);
    }

    DIR *dir = opendir(destinationPath);
    if (!dir)
    {
        mkdir(destinationPath, S_IRWXU | S_IRWXG | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    }

    for (int i = 0; i < filesCount; i++)
    {
        char path[PATH_LEN] = { 0 };
        sprintf(path, "%s%s", destinationPath, filePaths[i]);
        if (fileSizes[i] == 0)
        {
            mkdir(path, S_IRWXU | S_IRWXG | S_IRGRP);
            continue;
        }
        FILE* outputFile = fopen(path, "wb");
        for (int j = 0; j < fileSizes[i]; j++)
        {
            fputc(fgetc(archive), outputFile);
        }
        fclose(outputFile);
    }

    if (line) {
        free(line);
    }
    if (fileSizes) {
        free(fileSizes);
    }
    if (filePaths) {
        free(filePaths[0]);
        free(filePaths);
    }
    closedir(dir);
    fclose(archive);
}

// main function
int main(int argc, char *argv[])
{
    const char* archivePath = "~/Documents/repos/OS/1/archive.tar";
    // check the number of arguments
    if (argc!= 4)
    {
        fprintf(stderr, "Usage: %s <directory> <archive>\n", argv[0]);
        exit(1);
    }
    // create the counter
    int counter = 0;

    // archive the directory
    FILE* archive = archiver(argv[1], argv[2], &counter);

    printf("Archiver finished with %d files\n", counter);

    // unpack the archive
    unpacker(archive, counter, argv[3], argv[1]);
    return 0;
}