#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

//-----------------CONSTANTS-----------------
const int BUFFSIZE1024 = 1024; // Buffer size for reading files
const int buffSize256 = 256;
const int pATH_MAX = 4096; // Maximum path length

// TODO: Replace hardcoded paths with environment variables
const char *DEFAULT_PATH = "/home/artem/os_labs/archiver"; // Default path for archive file
char *ENCRYPTION_PATH = "/home/artem/os_labs/archiver/encode";


// Error codes
const int ERR_OPN_DIR = 1;
const int ERR_OPN_FILE = 2;
const int ERR_WRT_ARCH = 3;
const int ERR_RD_ARCH = 4;
const int ERR_EXTR_ARCH = 5;
const int ERR_CRT_DIR = 6;
const int ERR_EXTR = 7;
const int ERR_PTH_ARCH = 8;

//-----------------PROTOTYPES-----------------
int archiveFile(char *dir, size_t root_size, FILE *file_out); // Archive file
int processDir(char *dir, FILE *file_out, size_t root_size);  //
int createNewDirectory(char *curr_dir, char *base_path);      // Create new directory
int processArchive(FILE *archiveFile, char *top_dir_path);    // Process archive file
void PrintErr(int errorCode);                                 // Print error code
int zipDirectory(char *dirPath, char *encryptPath, size_t root_size);
int encode(char *dirPath, char *encryptPath, size_t root_size);
char* encode_line(char* line);


//-----------------VARIABLES-----------------
FILE *archiver;                    // Archive file
char topdir[BUFFSIZE1024];         // Current directory
char archiveFileName[buffSize256]; // Archive file name
int option;                           // Action (1 - archive, 2 - extract)
int errCode = 0;                   // Error code
char res[buffSize256];             // Response from user
char extractName[buffSize256];     // Extract directory name

//-----------------MAIN FUNCTION-----------------

int main()
{

    printf("Choose the action:\n1. Archive\n2. Extract\n/>");
    scanf("%d", &option);

    switch (option)
    {

    case 1: // Archive

        printf("Write the realpath of the directory to archive:\n/>");
        scanf("%s", topdir);
        // encoding the directory
        errCode = zipDirectory(topdir, ENCRYPTION_PATH, 0);
        if (errCode != 0){
            PrintErr(errCode);
            break;
        }
/*
        printf("Put the archive by default? (Y/N)\n/>");
        scanf("%s", res);
        if (res[0] == 'N' || res[0] == 'n')
        {
            printf("Enter the realpath of the directory to archive:\n/>");
            scanf("%s", archiveFileName);
            if (fopen(archiveFileName, "r") == NULL) // error if the directory does not exist
            {
                PrintErr(ERR_OPN_DIR);
                break;
            }
        }
        printf("Write the name of the archive:\n/>");
        scanf("%s", res);

        strcat(archiveFileName, res);
        archiver = fopen(archiveFileName, "ab+");

        // Archiving - not correct, we need just the names of the files!
        if ((errCode = processDir(ENCRYPTION_PATH, archiver, strlen(ENCRYPTION_PATH))) != 0)
        {
            PrintErr(errCode);
            fclose(archiver);
            break;
        }
        printf("Archive created successfully with path:\n");
        printf("%s\n", realpath(archiveFileName, NULL));
        fclose(archiver);
        */
        break;

    case 2: // Extract

        // Choosing the archive file
        printf("Write the realpath of the archive to extract:\n/>");
        scanf("%s", archiveFileName);
        if ((archiver = fopen(archiveFileName, "r")) == NULL)
        {
            PrintErr(ERR_PTH_ARCH);
            break;
        }

        // Choosing the extraction destination
        printf("Put the archive by default? (Y/N)\n/>");
        scanf("%s", res);
        if (res[0] == 'N' || res[0] == 'n')
        {
            printf("Enter the realpath of the directory for extract:\n/>");
            scanf("%s", res);
            if (fopen(res, "r") == NULL) // error if the directory does not exist
            {
                PrintErr(ERR_OPN_DIR);
                break;
            }
        }
        else
        {
            strcpy(res, DEFAULT_PATH);
        }
        // Ask for the extract directory name
        printf("Write the extract directory name:\n/>");
        scanf("%s", extractName);
        strcat(res, "/");
        strcat(res, extractName);

        // Extracting
        processArchive(archiver, res);
        printf("Extracted successfully with path:\n");
        printf("%s\n", realpath(res, NULL));
        fclose(archiver);
        break;

    default:

        printf("Invalid option. Exiting.\n");
        exit(1);
    }

    exit(0);
}

//-----------------IMPLEMENTATION OF FUNCTIONS-----------------

