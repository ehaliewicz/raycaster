#include "assert.h"
#if defined(PLATFORM_WEB)
#include "emscripten.h"
#include <emscripten/html5.h>
#endif 
#include "math.h"   
#include "raylib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"



#define FP_SCREEN_WIDTH (1280)
#define FP_SCREEN_HEIGHT (720)
#define OUTPUT_WIDTH (1280)
#define OUTPUT_HEIGHT (720)

typedef enum {
    WALL_SIDE_TOP,
    WALL_SIDE_UPPER_TOP,
    WALL_SIDE_BOTTOM,
    WALL_SIDE_UPPER_BOTTOM,
    WALL_SIDE_UPPER_NORTH,
    WALL_SIDE_UPPER_EAST,
    WALL_SIDE_UPPER_SOUTH,
    WALL_SIDE_UPPER_WEST,
    WALL_SIDE_LOWER_NORTH,
    WALL_SIDE_LOWER_EAST,
    WALL_SIDE_LOWER_SOUTH,
    WALL_SIDE_LOWER_WEST,
    WALL_SIDE_UPPER_DIAG,
    WALL_SIDE_LOWER_DIAG,
} wall_side;


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long int u64;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef signed long long int s64;

int draw_editor_buffer = 0;
int editor_mode_enabled = 0;
int editor_selected_map_idx = -1;
wall_side editor_selected_side;

typedef struct {
    u32 alpha:8;
    u32 cell_idx:16;
    u32 side:8;
} edit_wall_id;

edit_wall_id edit_id_buffer[FP_SCREEN_WIDTH*FP_SCREEN_HEIGHT];

void handle_click(int render_x, int render_y) {
    edit_wall_id id = edit_id_buffer[(FP_SCREEN_WIDTH-1-render_x)*FP_SCREEN_HEIGHT+(render_y)];
    editor_selected_map_idx = id.cell_idx>>2;
    editor_selected_side = id.side>>2;
}

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define CLAMP(x,a,b) MIN(MAX(x,a),b)



long long int rseed = 0x853c49e6748fea9bULL;

u32 lcg(long long int a, long long int inc) {
    u64 old_state = rseed;
    rseed = (a * rseed + inc);
    u32 xorshifted = ((old_state >> 18u) ^ old_state) >> 27u;
    u32 rot = old_state >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

u32 urand() {
    return lcg(6364136223846793005ULL, 0xda3e39cb94b95bdbULL);
}

// levels are what, 32x32 tile?

// textures are what... 32x32? :)

#define TEX_SIZE (32)

Color color_lut[5] = {
    {255,255,255,255},
    {255,0,0,255},
    {0,255,0,255},
    {0,255,255,255},
    {255,0,255,255},
};

u8* flat_textures[6];

u8* textures[6];
u8* decals[6];

typedef enum {
    NORMAL_CELL = 0,
    NE_TO_SW_DIAG=1,
    NW_TO_SE_DIAG=2,
} cell_types;

#define NUM_CELL_TYPES 3
#define NUM_TEXTURES 6
#define NUM_DECALS 4

#define MAP_SIZE 32

#define MAX_WALL_HEIGHT 32

typedef enum {
    DARK = 0,
    NEUTRAL = 1,
    BRIGHT = 2,
    FLICKER = 3
} light_levels;
#define NUM_LIGHT_LEVELS 4
float light_level_mults[4] = {1.0f, 0.25f, 1.5f, 1.5f};

typedef struct {
    int start_x, start_y, start_z;
    u8 upper_cell_types[MAP_SIZE*MAP_SIZE];
    u8 lower_cell_types[MAP_SIZE*MAP_SIZE];
    u8 floor[MAP_SIZE*MAP_SIZE];
    u8 ceil[MAP_SIZE*MAP_SIZE]; 
    u8 upper_floor[MAP_SIZE*MAP_SIZE]; 
    u8 upper_ceil[MAP_SIZE*MAP_SIZE];
    u8 untex[MAP_SIZE*MAP_SIZE];
    u8 uetex[MAP_SIZE*MAP_SIZE];
    u8 ustex[MAP_SIZE*MAP_SIZE];
    u8 uwtex[MAP_SIZE*MAP_SIZE];
    u8 lntex[MAP_SIZE*MAP_SIZE];
    u8 letex[MAP_SIZE*MAP_SIZE];
    u8 lstex[MAP_SIZE*MAP_SIZE];
    u8 lwtex[MAP_SIZE*MAP_SIZE];
    u8 ftex[MAP_SIZE*MAP_SIZE];
    u8 uftex[MAP_SIZE*MAP_SIZE];
    u8 ctex[MAP_SIZE*MAP_SIZE];
    u8 uctex[MAP_SIZE*MAP_SIZE];
    u8 udtex[MAP_SIZE*MAP_SIZE];
    u8 ldtex[MAP_SIZE*MAP_SIZE];
    u8 light[MAP_SIZE*MAP_SIZE];
    u8 step_action[MAP_SIZE*MAP_SIZE];
} level;

level levels[1] = {
    {
        .start_x = 2,
        .start_y = 2,
        .start_z = 2,
        //.map_width = 32,
        //.map_height = 32,
        //.floor = level_0_floor, // these two could be merged
        //.upper_floor = level_0_upper_floor,
        //.ceil = level_0_ceil,
        //.upper_ceil = level_0_upper_ceil,
        //.upper_cell_types = level_0_upper_cell_types,
        //.lower_cell_types = level_0_lower_cell_types,
        //.lntex = level_0_lntex, .letex = level_0_letex, .lstex = level_0_lstex, .lwtex = level_0_lwtex,
        //.untex = level_0_untex, .uetex = level_0_uetex, .ustex = level_0_ustex, .uwtex = level_0_uwtex,
        //.ctex = level_0_ctex, .uctex = level_0_uctex, .ftex = level_0_ftex, .uftex = level_0_uftex,
        //.udtex = level_0_udtex, .ldtex = level_0_ldtex,
        //.light = level_0_light,
        //.step_action = level_0_actions,
    }
};

float player_x;
float player_y;
float player_z;
float player_ang;
int pitch = 0;
int cur_level_idx;
int disable_collision = 0;
int collides(float x, float y, level this_level) {
    //return 0;
    if (disable_collision) { return 0; }
    if(editor_mode_enabled) { return 0; }
    int min_tile_x = (int)(x-.25f);
    int max_tile_x = (int)(x+.25f);
    int min_tile_y = (int)(y-.25f);
    int max_tile_y = (int)(y+.25f);
    for(int y = min_tile_y; y <= max_tile_y; y++) {
        for(int x = min_tile_x; x <= max_tile_x; x++) {
            if(this_level.ceil[y*MAP_SIZE + x] < player_z+1) {
                return 1;
            }
            if(this_level.floor[y*MAP_SIZE + x] > player_z+2) {
                return 1;
            }
        }
    }
    return 0;
}

void update_player(float frame_time) {
    float y = sin(player_ang);
    float x = cos(player_ang);
    float move_speed = .08f * frame_time / 16.0f;
    level cur_level = levels[cur_level_idx];
    if (IsKeyDown(KEY_W)) {
        float new_player_x = player_x + move_speed*x;
        float new_player_y = player_y + move_speed*y;
        if(!collides(new_player_x, player_y, cur_level)) {
            player_x = new_player_x;
        }
        if(!collides(player_x, new_player_y,cur_level)) {
            player_y = new_player_y;
        }
    }
    if (IsKeyDown(KEY_S)) {
        float new_player_x = player_x - move_speed*x;
        float new_player_y = player_y - move_speed*y;        
        if(!collides(new_player_x, player_y, cur_level)) {
            player_x = new_player_x;
        }
        if(!collides(player_x, new_player_y, cur_level)) {
            player_y = new_player_y;
        }
    }
    int map_x = (int)player_x;
    int map_y = (int)player_y;
    player_z = levels[cur_level_idx].floor[map_y*MAP_SIZE + map_x]+4.5f;

    if(IsKeyDown(KEY_A)) {
        player_ang += 0.035f;
    }
    if(IsKeyDown(KEY_D)) {
        player_ang -= 0.035f;
    }
}

#define SCALE_FACTOR 32

void draw_topdown_level() {
    level cur_level = levels[cur_level_idx];
    for(int y = 0; y < MAP_SIZE; y++) {
        for(int x = 0; x < MAP_SIZE; x++) {
            //DrawRectangle(x*SCALE_FACTOR, y*SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR, color_lut[cur_level.ttex[y*MAP_SIZE+x]]);
        }
    }
}

#define FOV (65.0f*.0174f)
//(120.0f*.0174f)

int flat_mip_offsets[6] = {
    0, 32*32, 32*32+16*16, 32*32+16*16+8*8, 32*32+16*16+8*8+4*4, 32*32+16*16+8*8+4*4+2*2
};
int flat_mip_scales[6] = {
    //32,16,8,4,2,1
    5,4,3,2,1,0
};
int wall_mip_offsets[6] = {
    0, 32*32, 32*32+16*16, 32*32+16*16+8*8, 32*32+16*16+8*8+4*4, 32*32+16*16+8*8+4*4+2*2
};
int wall_mip_scales[6] = {
    32,16,8,4,2,1
};
#define DARK_DIST 32.0f
#define DARK_DIST_FIXED (32<<16)
#define RECIP_DARK_DIST ((int)(65536.0f/32.0f))

void init_level(int init) {
    player_ang =  1.5707963f;
    player_x = levels[cur_level_idx].start_x;
    player_y = levels[cur_level_idx].start_y;
    if(init) {  
        player_x = 16;
        player_y = 16;
        memset(levels, 0, sizeof(levels));
        for(int y = 0; y < MAP_SIZE; y++) {
            for(int x = 0; x < MAP_SIZE; x++) {
                if(x == 0 || y == 0 || x == MAP_SIZE-1 || y == MAP_SIZE-1) { 
                    levels[cur_level_idx].ceil[y*MAP_SIZE+x] = 5;
                    levels[cur_level_idx].floor[y*MAP_SIZE+x] = 5;
                } else {
                    levels[cur_level_idx].ceil[y*MAP_SIZE+x] = 10;
                    levels[cur_level_idx].floor[y*MAP_SIZE+x] = 0;

                }
            }
        }
        levels[cur_level_idx].start_x = 16;
        levels[cur_level_idx].start_y = 16;
    }
    int map_x = player_x;
    int map_y = player_y;
    player_z = levels[cur_level_idx].floor[map_y*MAP_SIZE + map_x]+4.5f;
}



#define FOCAL_LENGTH (FP_SCREEN_WIDTH / (2.0f * tan(1.57f/2.0f)))
#define HEIGHT_SCALE (4)
typedef struct {
    int start_x, end_x;
} draw_cmd;

#define HALF_SCREEN_HEIGHT (FP_SCREEN_HEIGHT/2)

int project_to_screen(int height, float dist) {
    return pitch + HALF_SCREEN_HEIGHT - (HEIGHT_SCALE * (((height - player_z) * FOCAL_LENGTH / dist) / MAX_WALL_HEIGHT));
}

void draw_tint_vline(u8* output, int x, int y0, int y1, int prev_drawn_top, int prev_drawn_bot) {
    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot-1); y <= CLAMP(y1, prev_drawn_top, prev_drawn_bot-1); y++) {
        output[(x*FP_SCREEN_HEIGHT+y)*4+0] >>= 1;
        output[(x*FP_SCREEN_HEIGHT+y)*4+1] >>= 1;
        output[(x*FP_SCREEN_HEIGHT+y)*4+2] >>= 1;
    }
}


