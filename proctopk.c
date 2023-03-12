#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MEMORY_NAME "mem"

void findFreq(char* file_name, void *memory, int num_of_words) {
    FILE *f = fopen(file_name, "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *str = malloc(fsize + 1);
    fread(str, fsize, 1, f);
    fclose(f);

    str[fsize] = 0;
    int count = 0, c = 0, i, j = 0, k, space = 0;
    char p[1000][512], str1[512], freq[1000][512];
    char *ptr;
    for (i = 0; i < strlen(str); i++) {
        if ((str[i] == ' ') || (str[i] == '\t')) {
            space++;
        }
    }
    for (i = 0, j = 0, k = 0; j < strlen(str); j++) {
        if ((str[j] == ' ') || (str[j] == 44) || (str[j] == 46)) {
            p[i][k] = '\0';
            i++;
            k = 0;
        } else
            p[i][k++] = str[j];
    }
    k = 0;
    for (i = 0; i <= space; i++) {
        for (j = 0; j <= space; j++) {
            if (i == j) {
                strcpy(freq[k], p[i]);
                k++;
                count++;
                break;
            } else {
                if (strcmp(freq[j], p[i]) != 0)
                    continue;
                else
                    break;
            }
        }
    }
    char* words[count];
    int fr[count];
    for (i = 0; i < count; i++) {
        for (j = 0; j <= space; j++) {
            if (strcmp(freq[i], p[j]) == 0)
                c++;
        }
        words[i] = freq[i];
        fr[i] = c;
    }
    int max = 0;
    int max_index = 0;
    for (int i = 0; i < num_of_words; i++) {
        for (int j = 0; j < count; j++) {
            if (fr[i] > max) {
                max = fr[i];
                max_index = i;
            }
        }
        sprintf(memory + i * 16, "%s", freq[max_index]);
        char p[100];
        sprintf(p, "%d", max);
        sprintf(memory + i * 16 + 8, "%s", p);
        printf("Added to memory %s %s", freq[max_index], p);
        fr[max_index] = 0;
    }
}

int main(int argc, char *argv[]) {
    pid_t pid;

    int num_of_words = atoi(argv[1]);
    char* out_file_name = argv[2];
    int num_of_files = atoi(argv[3]);

    const int size = 4096;
    int shm_fd;
    void *ptr, *file_name_ptr, *child_ptr, *start, *child_mem_start;

    shm_fd = shm_open(MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, size);

    ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    start = file_name_ptr = ptr;
    child_mem_start = child_ptr = start + num_of_files * 8;

    if (ptr == MAP_FAILED) {
        printf("Map Failed\n");
        return -1;
    }
    
    for (int i = 0; i < num_of_files; i++) {
        sprintf(ptr + 8 * i, "%s", argv[4 + i]);
    }

    for (int i = 0; i < num_of_files; i++) {
        pid = fork();
        if (pid < 0) {
            // Fork failed
            exit(-1);
        }
        if (pid == 0) {
            char* file_name = (char*)(file_name_ptr + (8 * i));
            printf("Child executing %s\n", file_name);
            
            findFreq(file_name, child_ptr + 16 * num_of_words * i,num_of_words);

            break;
        }
    }

    if (pid != 0) {
        for (int i = 0; i < num_of_files; i++) {
            wait(NULL);
            printf("Child terminated\n");
        }
        
        char* words[num_of_files * num_of_words];
        int unique_words = 0;
        int isUnique = 1;
        int freq[num_of_files * num_of_words];
        for (int i = 0; i < num_of_files; i++) {
            printf("File değişti\n");
            for (int j = 0; j < num_of_words; j++) {
                printf("%s\n",(char*)child_mem_start + 16 * j);
                for (int wordIndex = 0; wordIndex < unique_words; wordIndex++) {
                    if (strcmp(words[wordIndex], (char*)child_mem_start + 16 * j) == 0) {
                        printf("Uniq değil");
                        isUnique = 0;
                        int fr = atoi((char*)child_mem_start + 16 * j + 8);
                        freq[wordIndex] += fr;
                    }
                }
                if (isUnique) {
                    words[unique_words] = (char*)child_mem_start + 16 * j;
                    int fr = atoi((char*)child_mem_start + 16 * j + 8);
                    freq[unique_words] = fr;
                    unique_words++;
                }
                // fprintf(out, "%s", (char*)child_mem_start + 16 * j);
                // fprintf(out, " %s\n", (char*)child_mem_start + 16 * j + 8);
            }
            child_mem_start += 16 * num_of_words;
        }
        FILE *out;
        out = fopen(out_file_name, "w");
        for (int i = 0; i < num_of_words; i++) {
            int max = freq[0];
            int max_index = 0;
            for (int j = 0; j < unique_words; j++) {
                if (freq[j] > max) {
                    max = freq[j];
                    max_index = j;
                }
            }
            // Found max
            fprintf(out, "%s", words[max_index]);
            fprintf(out, " %d\n", max);
            freq[max_index] = 0;
        }
        fclose(out);
        printf("Parent\n");
        exit(0);
    }
    return 0;
}