#include "queue.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

queue_t *queue_new(int size) {
    queue_t *q = malloc(sizeof(queue_t));
    q->size = size;
    q->count = 0;
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    return q;
}

bool queue_push(queue_t *q, void *elem) {
    if (q == NULL) {
        return false;
    }

    if (q->head == NULL) {
        pthread_mutex_lock(&q->mutex);
        node_t *node = malloc(sizeof(node_t));
        node->content = elem;
        node->next = NULL;
        node->prev = NULL;
        q->head = node;
        q->tail = node;
        q->count++;
        pthread_mutex_unlock(&q->mutex);
        return true;
    } else {
        pthread_mutex_lock(&q->mutex);
        node_t *node = malloc(sizeof(node_t));
        q->tail->next = node;
        node->content = elem;
        node->next = NULL;
        node->prev = q->tail;
        q->tail = node;
        q->count++;
        pthread_mutex_unlock(&q->mutex);
        return true;
    }
}

bool queue_pop(queue_t *q, void **elem) {
    if (q == NULL) {
        return false;
    }

    pthread_mutex_lock(&q->mutex);
    *elem = q->head->content;
    node_t *temp = q->head;
    q->head = q->head->next;
    free(temp);
    q->count--;
    pthread_mutex_unlock(&q->mutex);
    return true;
}

void queue_delete(queue_t **q) {
    if ((*q)->count != 0) {
        node_t *node = (*q)->head;
        while (node != NULL) {
            node_t *temp = node;
            node = node->next;
            free(temp);
        }
        pthread_mutex_destroy(&(*q)->mutex);
        free(*q);
    } else {
        free(*q);
    }
}

bool queue_empty(queue_t *q) {
    if (q->count == 0) {
        return true;
    } else {
        return false;
    }
}