void draw_depth_interp_vline(u8* output, int x, int y0, int y1, float z0, float z1, int prev_drawn_top, int prev_drawn_bot, Color col) {
    float inv_z0 = 1.0f / z0;
    float inv_z1 = 1.0f / z1;
    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot-1); y <= CLAMP(y1, prev_drawn_top, prev_drawn_bot-1); y++) {
        float inv_z = inv_z0 + (inv_z1 - inv_z0) * ((y - y0) / (float)(y1 - y0));
        float cur_z = 1.0f / inv_z;
        float scale = 1.0f - CLAMP(cur_z / DARK_DIST, 0.0f, 1.0f);
        output[(x*FP_SCREEN_HEIGHT+y)*4+0] = col.r * scale;
        output[(x*FP_SCREEN_HEIGHT+y)*4+1] = col.g * scale;
        output[(x*FP_SCREEN_HEIGHT+y)*4+2] = col.b * scale;
    }
}


// draws a textured
void draw_lit_fogged_tex_flat(u8* output, u8* texture, u8* decal, int x, int y0, int y1, float z0, float z1, float start_u, float start_v, float end_u, float end_v, int prev_drawn_top, int prev_drawn_bot, float light_factor) {
    float inv_z0 = 1.0f / z0;
    float inv_z1 = 1.0f / z1;
    float one_over_z = 1.0f/z0;
    float d_one_over_z = ((1.0f/z1) - one_over_z) / (y1-y0);
    float u_over_z = start_u * inv_z0;
    float v_over_z = start_v * inv_z0;
    float d_u_over_z = ((end_u * inv_z1) - u_over_z) / (y1-y0);
    float d_v_over_z = ((end_v * inv_z1) - v_over_z) / (y1-y0);
    int clipped_y0 = CLAMP(y0, prev_drawn_top, prev_drawn_bot);
    int clipped_y1 = CLAMP(y1, prev_drawn_top, prev_drawn_bot);

    if(y0 > prev_drawn_bot) {
        return;
    }
    if(y1 < prev_drawn_top) {
        return;
    }

    for(int y = clipped_y0; y < clipped_y1; y++) {
        float cur_z = 1.0f / (inv_z0 + d_one_over_z*(y-y0));
        float cur_u = CLAMP((u_over_z+d_u_over_z*(y-y0)) * cur_z * 32.0f, 0.0f, 31.0f);
        float cur_v = CLAMP((v_over_z+d_v_over_z*(y-y0)) * cur_z * 32.0f, 0.0f, 31.0f);

        float mult = (1.0f-CLAMP(cur_z/DARK_DIST, 0.0f, 1.0f)) * light_factor;
        //depth_scale *= depth_scale;
        int u = (int)floorf(cur_u);
        int v = (int)floorf(cur_v);

        int idx = (v<<5)+u;

        u32 decal_texel = *(u32*)(&decal[idx*4]);
        u32 texel = *(u32*)(&texture[idx*4]);
        u8 decal_alpha = decal_texel>>24;
        float decal_a = decal_alpha/255.0f;
        u32 texel_r = ((texel >> 16) & 0xFF);
        u32 texel_g = ((texel >> 8) & 0xFF);
        u32 texel_b = ((texel >> 0) & 0xFF);
        u32 decal_r = ((decal_texel >> 16) & 0xFF);
        u32 decal_g = ((decal_texel >> 8) & 0xFF);
        u32 decal_b = ((decal_texel >> 0) & 0xFF);
        float r = ((decal_r * decal_a) + ((1.0f - decal_a) * texel_r));
        float g = ((decal_g * decal_a) + ((1.0f - decal_a) * texel_g));
        float b = ((decal_b * decal_a) + ((1.0f - decal_a) * texel_b));
        u32 intr = CLAMP((int)(r*mult), 0, 0xFF);
        u32 intg = CLAMP((int)(g*mult), 0, 0xFF);
        u32 intb = CLAMP((int)(b*mult), 0, 0xFF);
        *(u32*)(&output[(x*FP_SCREEN_HEIGHT+y)*4]) = 0xFF000000|(intr<<16)|(intg<<8)|intb;
    }
}