int archiveFile(char *dir, size_t root_size, FILE *file_out)
{

    int errCode = 0;

    FILE *file_to_archive = fopen(dir, "r");

    if (!file_to_archive)
    {
        return ERR_OPN_FILE;
    }

    // Put the path and size of the file
    char full_path[pATH_MAX];
    realpath(dir, full_path);

    // full_path = full_path - top_dir_path
    char *new_path = full_path + root_size;

    fputs(new_path, file_out);
    fputs(" | ", file_out);

    fseek(file_to_archive, 0, SEEK_END);
    long length = ftell(file_to_archive);
    fprintf(file_out, "%ld\n", length); // put the length
    fseek(file_to_archive, 0, SEEK_SET);
    char buffer[BUFFSIZE1024];
    size_t total_bytes_read = 0;
    while ((total_bytes_read = fread(buffer, 1, BUFFSIZE1024, file_to_archive)) > 0)
    {
        if (fwrite(buffer, 1, total_bytes_read, file_out) != total_bytes_read)
        {
            fclose(file_to_archive);
            fclose(file_out);
            return ERR_WRT_ARCH;
        }
    }

    fputs("\n", file_out);
    fclose(file_to_archive);
    return 0;
}

int processDir(char *dir, FILE *file_out, size_t root_size)
{

    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    int errCode = 0;

    if ((dp = opendir(dir)) == NULL)
    {
        return ERR_OPN_DIR;
    }
    chdir(dir);
    while ((entry = readdir(dp)) != NULL)
    {
        lstat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode))
        {
            /* Finds the directory but ignores . and .. */
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;
            // Enters a new directory
            if ((errCode = processDir(entry->d_name, file_out, root_size)) != 0)
            {
                return errCode;
            }
        }
        else
        {
            if ((errCode = archiveFile(entry->d_name, root_size, file_out)) != 0) // Archiving
            {
                return errCode;
            }
        }
    }
    chdir("..");
    closedir(dp);
    return 0;
}

int processArchive(FILE *archiveFile, char *top_dir_path)
{
    int errCode = 0;
    char buffer[BUFFSIZE1024];

    while (fgets(buffer, BUFFSIZE1024, archiveFile))
    {
        char original_filename[BUFFSIZE1024]; // Store the original path before createNewDirectory() called

        char *filename = strtok(buffer, " | ");
        size_t size = atol(strtok(NULL, " | "));

        // Recover the directory structure
        // filename = top_dir_path + filename
        char *newPath = (char *)malloc(strlen(top_dir_path) + strlen(filename) + 1); // +1 for '/', +1 for '\0'
        strcpy(newPath, top_dir_path);
        strcat(newPath, filename);
        strncpy(original_filename, newPath, BUFFSIZE1024); // save the original path before createNewDirectory() called

        if ((errCode = createNewDirectory(strtok(newPath, "/"), "")) != 0)
        {
            return errCode;
        }

        // Create a new file in the decompressed directory
        FILE *file_to_write = fopen(original_filename, "wb");
        if (!file_to_write)
        {
            return ERR_OPN_FILE;
        }

        // Read the file data from the archive
        char *data = (char *)malloc(size * sizeof(char)); // dynamical array
        if (fread(data, 1, size, archiveFile) != size)
        {
            fprintf(stderr, "Error reading file data for %s\n", original_filename);
            fclose(file_to_write);
            free(data);
            return ERR_RD_ARCH;
        }

        // Write the file data to the
        if (fwrite(data, 1, size, file_to_write) != size)
        {
            fprintf(stderr, "Error writing data to %s\n", original_filename);
            fclose(file_to_write);
            free(data);
            return ERR_EXTR_ARCH;
        }

        // free allocated memory
        free(data);
        free(newPath);
        fclose(file_to_write);

        // Skip to the next line in the archive
        fgets(buffer, BUFFSIZE1024, archiveFile);
    }
    return 0;
}

// Рекурсивная функция для создания директорий
int createNewDirectory(char *curr_dir, char *base_path)
{
    int errCode = 0;

    if (curr_dir == NULL)
    {
        return 0; // Если нет больше частей пути, выходим
    }

    // Получаем следующую часть пути
    char *next_part = strtok(NULL, "/");

    // Строим полный путь для текущей директории
    char current_path[BUFFSIZE1024];
    snprintf(current_path, BUFFSIZE1024, "%s/%s", base_path, curr_dir);

    if (next_part == NULL)
    {
        // Если это последняя часть пути, значит, это файл
        printf("File to create: %s\n", current_path); // Обработка файла в будущем
        return 0;
    }

    // Проверяем, существует ли директория
    struct stat statbuf;
    if (stat(current_path, &statbuf) != 0)
    {
        // Директории не существует, создаем ее
        if (mkdir(current_path, 0777) != 0)
        {
            return ERR_CRT_DIR;
        }
        printf("Created directory: %s\n", current_path);
    }
    else if (!S_ISDIR(statbuf.st_mode))
    {
        // Существующий путь - не директория, ошибка
        return ERR_EXTR;
    }

    // Переходим к следующей части пути
    if ((errCode = createNewDirectory(next_part, current_path)) != 0)
    {
        return errCode;
    }
    return 0;
}

