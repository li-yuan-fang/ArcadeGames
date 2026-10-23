/*
 * arcade_queue.h
 *
 *  Created on: 2026/09/08
 *      Author: Admin
 */

#ifndef SRC_ARCADE_QUEUE_H_
#define SRC_ARCADE_QUEUE_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct arcade_queue_node arcade_queue_node_t;

typedef struct {
    arcade_queue_node_t* head;
    arcade_queue_node_t* tail;
} arcade_queue_t;

void arcade_queue_init(arcade_queue_t* queue);

int arcade_queue_enqueue(arcade_queue_t* queue, void* ptr);

void* arcade_queue_dequeue(arcade_queue_t* queue);

bool arcade_queue_is_empty(arcade_queue_t* queue);

void arcade_queue_clear(arcade_queue_t* queue);

#ifdef __cplusplus
}
#endif

#endif /* SRC_ARCADE_QUEUE_H_ */
