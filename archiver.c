#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

 
//-----------------CONSTANTS-----------------
const int BUFFSIZE1024 = 1024;  // Buffer size for reading files
const int buffSize256 = 256;
const int pATH_MAX = 4096;  // Maximum path length


//-----------------PROTOTYPES-----------------
int archiveFile(char *dir, FILE* file_out); // Archive file
int processDir(char *dir, FILE* file_out);  // 
int createNewDirectory(char *curr_dir, char *base_path); // Create new directory
int processArchive(FILE* archiveFile);  // Process archive file
void PrintErr(int errorCode); // Print error code


//-----------------VARIABLES-----------------
FILE* archiver;  // Archive file
char* topdir = ".";  // Current directory
char* archiveFileName;  // Archive file name
int opt;  // Action (1 - archive, 2 - extract)
int errCode = 0;  // Error code


//-----------------MAIN FUNCTION-----------------

int main() {

    printf("Choose the action:\n1. Archive\n2. Extract\n/>");
    scanf("%d", &opt);

    switch (opt) {

        case 1:  // Archive

            printf("Write the realpath of the directory to archive:\n/>");
            scanf("%s", topdir);
            printf("Write the name of the archive:\n/>");
            scanf("%s", archiveFileName);

            archiver = fopen(archiveFileName, "w");
            if ((errCode = processDir(topdir, archiver))!=0){
                PrintErr(errCode);
                fclose(archiver);
                break;
            }
            printf("Archive created successfully with path:\n");
            printf("%s\n", realpath(archiveFileName, NULL));
            fclose(archiver);
            break;

        case 2:  // Extract

            // TODO: add the option to choose the extraction directory
            printf("Write the realpath of the archive to extract:\n/>");
            scanf("%s", archiveFileName);
            if ((archiver = fopen(archiveFileName, "r"))==NULL){
                PrintErr(8);
                break;
            }
            processArchive(archiver);
            printf("Extracted successfully with path:\n");
            printf("%s\n", realpath(archiveFileName, NULL));
            fclose(archiver);
            break;

        default:

            printf("Invalid option. Exiting.\n");
            exit(1);

    }
    
    exit(0);
}


//-----------------IMPLEMENTATION OF FUNCTIONS-----------------

int archiveFile(char *dir, FILE* file_out){
    
    int errCode = 0;

    FILE* file_to_archive = fopen(dir, "r");
    

    if(!file_to_archive){
        return 2;
    }

    
    // Put the path and size of the file
    char full_path[pATH_MAX];
    realpath(dir, full_path);
    fputs(full_path, file_out);
    fputs(" | ", file_out);

    fseek(file_to_archive, 0, SEEK_END);
    long length = ftell(file_to_archive);
    fprintf(file_out, "%ld\n", length); // put the length
    fseek(file_to_archive, 0, SEEK_SET);
    char buffer[BUFFSIZE1024];
    size_t total_bytes_read = 0;
    while((total_bytes_read = fread(buffer, 1, BUFFSIZE1024, file_to_archive)) > 0){
        if (fwrite(buffer, 1, total_bytes_read, file_out)!= total_bytes_read){
            fclose(file_to_archive);
            fclose(file_out);
            return 3;
        }
    }
    
    fputs("\n", file_out);
    fclose(file_to_archive);
    return 0;
}

int processDir(char *dir, FILE* file_out) {
 
 DIR *dp;
 struct dirent *entry;
 struct stat statbuf;
 int errCode = 0;

 if ((dp = opendir(dir)) == NULL) {
  return 1;
 }
 chdir(dir);
 while((entry = readdir(dp)) != NULL) {
  lstat(entry->d_name, &statbuf);
  if (S_ISDIR(statbuf.st_mode)) {
   /* Finds the directory but ignores . and .. */
   if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
    continue;
    // Enters a new directory
   if ((errCode = processDir(entry->d_name, file_out))!=0){
    return errCode;
   }
  } else {
    if ((errCode=archiveFile(entry->d_name, file_out))!=0) // Archiving
    {
        return errCode;
    }
  }
 }
 chdir("..");
 closedir(dp);
 return 0;
}

int processArchive(FILE* archiveFile) {
    int errCode = 0;
    char buffer[BUFFSIZE1024];

    while (fgets(buffer, BUFFSIZE1024, archiveFile)) {
        char original_filename[BUFFSIZE1024]; // Store the original path before createNewDirectory() called
        
        char *filename = strtok(buffer, " | ");
        size_t size = atol(strtok(NULL, " | "));
        strncpy(original_filename, filename, BUFFSIZE1024); //save the original path before createNewDirectory() called

        // Recover the directory structure
        if ((errCode = createNewDirectory(strtok(filename, "/"), ""))!=0){
            return errCode;
        }

        // Create a new file in the decompressed directory
        FILE* file_to_write = fopen(original_filename, "wb");
        if (!file_to_write) {
            return 2;
        }
        
        // Read the file data from the archive
        char *data = (char*)malloc(size*sizeof(char)); // dynamical array
        if (fread(data, 1, size, archiveFile) != size) {
            fprintf(stderr, "Error reading file data for %s\n", original_filename);
            fclose(file_to_write);
            free(data);
            return 4;
        }

        // Write the file data to the
        if (fwrite(data, 1, size, file_to_write) != size) {
            fprintf(stderr, "Error writing data to %s\n", original_filename);
            fclose(file_to_write);
            free(data);
            return 5;
        }

        // free allocated memory
        free(data);
        fclose(file_to_write);

        // Skip to the next line in the archive
        fgets(buffer, BUFFSIZE1024, archiveFile);

    }
    return 0;
}

// Рекурсивная функция для создания директорий
int createNewDirectory(char *curr_dir, char *base_path) {
    int errCode = 0;

    if (curr_dir == NULL) {
        return 0; // Если нет больше частей пути, выходим
    }

    // Получаем следующую часть пути
    char *next_part = strtok(NULL, "/");

    // Строим полный путь для текущей директории
    char current_path[BUFFSIZE1024];
    snprintf(current_path, BUFFSIZE1024, "%s/%s", base_path, curr_dir);

    if (next_part == NULL) {
        // Если это последняя часть пути, значит, это файл
        printf("File to create: %s\n", current_path); // Обработка файла в будущем
        return 0;
    }

    // Проверяем, существует ли директория
    struct stat statbuf;
    if (stat(current_path, &statbuf) != 0) {
        // Директории не существует, создаем ее
        if (mkdir(current_path, 0777) != 0) {
            return 6;
        }
        printf("Created directory: %s\n", current_path);
    } else if (!S_ISDIR(statbuf.st_mode)) {
        // Существующий путь - не директория, ошибка
        return 7;
    }

    // Переходим к следующей части пути
    if ((errCode=createNewDirectory(next_part, current_path))!=0){
        return errCode;
    }
    return 0;
}

void PrintErr(int errorCode){
    switch(errorCode){
        case 1:
            fprintf(stderr, "Error opening directory\n");
            break;
        case 2:
            fprintf(stderr, "Error opening file\n");
            break;
        case 3:
            fprintf(stderr, "Error writing to archive file\n");
            break;
        case 4:
        fprintf(stderr, "Error reading archived data\n");
        break;
        case 5:
        fprintf(stderr, "Error extracting archived data\n");
        break;
        case 6:
        fprintf(stderr, "Error creating directory\n");
        break;
        case 7:
        fprintf(stderr, "Error: not a directory requested to extract\n");
        break;
        default:
            fprintf(stderr, "Unknown error code: %d\n", errorCode);
            break;
    }
}
