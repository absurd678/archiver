#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
/// home/artem/os_labs/archiver/archiver.c
//-----------------CONSTANTS-----------------
#define BUFFSIZE1024 1024 // Buffer size for reading files
#define buffSize256 256
#define pATH_MAX 4096 // Maximum path length

// TODO: Replace hardcoded paths with environment variables
const char *DEFAULT_PATH = "/home/artem/os_labs/archiver/archiver.c"; // Default path for archive file
const char *DEFAULT_ARCH_NAME = "son_arch.geo";
const char *DEFAULT_NAME = "son_arch";

// Error codes
#define ERR_OPN_DIR 1
#define ERR_OPN_FILE 2
#define ERR_WRT_ARCH 3
#define ERR_RD_ARCH 4
#define ERR_EXTR_ARCH 5
#define ERR_CRT_DIR 6
#define ERR_EXTR 7
#define ERR_PTH_ARCH 8

//-----------------PROTOTYPES-----------------
int archiveFile(char *dir, size_t root_size, FILE *file_out); // Archive file
int processDir(char *dir, FILE *file_out, size_t root_size);  //
int createNewDirectory(char *curr_dir, char *base_path);      // Create new directory
int processArchive(FILE *archiveFile, char *top_dir_path);    // Process archive file
void PrintErr(int errorCode);                                 // Print error code

//-----------------VARIABLES-----------------
FILE *archiver;                    // Archive file
char topdir[BUFFSIZE1024];         // Current directory
char archiveFileName[buffSize256]; // Archive file name
int opt;                           // Action (1 - archive, 2 - extract)
int errCode = 0;                   // Error code
char res[buffSize256];             // Response from user
char extractName[buffSize256];     // Extract directory name

//-----------------MAIN FUNCTION-----------------

int main()
{

    printf("Выберете действие:\n1. Архивировать\n2. Разархивировать\n/>");
    scanf("%d", &opt);

    switch (opt)
    {

    case 1: // Archive

        printf("Напишите полный путь, где находится папка:\n/>");
        scanf("%s", topdir);

        printf("Положить архив в папку по умолчанию? (Y/N)\n/>");
        scanf("%s", res);
        if (res[0] == 'N' || res[0] == 'n')
        {
            printf("Напишите полный путь, куда положить архив:\n/>");
            scanf("%s", archiveFileName);
            if (fopen(archiveFileName, "r") == NULL) // error if the directory does not exist
            {
                PrintErr(ERR_OPN_DIR);
                break;
            }
            strcat(archiveFileName, "/");
        }
        printf("Придумайте имя для архива:\n/>");
        scanf("%s", res);

        strcat(archiveFileName, res);
        archiver = fopen(archiveFileName, "ab+");

        // Archiving - not correct, we need just the names of the files!
        if ((errCode = processDir(topdir, archiver, strlen(topdir))) != 0)
        {
            PrintErr(errCode);
            fclose(archiver);
            break;
        }
        printf("Архив создан по пути:\n");
        printf("%s\n", realpath(archiveFileName, NULL));
        fclose(archiver);
        break;

    case 2: // Extract

        // Choosing the archive file
        printf("Напишите полный путь к архиву:\n/>");
        scanf("%s", archiveFileName);
        if ((archiver = fopen(archiveFileName, "r")) == NULL)
        {
            PrintErr(ERR_PTH_ARCH);
            break;
        }

        // Choosing the extraction destination
        printf("Распаковать в папку по умолчанию? (Y/N)\n/>");
        scanf("%s", res);
        if (res[0] == 'N' || res[0] == 'n')
        {
            printf("Напишите полный путь, куда распаковать:\n/>");
            scanf("%s", res);
            if (fopen(res, "r") == NULL) // error if the directory does not exist
            {
                PrintErr(ERR_OPN_DIR);
                break;
            }
            strcat(res, "/");
        }
        else
        {
            strcpy(res, DEFAULT_PATH);
        }
        // Ask for the extract directory name
        printf("Придумайте имя для распакованной папки:\n/>");
        scanf("%s", extractName);
        strcat(res, "/");
        strcat(res, extractName);

        // Extracting
        printf("Ext extract %s\n", res);
        processArchive(archiver, res);
        printf("Распаковано по пути:\n");
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

        printf("newPath %s\n", newPath);
        printf("top_dir_path %s\n", top_dir_path);
        if ((errCode = createNewDirectory(strtok(newPath, "/"), "")) != 0)
        {
            
            return errCode;
        }

        // Create a new file in the decompressed directory
        char *extension = strrchr(original_filename, '.');
        FILE *file_to_write;
        char *helppath = (char *)malloc(strlen(DEFAULT_PATH) + strlen(DEFAULT_ARCH_NAME) + 1);
        if (extension != NULL && strcmp(extension, ".geo") == 0)
        {
            strcpy(helppath, DEFAULT_PATH);
            strcat(helppath, "/");
            strcat(helppath, DEFAULT_ARCH_NAME);
            file_to_write = fopen(helppath, "wb");
        }
        else
        {
            file_to_write = fopen(original_filename, "wb");
        }

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

        if (extension != NULL && strcmp(extension, ".geo") == 0)
        {
            printf(" creating new directory with    extension\n");
            FILE *inner_archive = fopen(helppath, "r");
            if (!inner_archive)
            {
                printf("error");
                return ERR_OPN_FILE;
            }

            char *innerPath = (char *)malloc(strlen(original_filename) + strlen(DEFAULT_ARCH_NAME) + 1); // +1 for '/', +1 for '\0'
            strcpy(innerPath, original_filename);
            char *last_slash = strrchr(innerPath, '/'); // Находим последнее вхождение '/'
            sprintf(last_slash + 1, "%s", DEFAULT_NAME);

            printf("%s\n", innerPath);
            processArchive(inner_archive, innerPath);
        }

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