void draw_edit_vline(int x, float y0, float y1, int prev_drawn_top, int prev_drawn_bot, int cell_idx, wall_side side) {
    edit_wall_id id = {.alpha = 0xFF, .cell_idx = cell_idx<<2, .side = side<<2};
    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot); y < CLAMP(y1, prev_drawn_top, prev_drawn_bot); y++) {
        edit_id_buffer[x*FP_SCREEN_HEIGHT+y] = id;
    }
}

typedef enum {
    TOP_PEGGED,
    BOTTOM_PEGGED,
} pegging_type;

void draw_lit_fogged_clipped_textured_wall(
    u8* output, u8 *tex_column, u8* decal_tex_column,
    int x,
    float y0, float y1, 
    float world_z0, float world_z1, pegging_type peg_type,
    int prev_drawn_top, int prev_drawn_bot,
    float z, float light_factor) {
    if(y0 > prev_drawn_bot) {
        return;
    }
    if(y1 < prev_drawn_top) {
        return;
    }
    int units = fabsf(world_z1 - world_z0);
    float depth_scale = 1.0f - CLAMP(z / DARK_DIST, 0.0f, 1.0f);
    float mult = depth_scale * light_factor;
    // ok what about 7 light levels, 0 through 6

    float start_v = 0.0f;
    float tex_per_pix = units * 4.0f / (y1-y0);
    float decal_tex_per_pix = units * 4.0f / (y1-y0);
    if(peg_type == BOTTOM_PEGGED) {
        float end_v = units * 4.0f;
        float full_wraps = end_v / 32.0f;
        start_v = (32.0f * (full_wraps - floorf(full_wraps)));
        tex_per_pix = -tex_per_pix;
    }

    for(int y = CLAMP(y0, prev_drawn_top, prev_drawn_bot); y < CLAMP(y1, prev_drawn_top, prev_drawn_bot); y++) {
            int dy = y-y0;
            int idx = (int)floorf(start_v + dy*tex_per_pix)&31;
            int decal_idx = CLAMP((int)floorf(dy*decal_tex_per_pix), 0, 31);

            u32 decal_texel = *(u32*)(&decal_tex_column[decal_idx*4]);
            u32 texel = *(u32*)(&tex_column[idx*4]);
            u8 decal_alpha = decal_texel>>24;
            float decal_a = decal_alpha/255.0f;
            u32 texel_r = ((texel >> 16) & 0xFF);
            u32 texel_g = ((texel >> 8) & 0xFF);
            u32 texel_b = ((texel >> 0) & 0xFF);
            u32 decal_r = (decal_texel >> 16)&0xFF;
            u32 decal_g = (decal_texel >> 8)&0xFF;
            u32 decal_b = (decal_texel >> 0)&0xFF;
            float r = ((decal_r * decal_a) + ((1.0f - decal_a) * texel_r));
            float g = ((decal_g * decal_a) + ((1.0f - decal_a) * texel_g));
            float b = ((decal_b * decal_a) + ((1.0f - decal_a) * texel_b));
            u32 intr = CLAMP((int)(r*mult), 0, 0xFF);
            u32 intg = CLAMP((int)(g*mult), 0, 0xFF);
            u32 intb = CLAMP((int)(b*mult), 0, 0xFF);

            //u32 texel = *(u32*)(&tex_column[idx*4]);
            //u32 r = ((texel >> 16) & 0xFF) * mult;
            //u32 g = ((texel >> 8) & 0xFF) * mult;
            //u32 b = ((texel >> 0) & 0xFF) * mult;


            *(u32*)(&output[(x*FP_SCREEN_HEIGHT+y)*4]) = 0xFF000000|(intr<<16)|(intg<<8)|intb;

    }
}


u8* get_texture_column(u8* texture, float wall_u) {
    float u_scaled_to_tex_size = CLAMP(wall_u * 32.0f, 0.0f, 31.0f);
    int int_u = (int)floorf(u_scaled_to_tex_size);
    return &texture[int_u*TEX_SIZE*4];
}

typedef enum {
    VERTICAL_SIDE = 0,
    HORIZONTAL_SIDE = 1
} side;

#define DEGREES_TO_RAD(deg) ((deg)*.0174f)


typedef struct {
    float diag_wall_u;
    float diag_perp_dist;
    float mid_flat_u;
    float mid_flat_v;
} diag_intersect;

const float diag_dy[3] = {
    1.0f, // dummy entry for normal walls
    -1.0f, // NE_TO_SW_DIAG
    1.0f,  // NW_TO_SE_DIAG
};

#define CEIL_LIGHT_FACTOR (0.35f)
#define FLOOR_LIGHT_FACTOR (0.65f)
#define DIAG_LIGHT_FACTOR (0.87)

