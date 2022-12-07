#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

typedef struct node {
    struct node *next;
    char *filename;
} node;

typedef struct linkedlist {
    node *head;
    pthread_mutex_t lock;
} linkedlist;

linkedlist* create_linkedlist();
void* insert(linkedlist *list, char *filename);
void delete(linkedlist *list, char *filename);
void delete_linkedlist(linkedlist *list);
bool empty(linkedlist *list);
