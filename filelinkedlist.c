#include "filelinkedlist.h"

linkedlist* create_linkedlist() {
    linkedlist *list = malloc(sizeof(linkedlist));
    list->head = NULL;
    pthread_mutex_init(&list->lock, NULL);
    return list;
}

void* insert(linkedlist *list, char *filename) {
    while (pthread_mutex_trylock(&list->lock) != 0) {
        sleep(1);
    }
    node *current = list->head;
    while (current != NULL) {
        if (strcmp(current->filename, filename) == 0) {
            pthread_mutex_unlock(&list->lock);
            sleep(1);
            return insert(list, filename);
        }
        current = current->next;
    }
    node *new_node = malloc(sizeof(node));
    new_node->filename = filename;
    new_node->next = list->head;
    list->head = new_node;
    pthread_mutex_unlock(&list->lock);
    return NULL;
}

void delete(linkedlist *list, char *filename) {
    while (pthread_mutex_trylock(&list->lock) != 0) {
        sleep(1);
    }
    node *current = list->head;
    node *prev = NULL;
    while (current != NULL) {
        if (strcmp(current->filename, filename) == 0) {
            if (prev == NULL) {
                list->head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            pthread_mutex_unlock(&list->lock);
            return;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&list->lock);
}

void delete_linkedlist(linkedlist *list) {
    while (pthread_mutex_trylock(&list->lock) != 0) {
        sleep(1);
    }
    node *current = list->head;
    node *next = NULL;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    pthread_mutex_unlock(&list->lock);
    pthread_mutex_destroy(&list->lock);
    free(list);
}

bool empty(linkedlist *list) {
    while (pthread_mutex_trylock(&list->lock) != 0) {
        sleep(1);
    }
    bool empty = list->head == NULL;
    pthread_mutex_unlock(&list->lock);
    return empty;
}