void draw_first_person_level(u8 *output, int start_x, int end_x, int frame) {
    int flash_frame = (frame&0b1000000) == 0b1000000;

    level this_level = levels[cur_level_idx];
    //u8* cur_level = this_level;
    u8* cur_level_floor = this_level.floor;
    u8* cur_level_ceil = this_level.ceil;
    u8* cur_level_upper_floor = this_level.upper_floor;
    u8* cur_level_upper_ceil = this_level.upper_ceil;
    float ray_start_x = player_x;
    float ray_start_y = player_y;

    float dang = FOV / FP_SCREEN_WIDTH;
    float start_ang = player_ang - (FOV/2.0f);
    float cam_dir_x = cosf(player_ang);
    float cam_dir_y = sinf(player_ang);

    
    
    for(int screen_x = start_x; screen_x < end_x; screen_x++) {

        float cam_x = 2.0f * screen_x / (float)FP_SCREEN_WIDTH - 1.0f; // -1 to 1
        float ray_dir_x = cosf(player_ang) + cam_x * -sinf(player_ang);
        float ray_dir_y = sinf(player_ang) + cam_x * cosf(player_ang);
        float ray_ang = atan2f(ray_dir_y, ray_dir_x);
        if(ray_ang < 0.0f) {
            ray_ang += 6.28f;
        }
        // length of ray from one x/y side to the next x/y side
        float delta_dist_x = fabsf(1.0f / ray_dir_x);
        float delta_dist_y = fabsf(1.0f / ray_dir_y);

        int map_x = floorf(player_x);
        int map_y = floorf(player_y);

        float def_exit_u = (ray_dir_x >= 0) ? 1.0f : 0.0f;
        float def_exit_v = (ray_dir_y >= 0) ? 1.0f : 0.0f;
        float def_start_u = (ray_dir_x >= 0) ? 0.0f : 1.0f;
        float def_start_v = (ray_dir_y >= 0) ? 0.0f : 1.0f;
        int cell_idx = map_y * MAP_SIZE + map_x;
        int prev_floor_height = cur_level_floor[cell_idx];
        int prev_ceil_height = cur_level_ceil[cell_idx];

        // TODO: not always correct if the current cell has a step!!
        u8 prev_ceil_texture = this_level.ctex[cell_idx];
        u8 prev_ceil_decal_texture = this_level.ctex[cell_idx];
        u8 prev_floor_texture = this_level.ftex[cell_idx];
        float prev_cell_light_level = light_level_mults[this_level.light[cell_idx]];
        wall_side prev_ceil_side = WALL_SIDE_BOTTOM;
        wall_side prev_floor_side = WALL_SIDE_TOP;

        float prev_perp_dist = 0.001f;
        float prev_flat_u = player_x - map_x;
        float prev_flat_v = player_y - map_y;
        int proj_prev_floor_height_at_prev_dist = project_to_screen(prev_floor_height, prev_perp_dist);
        int proj_prev_ceil_height_at_prev_dist = project_to_screen(prev_ceil_height, prev_perp_dist);


        int step_x = (ray_dir_x < 0) ? -1 : 1;
        float side_dist_x = (ray_dir_x < 0) ? ((player_x - map_x) * delta_dist_x) : ((map_x + 1.0f - player_x) * delta_dist_x);

        int step_y = (ray_dir_y < 0) ? -1 : 1;
        float side_dist_y = (ray_dir_y < 0) ? ((player_y - map_y) * delta_dist_y) : ((map_y + 1.0 - player_y) * delta_dist_y);


        int prev_drawn_top = 0;
        int prev_drawn_bot = FP_SCREEN_HEIGHT;
        const int MAX_STEPS = 64;

        int prev_map_idx = cell_idx;

        

        for(int i = 0; i < MAX_STEPS && (prev_drawn_top < prev_drawn_bot); i++) {
            float perp_dist;
            float wall_u;
            float flat_u, flat_v;           // the u,v position of where we enter the next cell (which we use on the next iteration)
            float exit_flat_u, exit_flat_v; // the u,v position of where we "exit" the current cell before stepping to the new one
            float hit_x;
            float hit_y;

            int side;
            float light_factor;
            if(side_dist_x < side_dist_y) {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                side = VERTICAL_SIDE;
                light_factor = 1.0f;
                perp_dist = ((map_x - player_x + (1 - step_x) * 0.5f) / ray_dir_x);
            } else {
                side_dist_y += delta_dist_y;
                map_y += step_y;
                side = HORIZONTAL_SIDE;
                light_factor = .75f;
                perp_dist = ((map_y - player_y + (1 - step_y) * 0.5f) / ray_dir_y);
            }
            if(map_x >= MAP_SIZE || map_x < 0 || map_y >= MAP_SIZE || map_y < 0) {
                break;
            }

            int map_idx = map_y * MAP_SIZE + map_x;
            int selected_cur_map_idx = editor_selected_map_idx == map_idx;
            int selected_prev_map_idx = editor_selected_map_idx == prev_map_idx;
            cell_types upper_cell_type = this_level.upper_cell_types[map_idx];
            cell_types lower_cell_type = this_level.lower_cell_types[map_idx];

            perp_dist = MAX(prev_perp_dist, perp_dist);
            //int upper_hits_diag = 0;
            //int lower_hits_diag = 0;

            //float upper_diag_wall_u = 0.0f;
            //float lower_diag_wall_u = 0.0f;
            //float upper_diag_perp_dist = perp_dist;
            //float lower_diag_perp_dist = perp_dist;
            
            //float upper_mid_flat_u = 0.0f;
            //float upper_mid_flat_v = 0.0f;
            //float lower_mid_flat_v = 0.0f;
            //float lower_mid_flat_u = 0.0f;


            perp_dist = MAX(prev_perp_dist, perp_dist);

            int upper_hits_diag = 0;
            float upper_mid_flat_u = 0.0f;
            float upper_mid_flat_v = 0.0f;
            float upper_diag_wall_u = 0.0f;
            float upper_diag_perp_dist = perp_dist;
            int lower_hits_diag = 0;
            float lower_mid_flat_u = 0.0f;
            float lower_mid_flat_v = 0.0f;
            float lower_diag_wall_u = 0.0f;
            float lower_diag_perp_dist = perp_dist;

            if(upper_cell_type == NE_TO_SW_DIAG || upper_cell_type == NW_TO_SE_DIAG) {
                
                upper_hits_diag = 0;
                upper_mid_flat_u = 0.0f;
                upper_mid_flat_v = 0.0f;
                upper_diag_perp_dist = perp_dist;

                float diag_ix = 0.0f;
                float diag_iy = 0.0f;
                float p1x = player_x;
                float p1y = player_y;
                float q1x = player_x + ray_dir_x;
                float q1y = player_y + ray_dir_y;
                float p2x = map_x+0.5f;
                float p2y = map_y+0.5f;
                float q2x = p2x + 1.0f;
                float q2y = p2y + diag_dy[upper_cell_type];
                float a1 = q1y - p1y;
                float b1 = p1x - q1x;
                float c1 = a1 * p1x + b1 * p1y;

                float a2 = q2y - p2y;//-1;
                float b2 = p2x - q2x;//-1;
                float c2 = a2 * p2x + b2 * p2y;

                float determinant = a1 * b2 - a2 * b1;

                diag_ix = (c1 * b2 - c2 * b1) / determinant;
                diag_iy = (a1 * c2 - a2 * c1) / determinant;
                
                float lx = fabsf(diag_ix - map_x);
                upper_diag_wall_u = lx;


                upper_mid_flat_u = diag_ix - floorf(diag_ix);
                upper_mid_flat_v = diag_iy - floorf(diag_iy);


                if(floorf(diag_ix) == map_x && floorf(diag_iy) == map_y) {
                    upper_hits_diag = 1;
                    float dx = diag_ix-player_x;
                    float dy = diag_iy-player_y;
                    upper_diag_perp_dist = dx*cam_dir_x + dy*cam_dir_y;
                }
                
            }
            if(lower_cell_type == NE_TO_SW_DIAG || lower_cell_type == NW_TO_SE_DIAG) {                
                lower_hits_diag = 0;
                lower_mid_flat_u = 0.0f;
                lower_mid_flat_v = 0.0f;
                lower_diag_perp_dist = perp_dist;

                float diag_ix = 0.0f;
                float diag_iy = 0.0f;
                float p1x = player_x;
                float p1y = player_y;
                float q1x = player_x + ray_dir_x;
                float q1y = player_y + ray_dir_y;
                float p2x = map_x+0.5f;
                float p2y = map_y+0.5f;
                float q2x = p2x + 1.0f;
                float q2y = p2y + diag_dy[lower_cell_type];
                float a1 = q1y - p1y;
                float b1 = p1x - q1x;
                float c1 = a1 * p1x + b1 * p1y;

                float a2 = q2y - p2y;//-1;
                float b2 = p2x - q2x;//-1;
                float c2 = a2 * p2x + b2 * p2y;

                float determinant = a1 * b2 - a2 * b1;

                diag_ix = (c1 * b2 - c2 * b1) / determinant;
                diag_iy = (a1 * c2 - a2 * c1) / determinant;
                
                float lx = fabsf(diag_ix - map_x);
                lower_diag_wall_u = lx;


                lower_mid_flat_u = diag_ix - floorf(diag_ix);
                lower_mid_flat_v = diag_iy - floorf(diag_iy);


                if(floorf(diag_ix) == map_x && floorf(diag_iy) == map_y) {
                    lower_hits_diag = 1;
                    float dx = diag_ix-player_x;
                    float dy = diag_iy-player_y;
                    lower_diag_perp_dist = dx*cam_dir_x + dy*cam_dir_y;
                }
            }


            hit_x = player_x + perp_dist * ray_dir_x;
            hit_y = player_y + perp_dist * ray_dir_y;
            

            // lower floor height and higher ceil height are used for diagonals and other special cell types
            int floor_height = cur_level_floor[map_idx];
            int upper_floor_height = cur_level_upper_floor[map_idx];
            int ceil_height = cur_level_ceil[map_idx];
            int upper_ceil_height = cur_level_upper_ceil[map_idx];


            u8 upper_wall_tex, lower_wall_tex;
            wall_side upper_intersect_wall_side, lower_intersect_wall_side;
            if(side == VERTICAL_SIDE) {
                wall_u = hit_y - floorf(hit_y);
                flat_u = def_start_u;
                flat_v = wall_u;
                exit_flat_u = def_exit_u;
                exit_flat_v = wall_u;

                if(ray_dir_x > 0) {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_WEST;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_WEST;
                    wall_u = 1.0f - wall_u;
                    upper_wall_tex = this_level.uwtex[map_idx];
                    lower_wall_tex = this_level.lwtex[map_idx];
                } else {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_EAST;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_EAST;
                    upper_wall_tex = this_level.uetex[map_idx];
                    lower_wall_tex = this_level.letex[map_idx];
                }
            } else {
                wall_u = hit_x - floorf(hit_x);
                flat_u = wall_u;
                flat_v = def_start_v;
                exit_flat_u = wall_u;
                exit_flat_v = def_exit_v;
                if(ray_dir_y < 0) {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_NORTH;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_NORTH;
                    wall_u = 1.0f - wall_u;
                    upper_wall_tex = this_level.untex[map_idx];
                    lower_wall_tex = this_level.lntex[map_idx];
                } else {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_SOUTH;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_SOUTH;
                    upper_wall_tex = this_level.ustex[map_idx];
                    lower_wall_tex = this_level.lstex[map_idx];
                }
            }
            u8 upper_diag_tex = this_level.udtex[map_idx];
            u8 lower_diag_tex = this_level.ldtex[map_idx];

            
            u8 upper_floor_texture = this_level.uftex[map_idx];
            u8 floor_texture = this_level.ftex[map_idx];
            u8 upper_ceil_texture = this_level.uctex[map_idx];
            u8 ceil_texture = this_level.ctex[map_idx];

            float cell_light_level = light_level_mults[this_level.light[map_idx]];

            int enters_right_side = (step_x == -1) && (side == VERTICAL_SIDE);
            int enters_left_side = (step_x == 1) && (side == VERTICAL_SIDE);
            int enters_top_side = (step_y == 1) && (side == HORIZONTAL_SIDE);
            int enters_bot_side = (step_y == -1) && (side == HORIZONTAL_SIDE);


            int first_floor_height = floor_height;
            int second_floor_height = floor_height;
            int first_ceil_height = ceil_height;
            int second_ceil_height = ceil_height;

            u8 first_floor_texture = floor_texture;
            u8 second_floor_texture = floor_texture;
            u8 first_ceil_texture = ceil_texture;
            u8 second_ceil_texture = ceil_texture;
            wall_side first_floor_side;
            wall_side second_floor_side;
            wall_side first_ceil_side;
            wall_side second_ceil_side;
            if((upper_cell_type == NE_TO_SW_DIAG && (enters_top_side || enters_left_side)) ||
                (upper_cell_type == NW_TO_SE_DIAG && (enters_top_side || enters_right_side))) {
                first_ceil_height = upper_ceil_height;
                first_ceil_texture = upper_ceil_texture;
                first_ceil_side = WALL_SIDE_UPPER_BOTTOM;
                second_ceil_height = ceil_height;
                second_ceil_texture = ceil_texture;
                second_ceil_side = WALL_SIDE_BOTTOM;
            } else {
                first_ceil_height = ceil_height;
                first_ceil_texture = ceil_texture;
                first_ceil_side = WALL_SIDE_BOTTOM;
                second_ceil_height = upper_ceil_height;
                second_ceil_texture = upper_ceil_texture;
                second_ceil_side = WALL_SIDE_UPPER_BOTTOM;
            }        

             if((lower_cell_type == NE_TO_SW_DIAG && (enters_top_side || enters_left_side)) ||
                (lower_cell_type == NW_TO_SE_DIAG && (enters_top_side || enters_right_side))) {
                first_floor_height = upper_floor_height;
                first_floor_texture = upper_floor_texture;
                first_floor_side = WALL_SIDE_UPPER_TOP;
                second_floor_height = floor_height;
                second_floor_texture = floor_texture;
                second_floor_side = WALL_SIDE_TOP;
            } else {
                first_floor_height = floor_height;
                first_floor_texture = floor_texture;
                second_floor_height = upper_floor_height;
                second_floor_texture = upper_floor_texture;
                first_floor_side = WALL_SIDE_TOP;
                second_floor_side = WALL_SIDE_UPPER_TOP;
            }



            int proj_prev_floor_height = project_to_screen(prev_floor_height, perp_dist);
            int proj_prev_ceil_height = project_to_screen(prev_ceil_height, perp_dist);
            


            // draw previous step's steps and flats flats 

            if(proj_prev_ceil_height > prev_drawn_top) {
                draw_lit_fogged_tex_flat(
                    output, 
                    textures[prev_ceil_texture&0xF], decals[prev_ceil_texture>>4],
                    screen_x, 
                    proj_prev_ceil_height_at_prev_dist, proj_prev_ceil_height, 
                    prev_perp_dist, perp_dist, 
                    prev_flat_u, prev_flat_v, exit_flat_u, exit_flat_v, 
                    prev_drawn_top, prev_drawn_bot, prev_cell_light_level*CEIL_LIGHT_FACTOR);
                if(editor_mode_enabled) {
                    draw_edit_vline(
                        screen_x,
                        proj_prev_ceil_height_at_prev_dist, proj_prev_ceil_height, 
                        prev_drawn_top, prev_drawn_bot, 
                        prev_map_idx, prev_ceil_side
                    );
                    if(flash_frame && selected_prev_map_idx && editor_selected_side == prev_ceil_side) {
                        draw_tint_vline(
                            output, screen_x, proj_prev_ceil_height_at_prev_dist, proj_prev_ceil_height,
                            prev_drawn_top, prev_drawn_bot
                        );
                    }
                }
                prev_drawn_top = proj_prev_ceil_height;   
            }
            
            if(proj_prev_floor_height < prev_drawn_bot) {
                draw_lit_fogged_tex_flat(
                    output, 
                    textures[prev_floor_texture&0xF],  decals[prev_floor_texture>>4],
                    screen_x, 
                    proj_prev_floor_height, proj_prev_floor_height_at_prev_dist, 
                    perp_dist, prev_perp_dist, 
                    exit_flat_u, exit_flat_v, prev_flat_u, prev_flat_v, 
                    prev_drawn_top, prev_drawn_bot, prev_cell_light_level*FLOOR_LIGHT_FACTOR);
                if(editor_mode_enabled) {
                    draw_edit_vline(
                        screen_x,
                        proj_prev_floor_height, proj_prev_floor_height_at_prev_dist,
                        prev_drawn_top, prev_drawn_bot, 
                        prev_map_idx, prev_floor_side
                    );
                    if(flash_frame && selected_prev_map_idx && editor_selected_side == prev_floor_side) {
                        draw_tint_vline(
                            output, screen_x, 
                            proj_prev_floor_height, proj_prev_floor_height_at_prev_dist,
                            prev_drawn_top, prev_drawn_bot
                        );
                    }
                }
                prev_drawn_bot = proj_prev_floor_height;
            }


            
            // draw upper step

            int proj_first_floor_height_at_boundary = project_to_screen(first_floor_height, perp_dist);
            int proj_second_floor_height_at_boundary = project_to_screen(second_floor_height,perp_dist);
            int proj_first_floor_height_at_diag = project_to_screen(first_floor_height, lower_diag_perp_dist);
            int proj_second_floor_height_at_diag = project_to_screen(second_floor_height,lower_diag_perp_dist);


            int proj_first_ceil_height_at_boundary = project_to_screen(first_ceil_height, perp_dist);
            int proj_second_ceil_height_at_boundary = project_to_screen(second_ceil_height,perp_dist);
            int proj_first_ceil_height_at_diag = project_to_screen(first_ceil_height, upper_diag_perp_dist);
            int proj_second_ceil_height_at_diag = project_to_screen(second_ceil_height,upper_diag_perp_dist);

            // draw ceil first step
            
            if(proj_first_ceil_height_at_boundary > prev_drawn_top) {
                draw_lit_fogged_clipped_textured_wall(output, 
                    get_texture_column(textures[upper_wall_tex&0xF], wall_u),
                    get_texture_column(decals[upper_wall_tex>>4], wall_u),
                    screen_x, 
                    proj_prev_ceil_height, proj_first_ceil_height_at_boundary, 
                    prev_ceil_height, first_ceil_height, TOP_PEGGED,
                    prev_drawn_top, prev_drawn_bot, perp_dist, cell_light_level*light_factor);
                if(editor_mode_enabled) {
                    draw_edit_vline(
                        screen_x,
                        proj_prev_ceil_height, proj_first_ceil_height_at_boundary,
                        prev_drawn_top, prev_drawn_bot, 
                        map_idx, upper_intersect_wall_side
                    );
                    if(flash_frame && selected_cur_map_idx && editor_selected_side == upper_intersect_wall_side) {
                        draw_tint_vline(
                            output, screen_x, 
                            proj_prev_ceil_height, proj_first_ceil_height_at_boundary,
                            prev_drawn_top, prev_drawn_bot
                        );
                    }
                }
                prev_drawn_top = proj_first_ceil_height_at_boundary;
            }
            
            // draw floor first step
            if(proj_first_floor_height_at_boundary < prev_drawn_bot) {
                draw_lit_fogged_clipped_textured_wall(output, 
                    get_texture_column(textures[lower_wall_tex&0xF], wall_u),
                    get_texture_column(decals[lower_wall_tex>>4], wall_u),
                    screen_x, 
                    proj_first_floor_height_at_boundary, proj_prev_floor_height, 
                    prev_floor_height, first_floor_height, BOTTOM_PEGGED,
                    prev_drawn_top, prev_drawn_bot, perp_dist, cell_light_level*light_factor);
                if(editor_mode_enabled) {
                    draw_edit_vline(
                        screen_x,
                        proj_first_floor_height_at_boundary, proj_prev_floor_height, 
                        prev_drawn_top, prev_drawn_bot,
                        map_idx, lower_intersect_wall_side
                    );
                    if(flash_frame && selected_cur_map_idx && editor_selected_side == lower_intersect_wall_side) {
                        draw_tint_vline(
                            output, screen_x, 
                        proj_first_floor_height_at_boundary, proj_prev_floor_height, 
                            prev_drawn_top, prev_drawn_bot
                        );
                    }
                }
                prev_drawn_bot = proj_first_floor_height_at_boundary;
            }
            
            if(!upper_hits_diag) {
                ceil_height = first_ceil_height;
                proj_second_ceil_height_at_boundary = proj_first_ceil_height_at_boundary;
                second_ceil_texture = first_ceil_texture;
                prev_ceil_side = first_ceil_side;
            } else {
                ceil_height = second_ceil_height;
                prev_ceil_side = second_ceil_side;

                // draw previous ceiling
                if(proj_first_ceil_height_at_diag > prev_drawn_top) {
                    draw_lit_fogged_tex_flat(
                        output, 
                        textures[first_ceil_texture&0xF],  decals[first_ceil_texture>>4],
                        screen_x, 
                        proj_first_ceil_height_at_boundary, proj_first_ceil_height_at_diag, 
                        perp_dist, upper_diag_perp_dist, 
                        flat_u, flat_v, upper_mid_flat_u, upper_mid_flat_v, 
                        prev_drawn_top, prev_drawn_bot, cell_light_level*CEIL_LIGHT_FACTOR);
                    if(editor_mode_enabled) {
                        draw_edit_vline(
                            screen_x,
                            proj_first_ceil_height_at_boundary, proj_first_ceil_height_at_diag, 
                            prev_drawn_top, prev_drawn_bot,
                            map_idx, first_ceil_side
                        );
                        if(flash_frame && selected_cur_map_idx && editor_selected_side == first_ceil_side) {
                            draw_tint_vline(
                                output, screen_x, 
                                proj_first_ceil_height_at_boundary, proj_first_ceil_height_at_diag, 
                                prev_drawn_top, prev_drawn_bot
                            );
                        }
                    }
                    prev_drawn_top = proj_first_ceil_height_at_diag;
                }

                // draw step from ceiling
                if(proj_second_ceil_height_at_diag > prev_drawn_top) {
                    draw_lit_fogged_clipped_textured_wall(output, 
                        get_texture_column(textures[upper_diag_tex&0xF], upper_diag_wall_u),
                        get_texture_column(decals[upper_diag_tex>>4], upper_diag_wall_u),
                        screen_x, 
                        proj_first_ceil_height_at_diag, proj_second_ceil_height_at_diag, 
                        first_ceil_height, second_ceil_height, TOP_PEGGED,
                        prev_drawn_top, prev_drawn_bot, upper_diag_perp_dist, cell_light_level*DIAG_LIGHT_FACTOR);
                    if(editor_mode_enabled) {
                        draw_edit_vline(
                            screen_x,
                            proj_first_ceil_height_at_diag, proj_second_ceil_height_at_diag, 
                            prev_drawn_top, prev_drawn_bot,
                            map_idx, WALL_SIDE_UPPER_DIAG
                        );
                        if(flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_UPPER_DIAG) {
                            draw_tint_vline(
                                output, screen_x, 
                                proj_first_ceil_height_at_diag, proj_second_ceil_height_at_diag, 
                                prev_drawn_top, prev_drawn_bot
                            );
                        }
                    }
                    prev_drawn_top = proj_second_ceil_height_at_diag;
                }


            }

            if(!lower_hits_diag) {
                floor_height = first_floor_height;
                proj_second_floor_height_at_boundary = proj_first_floor_height_at_boundary;
                second_floor_texture = first_floor_texture;

                prev_floor_side = first_floor_side;
            } else {
                floor_height = second_floor_height;

                prev_floor_side = second_floor_side;

                // draw flats from first height at boundary to first height at diag
                
                if(proj_first_floor_height_at_diag < prev_drawn_bot) {
                    draw_lit_fogged_tex_flat(
                        output, 
                        textures[first_floor_texture&0xF],  decals[first_floor_texture>>4],
                        screen_x, 
                        proj_first_floor_height_at_diag, proj_first_floor_height_at_boundary, 
                        lower_diag_perp_dist, perp_dist, 
                        lower_mid_flat_u, lower_mid_flat_v, flat_u, flat_v, 
                        prev_drawn_top, prev_drawn_bot, cell_light_level*FLOOR_LIGHT_FACTOR);
                    if(editor_mode_enabled) {
                        draw_edit_vline(
                            screen_x,
                            proj_first_floor_height_at_diag, proj_first_floor_height_at_boundary, 
                            prev_drawn_top, prev_drawn_bot,
                            map_idx, first_floor_side
                        );
                        if(flash_frame && selected_cur_map_idx && editor_selected_side == first_floor_side) {
                            draw_tint_vline(
                                output, screen_x, 
                                proj_first_floor_height_at_diag, proj_first_floor_height_at_boundary, 
                                prev_drawn_top, prev_drawn_bot
                            );
                        }
                    }
                    prev_drawn_bot = proj_first_floor_height_at_diag;
                }

                // draw walls from first height at diag to second height at diag

                if(proj_second_floor_height_at_diag < prev_drawn_bot) {
                    draw_lit_fogged_clipped_textured_wall(output, 
                        get_texture_column(textures[lower_diag_tex&0xF], lower_diag_wall_u),
                        get_texture_column(decals[lower_diag_tex>>4], lower_diag_wall_u),
                        screen_x, 
                        proj_second_floor_height_at_diag, proj_first_floor_height_at_diag, 
                        first_floor_height, second_floor_height, BOTTOM_PEGGED,
                        prev_drawn_top, prev_drawn_bot, lower_diag_perp_dist, cell_light_level*DIAG_LIGHT_FACTOR);
                    if(editor_mode_enabled) {
                        draw_edit_vline(
                            screen_x,
                            proj_second_floor_height_at_diag, proj_first_floor_height_at_diag, 
                            prev_drawn_top, prev_drawn_bot,
                            map_idx, WALL_SIDE_LOWER_DIAG
                        );
                        if(flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_LOWER_DIAG) {
                            draw_tint_vline(
                                output, screen_x, 
                                proj_second_floor_height_at_diag, proj_first_floor_height_at_diag, 
                                prev_drawn_top, prev_drawn_bot
                            );
                        }
                    }
                    prev_drawn_bot = proj_second_floor_height_at_diag;
                }
            }

        next_iter:
            prev_floor_height = floor_height;
            prev_ceil_height = ceil_height;
            proj_prev_floor_height_at_prev_dist = proj_second_floor_height_at_boundary;
            proj_prev_ceil_height_at_prev_dist = proj_second_ceil_height_at_boundary;

            prev_perp_dist = perp_dist;
            prev_flat_u = flat_u;
            prev_flat_v = flat_v;
            prev_ceil_texture = second_ceil_texture;
            prev_floor_texture = second_floor_texture;

            prev_cell_light_level = cell_light_level;

            prev_map_idx = map_idx;
        }
    }

    return; 
}

