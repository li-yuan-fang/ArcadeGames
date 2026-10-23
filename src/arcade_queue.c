/*
 * arcade_queue.c
 *
 *  Created on: 2026/09/08
 *      Author: Admin
 */
#include "arcade_queue.h"

#include <errno.h>
#include <stdlib.h>

struct arcade_queue_node {
    void* ptr;
    struct arcade_queue_node* next;
};

void arcade_queue_init(arcade_queue_t* queue) {
    if (queue == NULL) return;

    queue->head = NULL;
    queue->tail = NULL;
}

int arcade_queue_enqueue(arcade_queue_t* queue, void* ptr) {
    arcade_queue_node_t* node;

    if (queue == NULL) return -EINVAL;
    if (ptr == NULL) return -EINVAL;

    node = (arcade_queue_node_t*)malloc(sizeof(arcade_queue_node_t));
    if (node == NULL) return -ENOMEM;

    node->ptr = ptr;
    node->next = NULL;

    if (queue->tail == NULL) {
        queue->head = node;
        queue->tail = node;
    } else {
        queue->tail->next = node;
        queue->tail = node;
    }

    return 0;
}

void* arcade_queue_dequeue(arcade_queue_t* queue) {
    arcade_queue_node_t* node;
    void* ptr;

    if (queue == NULL) return NULL;
    if (queue->head == NULL) return NULL;

    node = queue->head;
    ptr = node->ptr;
    queue->head = node->next;

    if (queue->head == NULL) queue->tail = NULL;

    free(node);

    return ptr;
}

bool arcade_queue_is_empty(arcade_queue_t* queue) {
    if (queue == NULL) return true;

    return queue->head == NULL;
}

void arcade_queue_clear(arcade_queue_t* queue) {
    arcade_queue_node_t* node;
    arcade_queue_node_t* next;

    if (queue == NULL) return;

    node = queue->head;
    while (node != NULL) {
        next = node->next;
        free(node);
        node = next;
    }

    queue->head = NULL;
    queue->tail = NULL;
}
