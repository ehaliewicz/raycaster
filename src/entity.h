#ifndef ENTITY_H
#define ENTITY_H

typedef enum {
    GATO,
    FOX,
    NUM_OBJ_TYPES
} obj_type;


void step_entities(float player_x, float player_y, float player_z);
void spawn_entity(int entity_type, float x, float y, float z, float ang);
#endif