void draw_player() {
    float y = 15*sin(player_ang);
    float x = 15*cos(player_ang);
    DrawCircle(player_x*SCALE_FACTOR, player_y*SCALE_FACTOR, 5, RED);
    DrawLine(player_x*SCALE_FACTOR, player_y*SCALE_FACTOR, 
        player_x*SCALE_FACTOR+x, player_y*SCALE_FACTOR+y, BLUE);
}

void load_resources() {

    Image tex0 = LoadImage(".\\resources\\wall_tex0.png");
    Image tex1 = LoadImage(".\\resources\\wall_tex1.png");
    Image tex2 = LoadImage(".\\resources\\flat_tex0.png");
    Image tex3 = LoadImage(".\\resources\\flat_tex1.png");
    Image tex4 = LoadImage(".\\resources\\bookshelf.png");
    Image window_tex = LoadImage(".\\resources\\glass_window.png");
    Image moss_tex = LoadImage(".\\resources\\moss.png");
    Image chandelier_tex = LoadImage(".\\resources\\chandelier.png");

    size_t mip_tex_size = sizeof(u8)*4*TEX_SIZE*TEX_SIZE + sizeof(u8)*4*16*16 + sizeof(u8)*4*8*8 + sizeof(u8)*4*4*4 + sizeof(u8)*4*2*2 + sizeof(u8)*4*1*1;
    u8* tex0_data = malloc(mip_tex_size);
    u8* tex1_data = malloc(mip_tex_size);
    u8* tex2_data = malloc(mip_tex_size);
    u8* tex3_data = malloc(mip_tex_size);
    u8* tex4_data = malloc(mip_tex_size);
    u8* tex5_data = malloc(mip_tex_size);
    u8* window_tex_data = malloc(mip_tex_size);
    u8* moss_tex_data = malloc(mip_tex_size);
    u8* chandelier_tex_data = malloc(mip_tex_size);
    
    u8* copy_ptrs[][2] = {
        tex0_data, tex0.data,
        tex1_data, tex1.data,
        tex2_data, tex2.data,
        tex3_data, tex3.data,
        tex4_data, tex4.data,
        window_tex_data, window_tex.data,
        moss_tex_data, moss_tex.data,
        chandelier_tex_data, chandelier_tex.data

    };
    for(int mip = 0; mip < 1; mip++) {
        int dim = TEX_SIZE>>mip;
        for(int y = 0; y < dim; y++) {
            for(int x = 0; x < dim; x++) {

                int off = flat_mip_offsets[mip];

                for(int i = 0; i < 8; i++) {
                    u8* src = copy_ptrs[i][1];
                    u8* dst = copy_ptrs[i][0];
                    dst[(off+y*dim+x)*4+0] = ((src))[(off+y*dim+x)*4+0];
                    dst[(off+y*dim+x)*4+1] = ((src))[(off+y*dim+x)*4+1];
                    dst[(off+y*dim+x)*4+2] = ((src))[(off+y*dim+x)*4+2];
                    dst[(off+y*dim+x)*4+3] = ((src))[(off+y*dim+x)*4+3];

                }
            }
        }
    }
    
    textures[0] = tex2_data;
    textures[1] = tex3_data;
    textures[2] = tex0_data;
    textures[3] = tex1_data;
    textures[4] = tex4_data;
    textures[5] = tex1_data;
    decals[0] = calloc(mip_tex_size, 1);
    decals[1] = window_tex_data;
    decals[2] = moss_tex_data;
    decals[3] = chandelier_tex_data;
}


