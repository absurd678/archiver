#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>

//g++ -o archiveCompile archiver.c
//./archiveCompile /home/egeniead/OS_Labi/lab_1_v3_Artem/root
//./archiveCompile /home/egeniead/OS_Labi/lab_1_v3_Artem/outArchiver.txt
 
//-----------------CONSTANTS-----------------
const int BUFFSIZE1024 = 1024;
//const int PATH_MAX = 4096;

void archiveFile(char *dir, FILE* file_out){
    const int buffSize = 256;

    FILE* file_to_archive = fopen(dir, "r");   //открыли файл внутри папки

    if(!file_to_archive){  //если не открылось
        fprintf(stderr, "Error opening files\n");
        exit(EXIT_FAILURE);
    }//if

    // Put the path and size of the file
    char full_path[4096];
    realpath(dir, full_path);
    //Функция realpath () выводит из имени пути абсолютное имя пути, 
    //которое называет тот же файл, разрешение которого не включает ".", ".." 
    //Сгенерированное имя пути сохраняется в буфере, на который указывает full_path

    fputs(full_path, file_out);  //записали строку, указанную в параметре full_path в поток file_out
    fputs(" | ", file_out);

    fseek(file_to_archive, 0, SEEK_END);  //перемещает указатель позиции в потоке
    //fseek(Указатель на объект типа FILE; Кол-во байт для смещения; Позиция указателя, относительно которой будет выполняться смещение)
    long length = ftell(file_to_archive); //возвращает значение указателя текущего положения потока
    fprintf(file_out, "%ld\n", length); // put the length
    fseek(file_to_archive, 0, SEEK_SET);
    char buffer[BUFFSIZE1024];
    size_t total_bytes_read = 0;

    while((total_bytes_read = fread(buffer, 1, BUFFSIZE1024, file_to_archive)) > 0){  //fread считывает массив
        //если не все считали
        if (fwrite(buffer, 1, total_bytes_read, file_out)!= total_bytes_read){
            perror("Error writing to archive file\n");
            fclose(file_to_archive);
            fclose(file_out);
            exit(1);
        }//if
    }//while
    
    fputs("\n", file_out);
    fclose(file_to_archive);
}//void archiveFile

//запись в архив
void processDir(char *dir, FILE* file_out) {
    DIR *dp; //поток каталога
    struct dirent *entry;
    struct stat statbuf;

    //Opendir - функция открывает и возвращает поток каталога для чтения каталога с именем dirname
    if ((dp = opendir(dir)) == NULL) {
        fprintf(stderr, "cannot open directory: %s\n", dir);
        return;
    }
    chdir(dir);

    //пока не все считали (?)
    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);  //вернули инфу о файле в буфер
        if (S_ISDIR(statbuf.st_mode)) {
            /* Находит каталог, но игнорирует . и .. */
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;
            // Enters a new directory
            processDir(entry->d_name, file_out);
        } //if
        else {
            archiveFile(entry->d_name, file_out); // Archiving
        }
    }//while
    chdir("..");
    closedir(dp);
}//void processDir
///////////////////////////////////////////////////////////////////////
void extractFile(FILE* file_in) {
    char full_path[4096];
    long length;

    // Читаем полный путь файла
    if (fgets(full_path, sizeof(full_path), file_in) == NULL) {
        fprintf(stderr, "Error reading file path\n");
        exit(EXIT_FAILURE);
    }
    full_path[strcspn(full_path, "\n")] = 0; // Убираем символ новой строки

    /*
    // Читаем размер файла
    if (fscanf(file_in, "%ld\n", &length) != 1) {
        fprintf(stderr, "Error reading file size\n");
        exit(EXIT_FAILURE);
    }*/

    // Создаем директорию, если она не существует
    char* dir_path = strdup(full_path);
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0'; // Отделяем путь от имени файла
        mkdir(dir_path, 0755); // Создаем директорию
    }
    free(dir_path);

    // Открываем файл для записи
    FILE* file_out = fopen(full_path, "wb");
    if (!file_out) {
        fprintf(stderr, "Error creating file: %s\n", full_path);
        exit(EXIT_FAILURE);
    }

    // Читаем содержимое файла из архива
    char buffer[BUFFSIZE1024];
    size_t total_bytes_read = 0;
    while (total_bytes_read < length) {
        size_t bytes_to_read = (length - total_bytes_read < BUFFSIZE1024) ? (length - total_bytes_read) : BUFFSIZE1024;
        size_t bytes_read = fread(buffer, 1, bytes_to_read, file_in);
        fwrite(buffer, 1, bytes_read, file_out);
        total_bytes_read += bytes_read;
    }

    fclose(file_out);
}

