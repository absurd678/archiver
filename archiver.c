#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
 
//-----------------CONSTANTS-----------------
const int BUFFSIZE = 1024;
//const int PATH_MAX = 4096;


//-----------------PROTOTYPES-----------------
void archiveFile(char *dir, FILE* file_out);
void processDir(char *dir, FILE* file_out);
void createNewDirectory(char * filename);
void processArchive(FILE* archiveFile);


//-----------------MAIN FUNCTION-----------------

int main(int argc, char* argv[]) {
    char *topdir = ".";
    if (argc >= 2) topdir = argv[1];
    printf("Directory scan of %s\n", topdir);

    FILE* file_out = fopen("outArchiver.txt", "r");
    processArchive(file_out);
    printf("done.\n");
    fclose(file_out);
    exit(0);
}

//-----------------IMPLEMENTATION OF FUNCTIONS-----------------

void archiveFile(char *dir, FILE* file_out){
    const int buffSize = 256;

    FILE* file_to_archive = fopen(dir, "r");
    

    if(!file_to_archive){
        fprintf(stderr, "Error opening files\n");
        exit(EXIT_FAILURE);
    }

    
    // Put the path and size of the file
    char full_path[4096];
    realpath(dir, full_path);
    fputs(full_path, file_out);
    fputs(" | ", file_out);

    fseek(file_to_archive, 0, SEEK_END);
    long length = ftell(file_to_archive);
    fprintf(file_out, "%ld\n", length); // put the length
    fseek(file_to_archive, 0, SEEK_SET);
    char buffer[BUFFSIZE];
    size_t total_bytes_read = 0;
    while((total_bytes_read = fread(buffer, 1, BUFFSIZE, file_to_archive)) > 0){
        if (fwrite(buffer, 1, total_bytes_read, file_out)!= total_bytes_read){
            perror("Error writing to archive file\n");
            fclose(file_to_archive);
            fclose(file_out);
            exit(1);
        }
    }
    
    fputs("\n", file_out);
    fclose(file_to_archive);
    
}

void processDir(char *dir, FILE* file_out) {
 DIR *dp;
 struct dirent *entry;
 struct stat statbuf;
 if ((dp = opendir(dir)) == NULL) {
  fprintf(stderr, "cannot open directory: %s\n", dir);
  return;
 }
 chdir(dir);
 while((entry = readdir(dp)) != NULL) {
  lstat(entry->d_name, &statbuf);
  if (S_ISDIR(statbuf.st_mode)) {
   /* Находит каталог, но игнорирует . и .. */
   if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
    continue;
    // Enters a new directory
   processDir(entry->d_name, file_out);
  } else {
    archiveFile(entry->d_name, file_out); // Archiving
  }
 }
 chdir("..");
 closedir(dp);
}

void processArchive(FILE* archiveFile) {
    char buffer[BUFFSIZE];
    while (fgets(buffer, BUFFSIZE, archiveFile)) {
        char *filename = strtok(buffer, " | ");
        long size = atol(strtok(NULL, " | "));

        createNewDirectory(filename);


    }
}

void createNewDirectory(char * filename){
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if ((dp = opendir(filename))!= NULL) {  // if the directory already exists
        closedir(dp);
        printf("%s already exists.\n", filename);
        return;
    }

    char *dir = strtok(filename, "/");
    char full_path_dir[BUFFSIZE];
    while((dp=opendir(realpath(dir, full_path_dir)))!= NULL) { // procceed to the unknown directory
        chdir(dir);
        dir = strtok(NULL, "/");
    }
    
    // create a new directory
    while (dir && (stat(dir, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode))) {
        mkdir(dir, 0777);
        chdir(dir);
        dir = strtok(NULL, "/");
    }
}
