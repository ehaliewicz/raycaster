#include <stdio.h>
#include <stdlib.h>
//#include <math.h>

#include "common.h"
#include "entity.h"
#include "my_defs.h"
#include "raycast.h"

typedef enum {
    GATO_WANDER1=0,
    GATO_WANDER2=1,
    GATO_WANDER3=2,
    GATO_WANDER4=3,
    GATO_FOLLOW1=4,
    GATO_FOLLOW2=5,
    GATO_FOLLOW3=6,
    GATO_FOLLOW4=7,
    FOX_WANDER1=8,
    FOX_WANDER2=9,
    FOX_FOLLOW1=10,
    FOX_FOLLOW2=11,
    NUM_STATES
} obj_states;


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
    float wander_speed;
    float chase_speed;
    float radius;
    float height;
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
    int type;
    int hp;
    int state_idx;
    int ticks_til_next_state;
    int target;
    int in_use;
    int alive;
    float x, y, z;
    float lstx, lsty, lstz;
    float ang;
} obj;


typedef struct {
    void(*func)(obj* o, float player_x, float player_y, float player_z);
    int spr_idx;
    int next_state;
    int ticks_til_next_state;
} obj_state;

void (*p[NUM_OBJ_TYPES]) (obj* o, float player_x, float player_y, float player_z);

obj_tmpl obj_tmps[NUM_OBJ_TYPES] = {
    {
        .start_state = GATO_WANDER1,
        .see_state = GATO_FOLLOW1,
        .see_sound = -1,
        .react_time = 1,
        .wander_speed = 0.025f,
        .chase_speed = 0.1f,
        .radius = 0.2,
        .height = 2,
        .active_sound = -1
    },
    {
        .start_state = FOX_WANDER1,
        .see_state = FOX_FOLLOW1,
        .see_sound = -1,
        .react_time = 1,
        .wander_speed = 0.020f,
        .chase_speed = 0.08f,
        .radius = 0.2,
        .height = 2,
        .active_sound = -1
    }
};


#define MAX_ENTITIES 2048

int num_allocated_entities = 0;

obj *entities = NULL; 


int can_raycast_between_points(int start_x, int start_y, int start_z, int end_x, int end_y, int end_z) {
    float dx = end_x - start_x;
    float dy = end_y - start_y;
    float dz = end_z - start_z;

    float len = my_sqrtf(dx*dx+dy*dy+dz*dz);
    float vx = dx / len;
    float vy = dy / len;
    float vz = dz / len;
    int steps = my_floorf(len)+1;

    float x = start_x;
    float y = start_y;
    float z = start_z;
    x += vx;
    y += vy;
    z += vz;
    for(int step = 1; step < steps; step++) {
        //debug_printf("test position %f, %f, %f\n", x, y, z);
        float floor_height = get_height_at_point(x, y, z, 0, 0);
        float ceil_height = get_height_at_point(x, y, z, 1, 0);
        if(floor_height >= z) {
            //debug_printf("floor %f blocked entity line of sight\n", floor_height);
            return 0;
        } else {
            //debug_printf("floor %f didn't block entity line of sight\n", floor_height);
        }
        if(ceil_height <= z) {
            //debug_printf("ceil blocked entity line of sight\n", ceil_height);
            return 0;
        } else {
            //debug_printf("ceil %f didn't block entity line of sight\n", ceil_height);
        }
        if((int)(x) == end_x && (int)(y) == end_y && (int)(z) == end_z) {
            //debug_printf("raycast successfully\n");
            return 1;
        }
        x += vx;
        y += vy;
        z += vz;
    }   
    //debug_printf("last position %f, %f, %f\n", x, y, z);
    return 1;
}

int collides_with_other_entity(obj* actor, float test_x, float test_y) {
    for(int i = 0; i < num_allocated_entities; i++) {
        if(&entities[i] == actor) {
            continue; // skip
        }
        float dx = entities[i].x - test_x;
        float dy = entities[i].y - test_y;
        float combined_radius = obj_tmps[entities[i].type].radius + obj_tmps[actor->type].radius;
        float squared_min_dist = combined_radius*combined_radius;

        float sqlen = (dx*dx+dy*dy);
        if(sqlen < squared_min_dist) {
            return 1;
        }
    }
    return 0;
}

