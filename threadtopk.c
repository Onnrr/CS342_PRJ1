#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>

struct Node** roots;

struct args {
    char* file_name;
    int num_of_words;
    int index;
};

struct Node {
    struct Node* prev;
    char* word;
    int frequency;
    struct Node* next;
};

struct Node* createNode(char* word, int frequency) {
    struct Node* newNode = (struct Node*) malloc(sizeof(struct Node));

    newNode->prev = newNode->next = NULL;
    newNode->word = word;
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
            cur = cur->next;
        }

        
        if (cur->next != NULL) {
            newNode->next = cur->next;
            newNode->next->prev = newNode;
            cur->next = newNode;
            newNode->prev = cur;
        }
        else {
            newNode->prev = cur;
            cur->next = newNode;
            cur = newNode;
        }
        
    }
}

void *findFrequents(void *arguments) {
    struct args *args = arguments;
    FILE *f = fopen(args->file_name, "r");
    int num_of_words = args->num_of_words;
    int index = args->index;
    printf("%d\n", index);
    char words[1000][64];
    int freq[1000] = {0};

    while (!feof(f)) {
        char word[64];
        fscanf(f, " %64s", word);

        // full uppercase
        for (int i = 0; word[i]!='\0'; i++) {
            if(word[i] >= 'a' && word[i] <= 'z') {
                word[i] = word[i] - 32;
            }   
        }

        for (int i = 0; i < 1000; i++) {
            if (strcmp(word, words[i]) == 0) {
                freq[i]++;
                break;
            }
            else if (freq[i] == 0) {
                strcpy(words[i], word);
                freq[i]++;
                break;
            }
        }
    }
    fclose(f);

    struct Node** root = &roots[index];
    for (int i = 0; i < 1000 && 0 != freq[i] != 0; i++) {
        struct Node* newNode = createNode(words[i], freq[i]);
        insert(root, newNode);
    }

    struct Node* cur = *root;
    int i = 0;
    while (cur->next != NULL) {
        if (i < num_of_words) {
            cur = cur->next;
            i++;
        }
        else {
            deleteNode(root, cur->next);
        }
    }
    deleteNode(root, cur);
    pthread_exit(0);
}

int main(int argc, char *argv[]) {

    int num_of_words = atoi(argv[1]);
    char* out_file_name = argv[2];
    int num_of_files = atoi(argv[3]);
    roots = malloc(num_of_files * sizeof(struct Node*));
    struct args arguments[num_of_files];

    pthread_t tid[num_of_files];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    for (int i = 0; i < num_of_files; i++) {
        char* file_name = argv[i + 4];
        arguments[i].file_name = file_name;
        arguments[i].num_of_words = num_of_words;
        arguments[i].index = i;
        
        int success = pthread_create(&tid[i], &attr, findFrequents, (void *)&arguments[i]);
        if (success != 0) {
            printf("Thread cannot be created.");
        }
        else {
            printf("Thread executing %s\n", file_name);
        }
    }

    for (int i = 0; i < num_of_files; i++) {
        pthread_join(tid[i], NULL);
    }

    for (int i = 0; i < num_of_files; i++) {
        
        for (struct Node* c = roots[i]; c != NULL; c = c->next) {
            printf("Word: %s, Frequency: %d\n", c->word, c->frequency);
        }
    }
    return 0;
}