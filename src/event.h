#ifndef EVENT_H
#define EVENT_H

#include "common.h"


typedef enum {
    MOVE_EVENT,
    EDIT_EVENT
} event_type;

typedef struct {
    edit_wall_id id;
    int keystroke;
} edit_event;

typedef struct {
    int keystroke;
} move_event;

// just 8 bytes?
typedef struct {
    union {
        edit_event edt;
        move_event mov;
    };
} event;


int push_event(event evt);

int pop_event(event* out);

#endif 