void processArchive(const char* archive_file) {
    FILE* file_in = fopen(archive_file, "rb");
    if (!file_in) {
        fprintf(stderr, "Error opening archive file: %s\n", archive_file);
        exit(EXIT_FAILURE);
    }

    while (!feof(file_in)) {
        extractFile(file_in);
    }

    fclose(file_in);
}

//////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    while(1) {
        char *topdir = ".";
        if (argc >= 2) topdir = argv[1];
            printf("Directory scan of %s\n", topdir);

        FILE* file_out = fopen("outArchiver.txt", "ab+");
        
        printf("Выберите вариант:\n1 - архивация,\n2 - разархивация,\n0 - завершить работу: ");
        int option = -1;
        if (scanf("%d", &option) != 1) {
            printf("Введен неверный параметр. Попробуйте еще раз.\n");
            while (getchar() != '\n');
            continue;
        }//if

        switch(option) {
            case 0:
                printf("Завершение работы…\n");
                return 0;
            case 1:  //архивация               
                processDir(topdir, file_out);
                printf("Все заархивировалось и не крашнулось!.\n");
                fclose(file_out);
                exit(0);
                break;
            case 2: //разархивация               
                if (argc != 2) {
                    fprintf(stderr, "Usage: %s <archive_file>\n", argv[0]);
                    printf("Все разархивировалось и не крашнулось!.\n");
                    return EXIT_FAILURE;
                }
                 processArchive(argv[1]);
                return EXIT_SUCCESS;
                break;
            default:
                printf("Введен неверный параметр. Попробуйте еще раз.\n");
                break;
        }//switch
    }//while
}//main

/*
int main(int argc, char* argv[]) {
while(1) {
        printf("Выберите вариант:\n1 - архивация,\n2 - разархивация,\n0 - завершить работу: ");
        int option = -1;
        if (scanf("%d", &option) != 1) {
            printf("Введен неверный параметр. Попробуйте еще раз.\n");
            while (getchar() != '\n');
            continue;
        }//if
        
        FILE* file_out = fopen("outArchiver.txt", "ab+");
        char *topdir = ".";
        switch(option) {
            case 0:
                printf("Завершение работы…\n");
                return 0;
            case 1:  //архивация               
                processDir(topdir, file_out);
                printf("done.\n");
                fclose(file_out);
                break;
            case 2: //разархивация               
                if (argc != 2) {
                    fprintf(stderr, "Usage: %s <archive_file>\n", argv[0]);
                    return EXIT_FAILURE;
                }
                 processArchive(argv[1]);
                return EXIT_SUCCESS;
                break;
            default:
                printf("Введен неверный параметр. Попробуйте еще раз.\n");
                break;
        }//switch
    }//while (1)
}//main
*/


