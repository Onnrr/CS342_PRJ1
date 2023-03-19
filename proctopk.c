#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define MEMORY_NAME "mem"
#define MAX_WORD_LENGTH 64

struct Node {
    struct Node* prev;
    char word[64];
    int frequency;
    struct Node* next;
};

void displayList(struct Node* root) {
    if (root == NULL) {
        printf("List is empty.\n");
    }
    else {
        struct Node* current = root;

        while (current != NULL) {
            printf("Word: %s, Frequency: %d\n", current->word, current->frequency);
            current = current->next;
        }
        printf("\n");
    }
}

struct Node* createNode(char word[64], int frequency) {
    struct Node* newNode = (struct Node*) malloc(sizeof(struct Node));

    newNode->prev = newNode->next = NULL;
    strcpy(newNode->word, word);
    newNode->frequency = frequency;
    return newNode;
}

void deleteNode(struct Node** root, struct Node* deleteNode) {

    // empty list
    if (*root == NULL || deleteNode == NULL)
        return;
    else {
        // delete head
        if (*root == deleteNode)
            *root = deleteNode->next;

        // not the last node
        if (deleteNode->next != NULL)
            deleteNode->next->prev = deleteNode->prev;

        // not the first node
        if (deleteNode->prev != NULL)
            deleteNode->prev->next = deleteNode->next;

        // free memory
        free(deleteNode);
    }
}

void distinctInsert(struct Node** root, struct Node* newNode) {

    // check if list is empty
    if (*root == NULL) {
        *root = newNode;
    }
    else {
        struct Node* cur = *root;

        while (cur->next != NULL && strcmp(cur->word, newNode->word) != 0) {
            cur = cur->next;
        }

        if (strcmp(cur->word, newNode->word) == 0) {
            cur->frequency += newNode->frequency;
            free(newNode);
        }
        else {
            newNode->prev = cur;
            cur->next = newNode;
            cur = newNode;
        }
        
    }
}

void insert(struct Node** root, struct Node* newNode) {

    // check if list is empty
    if (*root == NULL) {
        *root = newNode;
    }

    // insert to begining
    else if ((*root)->frequency < newNode->frequency) {
        newNode->next = *root;
        newNode->next->prev = newNode;
        *root = newNode;
    }

    else {
        // insert in the middle or end
        struct Node* cur = *root;

        while (cur->next != NULL && cur->next->frequency >= newNode->frequency) {
            if (cur->next->frequency == newNode->frequency && strcmp(cur->next->word, newNode->word) > 0) {
                break;
            }
            cur = cur->next;
        }

        if (cur->next != NULL) {
            newNode->next = cur->next;
            newNode->next->prev = newNode;
            cur->next = newNode;
            newNode->prev = cur;
        }
        else {
            newNode->next = cur->next;
            cur->next = newNode;
            newNode->prev = cur;
        }

    }
}

void insertionSort(struct Node** root) {
    struct Node* sorted = NULL;
    struct Node* current = *root;

    while (current != NULL) {
        struct Node* next = current->next;
        current->prev = current->next = NULL;
        insert(&sorted, current);
        current = next;
    }

    *root = sorted;
}

void trim(struct Node** root, int n) {
    if ((*root) != NULL) {
        struct Node* current = *root;

        for (int i = 1; i < n; i++) {
            if (current->next == NULL) {
                return;
            }
            current = current->next;
        }

        while (current->next != NULL) {
            deleteNode(root, current->next);
        }
    }
}

void findFreq(char* file_name, void *memory, int num_of_words) {
    FILE *fp;
    fp = fopen(file_name, "r");

    struct Node* root = NULL;

    char word[MAX_WORD_LENGTH];
    while (fscanf(fp, "%64s", word) != EOF) {
        // full uppercase
        for (int i = 0; word[i]!='\0'; i++) {
            if(word[i] >= 'a' && word[i] <= 'z') {
                word[i] = word[i] - 32;
            }   
        }

        struct Node* newNode = createNode(word, 1);
        distinctInsert(&root, newNode);
        
    }
    fclose(fp);
    insertionSort(&root);
    trim(&root, num_of_words);

    struct Node *ptr = root;
    for (int i = 0; i < num_of_words && ptr != NULL; i++) {
        sprintf(memory + i * (sizeof(char[64]) + 8), "%s", ptr->word);
        sprintf(memory + i * (sizeof(char[64]) + 8) + sizeof(char[64]), "%d", ptr->frequency);
        ptr = ptr->next;
    }

    struct Node* current = root;
    while (current != NULL) {
        struct Node* next = current->next;
        free(current);
        current = next;
    }
}


int main(int argc, char *argv[]) {

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    pid_t pid;
    (void)argc;
    int num_of_words = atoi(argv[1]);
    char* out_file_name = argv[2];
    int num_of_files = atoi(argv[3]);

    const int size = 10000;
    int shm_fd;
    void *ptr, *file_name_ptr, *child_ptr, *start, *child_mem_start;

    shm_fd = shm_open(MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, size);

    ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    start = file_name_ptr = ptr;
    child_mem_start = child_ptr = start + num_of_files * sizeof(char[64]);

    for (int i = 0; i < num_of_files; i++) {
        sprintf(ptr + sizeof(char[64]) * i, "%s", argv[4 + i]);
    }

    for (int i = 0; i < num_of_files; i++) {
        pid = fork();
        if (pid < 0) {
            // Fork failed
            exit(-1);
        }
        if (pid == 0) {
            char* file_name = (char*)(file_name_ptr + (sizeof(char[64]) * i));

            findFreq(file_name, child_ptr + (sizeof(char[64]) + 8) * num_of_words * i,num_of_words);

            break;
        }
    }
    if (pid != 0) {

        for (int i = 0; i < num_of_files; i++) {
            wait(NULL);
        }

        struct Node* resultRoot = NULL;
        for (int i = 0; i < num_of_files; i++) {
            for (int j = 0; j < num_of_words; j++) {
                char* word = (char*)child_mem_start + (sizeof(char[64]) + 8) * j;
                int fr = atoi((char*)child_mem_start + (sizeof(char[64]) + 8) * j + sizeof(char[64]));
                struct Node* newNode = createNode(word, fr);
                distinctInsert(&resultRoot, newNode);
            }
            child_mem_start += (sizeof(char[64]) + 8) * num_of_words;
        }
        insertionSort(&resultRoot);

        
        trim(&resultRoot, num_of_words);


        FILE *out;
        out = fopen(out_file_name, "w");
        struct Node* current = resultRoot;
        struct Node* next = resultRoot;


        while (current != NULL) {
            fprintf(out, "%s", current->word);
            fprintf(out, " %d", current->frequency);
            next = current->next;
            deleteNode(&resultRoot, current);
            current = next;
            if (current != NULL) {
                fprintf(out, "\n");
            }
        }
        fclose(out);

        exit(0);
    }

    gettimeofday(&end_time, NULL);
    printf("Time elapsed \nseconds: %ld\nmicroseconds: %ld\n", end_time.tv_sec - start_time.tv_sec, end_time.tv_usec - start_time.tv_usec);
    return 0;
}