void PrintErr(int errorCode)
{
    switch (errorCode)
    {
    case ERR_OPN_DIR:
        fprintf(stderr, "Error opening directory\n");
        break;
    case ERR_OPN_FILE:
        fprintf(stderr, "Error opening file\n");
        break;
    case ERR_WRT_ARCH:
        fprintf(stderr, "Error writing to archive file\n");
        break;
    case ERR_RD_ARCH:
        fprintf(stderr, "Error reading archived data\n");
        break;
    case ERR_EXTR_ARCH:
        fprintf(stderr, "Error extracting archived data\n");
        break;
    case ERR_CRT_DIR:
        fprintf(stderr, "Error creating directory\n");
        break;
    case ERR_EXTR:
        fprintf(stderr, "Error: not a directory requested to extract\n");
        break;
    case ERR_PTH_ARCH:
        fprintf(stderr, "Error of extraction: incorrect archive path\n");
        break;
    default:
        fprintf(stderr, "Unknown error code: %d\n", errorCode);
        break;
    }
}

int zipDirectory(char *dirPath, char *encryptPath, size_t root_size)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    int errCode = 0;
    char *encFoldName;   // to save the hierarchy
    char *newEncrypPath; // to mkdir in the encryption directory

    if ((dp = opendir(dirPath)) == NULL)
    {
        return ERR_OPN_DIR;
    }
    chdir(dirPath);

    //printf("encFoldName: %s", encFoldName);
    if (root_size!=0){
        // Put the path and size of the file
        char full_path[pATH_MAX];
        realpath(dirPath, full_path);

        // full_path = full_path - top_dir_path
        encFoldName = full_path + root_size;
        newEncrypPath = (char *)malloc(strlen(encryptPath) + strlen(encFoldName) + 2); // +1 for '/', +1 for '\0'
        strcpy(newEncrypPath, encryptPath);
        strcat(newEncrypPath, encFoldName);
        mkdir(newEncrypPath, 0777);
    } else {
        newEncrypPath = encryptPath;
    }

    // Process current directory
    while ((entry = readdir(dp)) != NULL)
    {
        lstat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode))
        {
            /* Finds the directory but ignores . and .. */
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;
            // Enters a new directory
            if ((errCode = zipDirectory(entry->d_name, newEncrypPath, strlen(dirPath))) != 0)
            {
                return errCode;
            }
        }
        else
        {
            if ((errCode = encode(entry->d_name, newEncrypPath, strlen(dirPath))) != 0) // Archiving
            {
                return errCode;
            }
        }
    }
    chdir("..");
    closedir(dp);
    return 0;
}

int encode(char *dirPath, char *encryptPath, size_t root_size)
{
    int errCode = 0;
    char *encFileName;   // to save the hierarchy
    char *newEncrypPath; // to mkdir in the encryption directory
    FILE* file_in;
    FILE* file_out;
    char buffer[BUFFSIZE1024]; // file lines
    char *encoded_buffer; // encoded file lines
    
    // Put the path and size of the file
    char full_path[pATH_MAX];
    realpath(dirPath, full_path);

    // full_path = full_path - top_dir_path
    encFileName = full_path + root_size;
    newEncrypPath = (char *)malloc(strlen(encryptPath) + strlen(encFileName) + 2); // +1 for '/', +1 for '\0'
    strcpy(newEncrypPath, encryptPath);
    strcat(newEncrypPath, encFileName);
    printf(newEncrypPath, "\tEncrypted file name");

    file_in = fopen(dirPath, "rb");
    file_out = fopen(newEncrypPath, "wb");
    if (file_out==NULL){
        return ERR_OPN_FILE;
    }

    while(fgets(buffer, BUFFSIZE1024, file_in)){
        encoded_buffer = encode_line(buffer);
        fputs(encoded_buffer, file_out);
    }
    return errCode;
}

char* encode_line(char* line){
    char* new_line = NULL;
    size_t len = 0;
    char *seqlen = NULL;
    char subseqSymbol = line[0];
    for (int i=0; i<strlen(line); i++){
        if (line[i] == subseqSymbol){
            len++;
        } else {
            sprintf(seqlen, "%d", len);
            strcat(new_line, seqlen);
            strcat(new_line, &subseqSymbol);
            subseqSymbol = line[i];
            len=1;
        }
    }
    return new_line;
}
