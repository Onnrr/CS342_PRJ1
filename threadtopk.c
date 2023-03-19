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
    char word[64];
    int frequency;
    struct Node* next;
};

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

void *findFrequents(void *arguments) {
    struct args *args = arguments;
    FILE *f = fopen(args->file_name, "r");
    int num_of_words = args->num_of_words;
    int index = args->index;
    struct Node** root = &roots[index];
    *root = NULL;

    while (!feof(f)) {
        char word[64];
        fscanf(f, " %64s", word);

        // full uppercase
        for (int i = 0; word[i]!='\0'; i++) {
            if(word[i] >= 'a' && word[i] <= 'z') {
                word[i] = word[i] - 32;
            }   
        }

        struct Node* newNode = createNode(word, 1);
        distinctInsert(root, newNode);
    }
    fclose(f);
    insertionSort(root);
    trim(root, num_of_words);

    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    (void)argc;
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
            // printf("Thread cannot be created.");
        }
        else {
            // printf("Thread executing %s\n", file_name);
        }
    }

    for (int i = 0; i < num_of_files; i++) {
        pthread_join(tid[i], NULL);
    }

    struct Node* resultRoot = NULL;
    for (int i = 0; i < num_of_files; i++) {

        struct Node* current = roots[i];
        while (current != NULL) {
            struct Node* next = current->next;
            current->prev = current->next = NULL;
            distinctInsert(&resultRoot, current);
            current = next;
        }
    }
    insertionSort(&resultRoot);
    trim(&resultRoot, num_of_words);

    FILE *out;
    out = fopen(out_file_name, "w");
    struct Node* current = resultRoot;
    struct Node* next = resultRoot;

    while (current != NULL) {
        fprintf(out, "%s", current->word);
        fprintf(out, " %d\n", current->frequency);
        next = current->next;
        deleteNode(&resultRoot, current);
        current = next;
    }
    fclose(out);
    free(roots);
    return 0;
}