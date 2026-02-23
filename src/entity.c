// we're just gonna do it doom style

// a giant state table for all entities


// a state has an action, a next state,


// big state table
#include "common.h"

typedef enum {
    GATO,
    NUM_OBJ_TYPES
} obj_type;

typedef enum {
    GATO_WANDER1,
    GATO_WANDER2,
    GATO_WANDER3,
    GATO_WANDER4,
    GATO_FOLLOW,
    NUM_STATES
} obj_state;

typedef struct {
    int(*func)(obj* actor);
    int spr_idx;
    int next_state;
    int ticks_til_next_state;
} obj_state;

typedef struct {
    int start_state;
    int see_state; // see player state
    int see_sound; // see player sound
    int react_time; // reaction time
    int attack_sound;
    int pain_state;
    int pain_chance;
    int melee_state;
    int missile_state;
    int death_state;
    int death_sound;
    int speed;
    int radius;
    int height;
    int mass;
    int damage;
    int active_sound;
    int flags;
    //int raise_state; 
} obj_tmpl;

// total hack lol
#define NO_TARGET -1
#define PLAYER_TARGET -2

typedef struct {
    int hp;
    int state_idx;
    int ticks_til_next_state;
    int target;
} obj;

int (*p[NUM_OBJ_TYPES]) (obj* o);

obj_tmpl obj_tmps[NUM_OBJ_TYPES] = {
    {
        .start_state = GATO_WANDER,
        .see_state = GATO_FOLLOW,
        .see_sound = -1,
        .react_time = 1,
        .speed = 10,
        .radius = 2,
        .height = 2,
        .active_sound = -1
    }
};

#define MAX_ENTITIES 256

int num_alive_entities = 0;

obj_state states[NUM_STATES] = {
    // GATO_WANDER
    {
        .func = &move_random_dir,
        .spr_idx = GATO_WANDER_SPR,
        .ticks_til_next_state = 20,
        .next_state = GATO_WANDER1,
    }
    // GATO_FOLLOW
    {
        .func = &follow_target,
        .spr_idx = GATO_FOLLOW_1,
        .ticks_til_next_state = 20,
        .next_state = GATO_WANDER2,

    }
};


obj entities[MAX_ENTITIES] = {

};

int step_entities() {
    for(int i = 0; i < num_alive_entities; i++) {
        if(entities[i].ticks_til_next_state <= 0) {
            // execute
            // set ticks based on state
            obj ent = entities[i];
            state obj_state = [ent.state_idx];

        }
    }
}