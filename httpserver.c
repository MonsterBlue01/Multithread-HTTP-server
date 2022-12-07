#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <sys/file.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "bind.h"
#include "queue.h"
#include "httpserver.h"
#include "filelinkedlist.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
sem_t sem;
int audit = -1;
queue_t *q;
linkedlist *list;
int term = 0;

void sigterm_handler() {
    if (audit != -1) {
        term = 1;
    }
}

uint16_t strtouint16(char number[]) {
    char *last;
    long num = strtol(number, &last, 10);
    if (num <= 0 || num > UINT16_MAX || *last != '\0') {
        return 0;
    }
    return num;
}

#define BUF_SIZE 4096 * 2
void producer(string_int *si) {
    printf("producer: connfd = %d\n", si->integer);
    void *elem = malloc(sizeof(string_int));
    memcpy(elem, si, sizeof(string_int));
    queue_push(q, elem);
    pthread_cond_signal(&cond);
}

void* consumer() {
    while (1) {
        if (q->count == 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        void *elem;
        queue_pop(q, &elem);
        string_int *si = (string_int *)elem;
        int connfd = si->integer;
        char *saveptr;
        char *line = strtok_r(si->string, "\r\n", &saveptr);
        char *line2 = malloc(strlen(line) + 1);
        strcpy(line2, line);
        char *header_field = malloc(BUF_SIZE);
        char *message_body = malloc(BUF_SIZE);
        int after_header = 0;
        while (line != NULL) {
            line = strtok_r(NULL, "\r\n", &saveptr);
            if (line != NULL) {
                if (strstr(line, ":") == NULL) {
                    after_header = 1;
                }
                if (after_header == 0) {
                    strcat(header_field, line);
                    strcat(header_field, "\r\n");
                } else {
                    strcat(message_body, line);
                    strcat(message_body, "\r\n");
                }
            }
        }
        char *method = strtok(line2, " ");
        char *path = strtok(NULL, " ");
        char *version = strtok(NULL, " ");
        int request_id = 0;
        char *request_id_str = strstr(header_field, "Request-Id: ");
        if (request_id_str != NULL) {
            request_id_str += strlen("Request-Id: ");
            request_id = atoi(request_id_str);
        }
        printf("Request-Id: %d", request_id);
        printf("method: %s\n", method);
        printf("path: %s\n", path);
        printf("version: %s\n", version);
        printf("consumer: header_field = %s\n", header_field);
        printf("consumer: message_body = %s\n", message_body);
        if (strcmp(method, "GET") == 0) {
            char *file_path = malloc(strlen(path) + 1);
            strcpy(file_path, path);
            file_path = file_path + 1;
            printf("file_path: %s\n", file_path);
            int fd = open(file_path, O_RDONLY);
            if (errno == ENOENT) {
                flock(audit, LOCK_EX);
                char *audit_str = malloc(BUF_SIZE);
                sprintf(audit_str, "%s,%s,%d,%d\n", method, path, 404, request_id);
                write(audit, audit_str, strlen(audit_str));
                flock(audit, LOCK_UN);
                char *response = "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n";
                write(connfd, response, strlen(response));
                errno = 0;
            } else if (errno == EACCES) {
                flock(audit, LOCK_EX);
                char *audit_str = malloc(BUF_SIZE);
                sprintf(audit_str, "%s,%s,%d,%d\n", method, path, 403, request_id);
                write(audit, audit_str, strlen(audit_str));
                flock(audit, LOCK_UN);
                char *response = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
                write(connfd, response, strlen(response));
                errno = 0;
            } else {
                char *file_content = malloc(BUF_SIZE);
                insert(list, file_path);
                read(fd, file_content, BUF_SIZE);
                delete(list, file_path);
                flock(audit, LOCK_EX);
                char *audit_str = malloc(BUF_SIZE);
                sprintf(audit_str, "%s,%s,%d,%d\n", method, path, 200, request_id);
                write(audit, audit_str, strlen(audit_str));
                flock(audit, LOCK_UN);
                int file_size = lseek(fd, 0, SEEK_END);
                char *response = malloc(BUF_SIZE);
                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s", file_size, file_content);
                write(connfd, response, strlen(response));
                free(file_content);
                free(response);
                close(fd);
            }
        } else if (strcmp(method, "PUT") == 0) {
            printf("PUT request\n");
            char* file_path = malloc(strlen(path) + 1);
            strcpy(file_path, path);
            file_path = file_path + 1;
            printf("file_path: %s\n", file_path);
            printf("message_body: %s\n", message_body);
            int fd1 = open(file_path, O_WRONLY | O_TRUNC, 0644);
            if (errno == EACCES) {
                flock(audit, LOCK_EX);
                char *audit_str = malloc(BUF_SIZE);
                sprintf(audit_str, "%s,%s,%d,%d\n", method, path, 403, request_id);
                write(audit, audit_str, strlen(audit_str));
                flock(audit, LOCK_UN);
                char *response = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
                write(connfd, response, strlen(response));
                errno = 0;
                close(connfd);
            } else if (errno == ENOENT) {
                close(fd1);
                flock(audit, LOCK_EX);
                char *audit_str = malloc(BUF_SIZE);
                sprintf(audit_str, "%s,%s,%d,%d\n", method, path, 201, request_id);
                write(audit, audit_str, strlen(audit_str));
                flock(audit, LOCK_UN);
                char *response = "HTTP/1.1 201 Created\r\nContent-Length: 7\r\n\r\nCreated\n";
                int fd1 = open(file_path, O_WRONLY | O_CREAT, 0644);
                insert(list, file_path);
                write(fd1, message_body, strlen(message_body));
                delete(list, file_path);
                write(connfd, response, strlen(response));
                errno = 0;
                close(connfd);
            } else {
                flock(audit, LOCK_EX);
                char *audit_str = malloc(BUF_SIZE);
                sprintf(audit_str, "%s,%s,%d,%d\n", method, path, 200, request_id);
                write(audit, audit_str, strlen(audit_str));
                flock(audit, LOCK_UN);
                char *response = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n";
                write(connfd, response, strlen(response));
                insert(list, file_path);
                write(fd1, message_body, strlen(message_body));
                delete(list, file_path);
                close(connfd);
            }
        } else if (strcmp(method, "HEAD") == 0) {
            printf("HEAD request\n");
            char* file_path = malloc(strlen(path) + 1);
            strcpy(file_path, path);
            file_path = file_path + 1;
            printf("file_path: %s\n", file_path);
            int fd = open(file_path, O_RDONLY);
            if (errno == ENOENT) {
                flock(audit, LOCK_EX);
                char *audit_str = malloc(BUF_SIZE);
                sprintf(audit_str, "%s,%s,%d,%d\n", method, path, 404, request_id);
                write(audit, audit_str, strlen(audit_str));
                flock(audit, LOCK_UN);
                char *response = "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n";
                write(connfd, response, strlen(response));
                errno = 0;
            } else if (errno == EACCES) {
                flock(audit, LOCK_EX);
                char *audit_str = malloc(BUF_SIZE);
                sprintf(audit_str, "%s,%s,%d,%d\n", method, path, 403, request_id);
                write(audit, audit_str, strlen(audit_str));
                flock(audit, LOCK_UN);
                char *response = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
                write(connfd, response, strlen(response));
                errno = 0;
            } else {
                insert(list, file_path);
                int file_size = lseek(fd, 0, SEEK_END);
                delete(list, file_path);
                flock(audit, LOCK_EX);
                char *audit_str = malloc(BUF_SIZE);
                sprintf(audit_str, "%s,%s,%d,%d\n", method, path, 200, request_id);
                write(audit, audit_str, strlen(audit_str));
                flock(audit, LOCK_UN);
                char *response = malloc(BUF_SIZE);
                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", file_size);
                write(connfd, response, strlen(response));
                free(response);
                close(fd);
            }
            close(connfd);
        } else {
            printf("Received an invalid request\n");
            char *response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n";
            write(connfd, response, strlen(response));
            close(connfd);
        }
    }
}

int main(int argc, char *argv[]) {
    int listenfd;
    uint16_t port = 0;
    q = queue_new(10);
    int num_threads = 1;
    char *filename = NULL;
    signal(SIGTERM, sigterm_handler);
    list = create_linkedlist();
    if ((argc != 2) && (argc != 4) && (argc != 6)) {
        warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "Usage: %s [-t threads] [-l logfile] <port>\n", argv[0]);
        exit(1);
    }
    
    int opt;
    while ((opt = getopt(argc, argv, "t:l:")) != -1) {
        switch (opt) {
        case 't':
            num_threads = atoi(optarg) - 1;
            break;
        case 'l':
            filename = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s [-t threads] [-l logfile] <port>\n", argv[0]);
            exit(1);
        }
    }
    pthread_t threads[num_threads];
    if (filename != NULL) {
        printf("filename: %s\n", filename);
        audit = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else {
        printf("filename: NULL\n");
    }
    
    if (argc == 2) {
        port = strtouint16(argv[1]);
    } else if (argc == 4) {
        port = strtouint16(argv[3]);
    } else if (argc == 6) {
        port = strtouint16(argv[5]);
    }
    listenfd = create_listen_socket(port);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, consumer, NULL);
    }

    listen(listenfd, 10);

    while(1) {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) {
            fprintf(stderr, "bind: accept error: Address already in use\n");
            exit(1);
        }
        char buf[BUF_SIZE];
        int n = read(connfd, buf, BUF_SIZE);
        if (n < 0) {
            fprintf(stderr, "read error: %s\n", strerror(errno));
            exit(1);
        }
        buf[n] = '\0';
        string_int *si = malloc(sizeof(string_int));
        si->integer = connfd;
        si->string = malloc(strlen(buf) + 1);
        strcpy(si->string, buf);
        printf("Request:\n%s\n", buf);
        if (term != 1) {
            producer(si);
        }
        if ((term == 1) && (empty(list)) == 1) {
            delete_linkedlist(list);
            queue_delete(&q);
            close(listenfd);
            close(audit);
            exit(0);
        }
    }
}