void move_random_dir(obj* actor) {
    int rand_ang = rand()%256;
    float rang = 6.28f*((float)rand_ang)/256.0f;
    float y = my_sinf(rang)*obj_tmps[actor->type].wander_speed;
    float x = my_cosf(rang)*obj_tmps[actor->type].wander_speed;
    float floor_height = get_height_at_point(actor->x+x, actor->y+y, actor->z, 0, 1);
    float ceil_height = get_height_at_point(actor->x+x, actor->y+y, actor->z, 1, 1);


    if(ceil_height < actor->z + 2.0f) {
        return;
    }
    if(floor_height > actor->z+2.0f) {
        return;
    }
    if(collides_with_other_entity(actor, actor->x+x, actor->y+y)) {
        return;
    }

    actor->x +=x;
    actor->y += y;
    actor->z = floor_height;
}

void move_towards_last_seen_target_pos(obj* actor) {
    //int rand_ang = rand()%256;
    //float rang = 6.28f*((float)rand_ang)/256.0f;

    float dx = actor->lstx - actor->x;
    float dy = actor->lsty - actor->y;
    float dz = actor->lstz - actor->z;

    float sq_len = dx*dx+dy*dy;// sqrtf(dx*dx+dy*dy);
    float ang = my_atan2f(dy, dx);
    float vy = my_sinf(ang);
    float vx = my_cosf(ang); // terrible, use sqrt wtf
    if(sq_len <= 0.01f && dz <= .1f) {
        game_over();
    }
    //if(sq_len < 0.1f) {
    //    return;
    //}
    //float vx = dx/sq_len;
    //float vy = dy/sq_len;

    float y = vy * obj_tmps[actor->type].chase_speed;
    float x = vx * obj_tmps[actor->type].chase_speed;

    float floor_height = get_height_at_point(actor->x+x, actor->y+y, actor->z, 0, 1);
    float ceil_height = get_height_at_point(actor->x+x, actor->y+y, actor->z, 1, 1);
    if(ceil_height < actor->z + 2.0f) {
        //debug_printf("moving randomly\n");
        move_random_dir(actor);
        return;
    }
    if(floor_height > actor->z+2.0f) {
        //debug_printf("moving randomly\n");
        move_random_dir(actor);
        return;
    }
    if(collides_with_other_entity(actor, actor->x+x, actor->y+y)) {
        move_random_dir(actor);
        return;
    }
    actor->x +=x;
    actor->y += y;
    actor->z = floor_height;
}

void wakeup_entity(obj* actor, float player_x, float player_y, float player_z) {
    //debug_printf("wakeup entity!!!!\n");
    actor->state_idx = obj_tmps[actor->type].see_state;
    //debug_printf("going to state %i\n", actor->state_idx);
    actor->target = PLAYER_TARGET;
    actor->lstx = player_x;
    actor->lsty = player_y;
    actor->lstz = player_z;

    actor->ticks_til_next_state = obj_tmps[actor->type].react_time + rand()&15;
    //debug_printf("WAKEUP!!!!\n");
}

void look_and_wander(obj* actor, float player_x, float player_y, float player_z) {
    // if asleep, check for line of sight and wake up
    if(0) { // can_raycast_between_points(actor->x, actor->y, actor->z, player_x, player_y, player_z)) {
        wakeup_entity(actor, player_x, player_y, player_z);

    } else {
        //move_random_dir(actor);
        actor->z = get_height_at_point(actor->x, actor->y, actor->z, 0, 1);
    }
}

void wakeup_entities(float player_x, float player_y, float player_z) {
    for(int i = 0; i < num_allocated_entities; i++) {
        wakeup_entity(&entities[i], player_x, player_y, player_z);
    }
}

void follow_target(obj* actor, float player_x, float player_y, float player_z) {
    if(actor->target == PLAYER_TARGET && can_raycast_between_points(actor->x, actor->y, actor->z, player_x, player_y, player_z)) {
        actor->lstx = player_x;
        actor->lsty = player_y;
        actor->lstz = player_z - cur_player_height;
    }

    move_towards_last_seen_target_pos(actor);

    //debug_printf("follow target!\n");
}

typedef enum {
    GATO_SPR1 = 9,
    GATO_SPR2 = 10,
    GATO_SPR3 = 11,
    GATO_SPR4 = 12,
    GATO_SPR5 = 13,
    GATO_SPR6 = 14,
    GATO_SPR7 = 15,
    GATO_SPR8 = 16,
    GATO_SPR9 = 17,
    GATO_SPR10 = 18,
    FOX_SPR1 = 19,
    FOX_SPR2 = 20
} entity_sprites;

