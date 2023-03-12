#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define MEMORY_NAME "mem"
#define MAX_WORD_LENGTH 64
#define MAX_WORDS 1000

void findFreq(char* file_name, void *memory, int num_of_words) {
    FILE *fp;
    fp = fopen(file_name, "r");
    if (fp == NULL) {
        printf("Error: Unable to open file %s\n", file_name);
        exit(1);
    }

    char words[MAX_WORDS][MAX_WORD_LENGTH];
    int frequencies[MAX_WORDS] = {0};
    int num_words = 0;

    char word[MAX_WORD_LENGTH];
    while (fscanf(fp, "%s", word) != EOF) {
        // Convert the word to lowercase
        for (int i = 0; word[i]; i++) {
            word[i] = tolower(word[i]);
        }

        // Check if the word already exists in the array
        int found = 0;
        for (int i = 0; i < num_words; i++) {
            if (strcmp(word, words[i]) == 0) {
                frequencies[i]++;
                found = 1;
                break;
            }
        }

        // If the word is not found, add it to the array
        if (!found) {
            strncpy(words[num_words], word, MAX_WORD_LENGTH);
            frequencies[num_words] = 1;
            num_words++;
        }
    }

    int max = frequencies[0];
    int max_index = 0;
    for (int i = 0; i < num_of_words; i++) {
        max = frequencies[0];
        max_index = 0;
        for (int j = 0; j < num_words; j++) {
            if (frequencies[j] > max) {
                max = frequencies[j];
                max_index = j;
            }
        }
        sprintf(memory + i * 16, "%s", words[max_index]);
        char p[50];
        sprintf(p, "%d", max);
        sprintf(memory + i * 16 + 8, "%s", p);
        printf("Added to memory %s %s", words[max_index], p);
        frequencies[max_index] = 0;
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
                isUnique = 1;
                for (int wordIndex = 0; wordIndex < unique_words; wordIndex++) {
                    if (strcmp(words[wordIndex], (char*)child_mem_start + 16 * j) == 0) {
                        printf("Uniq değil");
                        isUnique = 0;
                        int fr = atoi((char*)child_mem_start + 16 * j + 8);
                        printf("%d\n", fr);
                        freq[wordIndex] += fr;
                        printf("%d\n", fr);
                    }
                }
                if (isUnique) {
                    printf("Uniq\n");
                    words[unique_words] = (char*)child_mem_start + 16 * j;
                    int fr = atoi((char*)child_mem_start + 16 * j + 8);
                    printf("%d\n", fr);
                    freq[unique_words] = fr;
                    unique_words++;
                }
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
