#include <pthread.h>
#include <stdbool.h>

typedef struct node_t {
    void *content;
    struct node_t *next;
    struct node_t *prev;
} node_t;

typedef struct queue_t {
    node_t *head;
    node_t *tail;
    int size;
    int count;
    pthread_mutex_t mutex;
} queue_t;

queue_t *queue_new(int size);
void queue_delete(queue_t **q);
bool queue_push(queue_t *q, void *elem);
bool queue_pop(queue_t *q, void **elem);