obj_state states[NUM_STATES] = {
    // GATO_WANDER1
    {
        .func = &look_and_wander,
        .spr_idx = GATO_SPR1,
        .ticks_til_next_state = 10,
        .next_state = GATO_WANDER1
    },
    // GATO_WANDER2
    {
        .func = &look_and_wander,
        .spr_idx = GATO_SPR2,
        .ticks_til_next_state = 10,
        .next_state = GATO_WANDER2
    },
    // GATO_WANDER3
    {
        .func = &look_and_wander,
        .spr_idx = GATO_SPR3,
        .ticks_til_next_state = 10,
        .next_state = GATO_WANDER3
    },
    // GATO_WANDER4
    {
        .func = &look_and_wander,
        .spr_idx = GATO_SPR4,
        .ticks_til_next_state = 10,
        .next_state = GATO_WANDER1
    },

    // GATO_FOLLOW1
    {
        .func = &follow_target,
        .spr_idx = GATO_SPR5,
        .ticks_til_next_state = 10,
        .next_state = GATO_FOLLOW2
    },

    // GATO_FOLLOW2
    {
        .func = &follow_target,
        .spr_idx = GATO_SPR6,
        .ticks_til_next_state = 10,
        .next_state = GATO_FOLLOW3
    },
    // GATO_FOLLOW3
    {
        .func = &follow_target,
        .spr_idx = GATO_SPR7,
        .ticks_til_next_state = 10,
        .next_state = GATO_FOLLOW4
    },
    // GATO_FOLLOW4
    {
        .func = &follow_target,
        .spr_idx = GATO_SPR7,
        .ticks_til_next_state = 10,
        .next_state = GATO_FOLLOW1
    },
    // FOX_WANDER1
    {
        .func = &look_and_wander,
        .spr_idx = FOX_SPR1,
        .ticks_til_next_state = 10,
        .next_state = FOX_WANDER2
    },
    // FOX_WANDER2
    {
        .func = &look_and_wander,
        .spr_idx = FOX_SPR2,
        .ticks_til_next_state = 10,
        .next_state = FOX_WANDER1
    },
    // FOX_FOLLOW1
    {
        .func = &follow_target,
        .spr_idx = FOX_SPR1,
        .ticks_til_next_state = 10,
        .next_state = FOX_FOLLOW2
    },
    // FOX_FOLLOW2
    {
        .func = &follow_target,
        .spr_idx = FOX_SPR2,
        .ticks_til_next_state = 10,
        .next_state = FOX_FOLLOW1
    },
};

void damage_entity(int damage, int entity_id) {
    if(entity_id > MAX_ENTITIES) {
        return;
    }
    entities[entity_id].alive = 0;
    for(int i = 0; i < num_allocated_entities; i++) {
        if(entities[i].alive) {
            return;
        }
    }
    you_win();
}


void step_entities(float player_x, float player_y, float player_z) {
    //int stepped = 0;
    for(int i = 0; i < num_allocated_entities; i++) {
        obj_state state = states[entities[i].state_idx];
        if(entities[i].alive == 0) {
            continue;
        }
        if(entities[i].ticks_til_next_state <= 0) {
            //stepped++;
            // execute
            // set ticks based on state
            entities[i].state_idx = state.next_state;
            state = states[entities[i].state_idx];
            entities[i].ticks_til_next_state = state.ticks_til_next_state;

            // execute func afterwards to override the above state/timer update
            state.func(&entities[i], player_x, player_y, player_z);
        

        } else {
            entities[i].ticks_til_next_state--;
        }
    }
}

void draw_entities() {
    for(int i = 0; i < num_allocated_entities; i++) {
        obj_state state = states[entities[i].state_idx];
        if(entities[i].alive == 0) {
            continue;
        }
        request_draw_sprite(entities[i].x, entities[i].y, entities[i].z, i, state.spr_idx);
    }
}

void spawn_entity(int entity_type, float x, float y, float z, float ang, int ticks) {
    if(num_allocated_entities < MAX_ENTITIES) {
        entities[num_allocated_entities].type = entity_type;
        entities[num_allocated_entities].alive = 1;
        entities[num_allocated_entities].in_use = 1;
        entities[num_allocated_entities].state_idx = obj_tmps[entity_type].start_state;
        entities[num_allocated_entities].ticks_til_next_state = ticks;
        entities[num_allocated_entities].x = x;
        entities[num_allocated_entities].y = y;
        entities[num_allocated_entities].z = z;
        entities[num_allocated_entities].ang = ang;
        entities[num_allocated_entities++].target = NO_TARGET;
    }
}


void init_entities_module() {
    entities = my_malloc(sizeof(obj)*MAX_ENTITIES, "entities array");
    num_allocated_entities = 0;

}