/////////////////////////////////////////////////////////////////////////
/*
//void read_from_archive(char* path, char *dir, FILE* file_out) {
void read_from_archive(char* path) {
    int MAX_PATH_SIZE = 4096;

    //FILE* file_to_archive = fopen(dir, "r");   //открыли файл для чтения
    FILE* file_to_archive = fopen(path, "r");   //открыли файл для чтения
    if(!file_to_archive){  //если не открылось
        fprintf(stderr, "Error opening files\n");
        exit(EXIT_FAILURE);
    }//if

    char currentPath[MAX_PATH_SIZE];
    char prevPath[MAX_PATH_SIZE];
    int length;

    //fscanf читает информацию из текстового файла и преобразует ее во внутреннее представление данных в памяти компьютера
    while (fscanf(file_to_archive, "%[*|]|%d<\n", currentPath, &length) == 2) {
        //убираем перенос строки при необходимости
        if (currentPath[0] == '\n') {
            memmove(currentPath, currentPath + 1, strlen(currentPath));
        }//if

        strcpy(prevPath, currentPath);
        printf("Файл: %s; Длина: %d\n", currentPath, length);
        char* folder = strdup(currentPath); //указатель на дублируемую строку пути
        char* fileName = strrchr(folder, '/'); //имя файла
        if (fileName != NULL) {
            *fileName = '\0';
        }//if

        char* token;
        char* copy = startup(folder);
        char* delimiter = "/";
        char buildPath[MAX_PATH_SIZE];   
        buildPath[MAX_PATH_SIZE] = " "; //инициализировали, блин
        
        token = strtok(copy, delimiter); //разбиение строки на части по разделителю /
        struct stat st;

        while (token != NULL) {
            strcat(buildPath, "/");  //присоединяет к концу строки str1 строку str2.
            strcat(buildPath, token);
            if (stat(buildPath, &st) == -1) {
                int status = mkdir(buildPath, 0777); //создание каталога по умолчанию
                if (status != 0) { //если каталог неуспешно создан
                    printf("Error creating folder\n");
                }//if
            }//if
            token = strtok(NULL, delimiter); //продолжить работу с последней указанной строкой
        }//while
        free(copy); //освобождение блока памяти

        int symbolsToWrite = length;
        int bufferSize;
        char buffer[BUFFSIZE];
        size_t bytes_read;
        FILE* currentFile = fopen(currentPath, "w"); //открыть файл для записи
        if (currentFile == NULL) {
            perror("Error opening output file");
            fclose(file_to_archive);
        }//if

        while (symbolsToWrite > 0) {
            if (BUFFSIZE > symbolsToWrite) {
                bufferSize = symbolsToWrite;
            }//if
            else {
                bufferSize = BUFFSIZE;
            }//else
            bytes_read = fread(buffer, 1, bufferSize, file_to_archive); //считываем по 1 байту в буфер
            if (bytes_read == 0) { //если ничего не считано 
                break;
            }//if
             
            fwrite(buffer, 1, bytes_read, currentFile); //запись побайтно в текущий файл
            symbolsToWrite -= bytes_read;
        }//while

        /*   (мне нужно по варианту время сохранять)
        char* extension = strrchr(currentPath, '.'); //определяем местонахождение точки в строке
        if (strcmp(extension, ".txt") == 0) { //если расширение текст
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            fprintf(currentFile, "Дата разархивации: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1,
            tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        }//if
        
        fclose(currentFile);
    }//while
    fclose(file_to_archive);
}//void read_from_archive

void extract() {
    int MAX_PATH_SIZE = 4096;
    char path[MAX_PATH_SIZE];
    path[MAX_PATH_SIZE] = "/home/egeniead/OS_Labi/lab_1_v3_Artem/root";
    char* extension = strrchr(path, '-');

    if (strcmp(extension, ".titov") != 0) { //если не расширение архива
        printf("Ошибка. Данный файл не является архивом.\n");
        return;
    }//if
    printf("Разархивация…\n");
    //archiveFile(entry->d_name, file_out); // Archiving
    read_from_archive(path);
    printf("Разархивация завершена. \n");
}//void extract
*/

/*
int main(int argc, char* argv[]) {
    char *topdir = ".";
    if (argc >= 2) topdir = argv[1];
    printf("Directory scan of %s\n", topdir);

    FILE* file_out = fopen("outArchiver.txt", "ab+");
    processDir(topdir, file_out);
    printf("done.\n");
    fclose(file_out);
    exit(0);
}//main
*/