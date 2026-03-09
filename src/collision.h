#ifndef COLLISION_H
#define COLLISION_H

#define MAX_STEP_HEIGHT 2.5f


float get_height_at_point(float px, float py, float pz, int return_ceil, int check_middle_sprite);

float get_height_at_point_for_sprites(float px, float py, int return_ceil);



int collides(
    float spx, float spy, float spz, float px, float py, float pz, level this_level,
    int disable_collision, int editor_mode_enabled);
    
#endif 