#define MAP_SAVE_FILE "./map_save"
int main(void) {
  
    const int screenWidth = OUTPUT_WIDTH;
    const int screenHeight = OUTPUT_HEIGHT;

    InitWindow(screenWidth, screenHeight, "raycast");

    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(60);

    load_resources();
    int num_loaded_bytes;
    u8* loaded_bytes = LoadFileData(MAP_SAVE_FILE, &num_loaded_bytes);
    if(num_loaded_bytes == sizeof(levels)) {
        memcpy(levels, loaded_bytes, sizeof(levels));
        printf("Loaded map data\n");
        init_level(0);
    } else {
        printf("Initializing new map data\n");
        init_level(1);
    }

    int frame = 0;
    //Image draw_img = GenImageColor(FP_SCREEN_HEIGHT, FP_SCREEN_WIDTH, BLACK);
    u8* draw_pix = malloc(sizeof(u8)*4*FP_SCREEN_HEIGHT*FP_SCREEN_WIDTH);

    Image draw_img = {
        .data = draw_pix,
        .width = FP_SCREEN_HEIGHT,
        .height = FP_SCREEN_WIDTH,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };

    Texture2D draw_tex = LoadTextureFromImage(draw_img);


    draw_cmd draw_tasks[4] = {
        {0, FP_SCREEN_WIDTH},
        {(FP_SCREEN_WIDTH/4), (2*FP_SCREEN_WIDTH/4)},
        {(2*FP_SCREEN_WIDTH/4), (3*FP_SCREEN_WIDTH/4)},
        {(3*FP_SCREEN_WIDTH/4), FP_SCREEN_WIDTH},
    };

    float rotation = 0.0f;

    int draw_x = 0;
    int draw_y = 0;


    int cnt_limit = 15;
    int cntr = 0;
    const u8 incs[16] = {
        +1,+1,+1,+1,+1,+1,+1,+1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    int inc_idx = 0;
    SetTextureFilter(draw_tex, TEXTURE_FILTER_POINT);
    while(!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        ClearBackground(BLACK);
        ImageClearBackground(&draw_img, BLACK);

        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse_pos = GetMousePosition();
            handle_click(FP_SCREEN_WIDTH*mouse_pos.x/OUTPUT_WIDTH, FP_SCREEN_HEIGHT*mouse_pos.y/OUTPUT_HEIGHT);
        }
        if (IsKeyPressed(KEY_E)) {
            editor_mode_enabled = !editor_mode_enabled;
        }
        if (IsKeyPressed(KEY_Z)) {
            draw_editor_buffer = !draw_editor_buffer;
        }

        if (IsKeyDown(KEY_I)) {
            pitch += 6;
        } else if (IsKeyDown(KEY_K)) {
            pitch -= 6;
        } else if (IsKeyPressed(KEY_SPACE)) {
            pitch = 0;
        }
        pitch = CLAMP(pitch, -(FP_SCREEN_HEIGHT/4), FP_SCREEN_HEIGHT/4);

        if(editor_mode_enabled) {
            int dy = 0;
            if(IsKeyPressed(KEY_DOWN)) {
                dy = -1;
            } else if (IsKeyPressed(KEY_UP)) {
                dy = 1;
            }
            if(dy != 0) {
                u8* height_ptr = NULL;
                switch(editor_selected_side) {
                    case WALL_SIDE_BOTTOM:
                    case WALL_SIDE_UPPER_NORTH:
                    case WALL_SIDE_UPPER_EAST:
                    case WALL_SIDE_UPPER_SOUTH:
                    case WALL_SIDE_UPPER_WEST:
                        height_ptr = &levels[cur_level_idx].ceil[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_UPPER_DIAG:
                    case WALL_SIDE_UPPER_BOTTOM:
                        height_ptr = &levels[cur_level_idx].upper_ceil[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_TOP:   
                    case WALL_SIDE_LOWER_NORTH:
                    case WALL_SIDE_LOWER_EAST:
                    case WALL_SIDE_LOWER_SOUTH:
                    case WALL_SIDE_LOWER_WEST:
                        height_ptr = &levels[cur_level_idx].floor[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_UPPER_TOP:  
                    case WALL_SIDE_LOWER_DIAG:
                        height_ptr = &levels[cur_level_idx].upper_floor[editor_selected_map_idx];
                        break;
                }
                if(height_ptr != NULL) {
                    int nval = *height_ptr+dy;
                    nval = CLAMP(nval, 0, MAX_WALL_HEIGHT);
                    *height_ptr = nval;
                }
            } else if (IsKeyPressed(KEY_T)) {
                u8* type_ptr = NULL;
                switch(editor_selected_side) {
                    case WALL_SIDE_BOTTOM:
                    case WALL_SIDE_UPPER_NORTH:
                    case WALL_SIDE_UPPER_EAST:
                    case WALL_SIDE_UPPER_SOUTH:
                    case WALL_SIDE_UPPER_WEST:
                    case WALL_SIDE_UPPER_DIAG:
                    case WALL_SIDE_UPPER_BOTTOM:
                        type_ptr = &levels[cur_level_idx].upper_cell_types[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_TOP:   
                    case WALL_SIDE_LOWER_NORTH:
                    case WALL_SIDE_LOWER_EAST:
                    case WALL_SIDE_LOWER_SOUTH:
                    case WALL_SIDE_LOWER_WEST:
                    case WALL_SIDE_UPPER_TOP:  
                    case WALL_SIDE_LOWER_DIAG:
                        type_ptr = &levels[cur_level_idx].lower_cell_types[editor_selected_map_idx];
                        break;
                }
                if(type_ptr != NULL) {
                    u8 nval = *type_ptr + 1;
                    if(nval >= NUM_CELL_TYPES) {
                        nval = 0;
                    }
                    *type_ptr = nval;
                }
            } else if (IsKeyPressed(KEY_L)) {
                levels[cur_level_idx].light[editor_selected_map_idx] += 1;
                if(levels[cur_level_idx].light[editor_selected_map_idx] >= NUM_LIGHT_LEVELS) {
                    levels[cur_level_idx].light[editor_selected_map_idx] = 0;
                }
            } else if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_F)) {
                u8* tex_ptr = NULL;
                switch(editor_selected_side) {
                    case WALL_SIDE_BOTTOM:
                        tex_ptr = &levels[cur_level_idx].ctex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_UPPER_BOTTOM:
                        tex_ptr = &levels[cur_level_idx].uctex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_TOP:
                        tex_ptr = &levels[cur_level_idx].ftex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_UPPER_TOP:
                        tex_ptr = &levels[cur_level_idx].uftex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_LOWER_NORTH:
                        tex_ptr = &levels[cur_level_idx].lntex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_UPPER_NORTH:
                        tex_ptr = &levels[cur_level_idx].untex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_LOWER_EAST:
                        tex_ptr = &levels[cur_level_idx].letex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_UPPER_EAST:
                        tex_ptr = &levels[cur_level_idx].uetex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_LOWER_SOUTH:
                        tex_ptr = &levels[cur_level_idx].lstex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_UPPER_SOUTH:
                        tex_ptr = &levels[cur_level_idx].ustex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_LOWER_WEST:
                        tex_ptr = &levels[cur_level_idx].lwtex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_UPPER_WEST:
                        tex_ptr = &levels[cur_level_idx].uwtex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_UPPER_DIAG:
                        tex_ptr = &levels[cur_level_idx].udtex[editor_selected_map_idx];
                        break;
                    case WALL_SIDE_LOWER_DIAG:
                        tex_ptr = &levels[cur_level_idx].ldtex[editor_selected_map_idx];
                        break;
                }
                if(tex_ptr != NULL) {
                    if(IsKeyPressed(KEY_R)) {
                        u8 ntex_idx = ((*tex_ptr)&0xF)+1;
                        if(ntex_idx >= NUM_TEXTURES) {
                            ntex_idx = 0;
                        }
                        *tex_ptr &= 0xF0;
                        *tex_ptr |= ntex_idx;
                    } else if (IsKeyPressed(KEY_F)) {
                        u8 ndec_idx = ((*tex_ptr)>>4) + 1;
                        if(ndec_idx >= NUM_DECALS) {
                            ndec_idx = 0;
                        }
                        *tex_ptr &= 0x0F;
                        *tex_ptr |= (ndec_idx << 4);
                    }
                }
            }
        }

        BeginDrawing(); {        

            draw_first_person_level(draw_img.data, 0, FP_SCREEN_WIDTH, frame);

            if(cntr++ == cnt_limit) {
                for(int y = 5; y < 7; y++) {
                    for(int x = 5; x < 7; x++) {
                        levels[cur_level_idx].floor[y*MAP_SIZE+x] += incs[inc_idx];
                    }
                }
                inc_idx += 1;
                if(inc_idx > 15) {
                    inc_idx = 0;
                }
                cntr = 0;
            }
            if(draw_editor_buffer) {
                UpdateTexture(draw_tex, (u32*)edit_id_buffer);
            } else {
                UpdateTexture(draw_tex, draw_img.data);
            }
            float scale = ((float)OUTPUT_WIDTH/((float)FP_SCREEN_WIDTH));
            DrawTextureEx(draw_tex, (Vector2){.x=OUTPUT_WIDTH,.y=0}, 90.0f, scale, WHITE);

            //draw_player();
            //draw_objects();
            float frame_time_ms = GetFrameTime()*1000.0f;
            update_player(frame_time_ms);
            //printf("%.2f\n", 1000.0f/frame_time_ms);
            //printf("px py %f %f\n", player_x, player_y);
        } EndDrawing();
        frame++;
    }


    levels[cur_level_idx].start_x = player_x;
    levels[cur_level_idx].start_y = player_y;
    if(!SaveFileData(MAP_SAVE_FILE, levels, sizeof(levels))) {
        printf("Error saving file :(\n");
    }
}