#include <assert.h>

#include "event.h"

#define MAX_EVENTS 1024  // is this enough?
static event *queue;


static u32 read;
static u32 write;

u32 mask(u32 idx)  { 
    return idx & (MAX_EVENTS - 1); 
}

void push(event evt)  { 
    assert(!full()); queue[mask(write++)] = evt; 
}

event shift() { 
    assert(!empty()); 
    return queue[mask(read++)]; 
}

u32 empty()    { 
    return read == write; 
}

u32 full()     { 
    return size() == MAX_EVENTS; 
}

u32 size()     { 
    return write - read; 
}

void initialize_event_module() {
    queue = malloc(MAX_EVENTS*sizeof(event));
    read = 0;
    write = 0;
}