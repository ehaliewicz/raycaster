#ifndef COMMON_H
#define COMMON_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long int u64;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef signed long long int s64;

#define PLAYER_HEIGHT 5.5f

typedef enum {
    VERTICAL_SIDE = 0,
    HORIZONTAL_SIDE = 1
} wall_side;

typedef enum {
    NORMAL_CELL = 0,
    NE_TO_SW_DIAG=1,
    NW_TO_SE_DIAG=2,
    SLOPE_Y=3,
    SLOPE_X=4,
    DOOR_Y=5,
    DOOR_X=6,
    THIN_WALL_X=7, // split vertically in half
    THIN_WALL_Y=8, // split horizontally in half
    //HEIGHTMAP=6
} cell_types;


#define NUM_CELL_TYPES 9
#define NUM_TEXTURES 9
#define SKYBOX_TEX_IDX 15
#define NUM_SPRITES 21
//#define NUM_DECALS 4
//#define BLANK_DECAL_IDX 0

#define EMPTY_SPRITE_INDEX 64

#define MAP_SIZE 32

#define MAX_WALL_HEIGHT 64
#define FOV (cur_fov*0.0174533f)
#define VFOV (45.0f*0.0174533f)

#define NUM_RESOLUTIONS 7

extern float cur_fov;
extern int cur_output_width;
extern int cur_output_height;
extern int cur_render_width;
extern int cur_render_height;
extern int cur_render_scale;

#define OUTPUT_WIDTH (cur_output_width)
#define OUTPUT_HEIGHT (cur_output_height)
#define FP_SCREEN_WIDTH (cur_render_width)
#define FP_SCREEN_HEIGHT (cur_render_height)


#define TEX_SIZE (32)
#define SKYBOX_TEX_HEIGHT (256)
#define SKYBOX_TEX_WIDTH (1024)
#define NEAR_PLANE_DIST (0.001f)


#define NUM_LIGHT_LEVELS 4

#define DARK_DIST 80.0f
#define DARK_DIST_FIXED (32<<16)
#define RECIP_DARK_DIST ((int)(65536.0f/32.0f))

#define SKYBOX_V_PER_PIX (((float)SKYBOX_TEX_HEIGHT/2)/FP_SCREEN_HEIGHT)


#ifdef DEBUG
    #define NUM_THREADS 1
#elif PLATFORM_WEB
    #define NUM_THREADS 1
#else
    #define NUM_THREADS 12
#endif 



#define PLAYER_RADIUS (0.25f)

#define DOOR_FULLY_OPEN  200

#define NUM_LEVELS 1

typedef struct {
    int start_x, start_y, start_z; // map position on load
    u8 upper_cell_types[MAP_SIZE*MAP_SIZE]; // cell types: BLOCK, NE_TO_SW_DIAG, NW_TO_SE_DIAG, X_SLOPE, Y_SLOPE, DOOR.  half size walls?
    u8 lower_cell_types[MAP_SIZE*MAP_SIZE];

    // base floor/ceil height, for cells with two heights, this is the height of the y+ portion of the cell
    u8 floor[MAP_SIZE*MAP_SIZE], ceil[MAP_SIZE*MAP_SIZE];
    // secondary floor/ceil height, for cells with two heights, the y-1 portion of the cell
    u8 upper_floor[MAP_SIZE*MAP_SIZE], upper_ceil[MAP_SIZE*MAP_SIZE];

    // upper north, east, south, and west face textures (the vertical walls of the extruded ceiling)
    u8 untex[MAP_SIZE*MAP_SIZE]; // 3 bits!
    u8 uetex[MAP_SIZE*MAP_SIZE]; // 3 bits!
    u8 ustex[MAP_SIZE*MAP_SIZE]; // 3 bits
    u8 uwtex[MAP_SIZE*MAP_SIZE]; 

    // lower north, east, south, and west face textures (vertical walls of the extruded floor)
    u8 lntex[MAP_SIZE*MAP_SIZE];
    u8 letex[MAP_SIZE*MAP_SIZE];
    u8 lstex[MAP_SIZE*MAP_SIZE];
    u8 lwtex[MAP_SIZE*MAP_SIZE];

    // floor/ceiling texuters
    // floor, upper_floor texture
    // 'upper'  texture.  used for diagonals.  The NW side of NW_TO_SE diag, and the NE side of a NE_TO_SW diag
    u8 ftex[MAP_SIZE*MAP_SIZE], uftex[MAP_SIZE*MAP_SIZE];
    // base, upper ceiling texture
    u8 ctex[MAP_SIZE*MAP_SIZE], uctex[MAP_SIZE*MAP_SIZE];

    // upper/lower diagonal face texture, for the visible diagonal wall of a ceiling/floor cell
    u8 udtex[MAP_SIZE*MAP_SIZE], ldtex[MAP_SIZE*MAP_SIZE];

    // cell light level (currently unused, "volumetric lighting"?)
    u8 light[MAP_SIZE*MAP_SIZE];
    // used for timers/etc
    u8 parameter[MAP_SIZE*MAP_SIZE];

    // center sprite (billboard) + fixed orientation sprites
    u8 sprite_index[MAP_SIZE*MAP_SIZE]; 
    u8 n_sprite_index[MAP_SIZE*MAP_SIZE];
    u8 e_sprite_index[MAP_SIZE*MAP_SIZE];
    u8 s_sprite_index[MAP_SIZE*MAP_SIZE];
    u8 w_sprite_index[MAP_SIZE*MAP_SIZE];

    // light levels per face
    u8 ln_light[MAP_SIZE*MAP_SIZE];
    u8 le_light[MAP_SIZE*MAP_SIZE];
    u8 ls_light[MAP_SIZE*MAP_SIZE];
    u8 lw_light[MAP_SIZE*MAP_SIZE];

    u8 un_light[MAP_SIZE*MAP_SIZE];
    u8 ue_light[MAP_SIZE*MAP_SIZE];
    u8 us_light[MAP_SIZE*MAP_SIZE];
    u8 uw_light[MAP_SIZE*MAP_SIZE];

    u8 f_light[MAP_SIZE*MAP_SIZE];
    u8 uf_light[MAP_SIZE*MAP_SIZE];
    u8 c_light[MAP_SIZE*MAP_SIZE];
    u8 uc_light[MAP_SIZE*MAP_SIZE];
    u8 ud_light[MAP_SIZE*MAP_SIZE];
    u8 ld_light[MAP_SIZE*MAP_SIZE];

    float start_ang;

    u8 f_sprite_index[MAP_SIZE*MAP_SIZE];
    u8 c_sprite_index[MAP_SIZE*MAP_SIZE];
    u8 m_sprite_index[MAP_SIZE*MAP_SIZE];
    u8 m_sprite_offset[MAP_SIZE*MAP_SIZE];
    u8 floor_anchor[MAP_SIZE*MAP_SIZE];
    u8 ceil_anchor[MAP_SIZE*MAP_SIZE];
} level;


extern level *levels;
extern int cur_level_idx;


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
    CELL_SPRITE,
    N_SPRITE, E_SPRITE, S_SPRITE, W_SPRITE,
    FLOOR_SPRITE,
    CEIL_SPRITE,
    MIDDLE_SPRITE
} editor_selected_thing;

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define CLAMP(x,a,b) MIN(MAX(x,a),b)



typedef enum {
    DARK = 0,
    NEUTRAL = 1,
    BRIGHT = 2,
    FLICKER = 3
} light_levels;



extern u32** textures;//[16];
extern u32** sprites;//[NUM_SPRITES];

typedef u32 edit_wall_id;

extern float player_x, player_y, player_z;
extern float player_ang;
extern float pitch;
extern float skybox_u_offset;


typedef enum {
    CONTINUE_GAME = 0,
    EXIT_GAME = 1,
    RELOAD_GAME = 2,
    SET_RESOLUTION_VSYNC = 3
} game_ret_code;

typedef struct {
    int resolution[2];
    int use_vsync;
    int wide_fov;
    int requested_render_scale;
} game_ret_payload;

typedef struct {
    game_ret_code code;
    game_ret_payload payload;
} game_ret_value;

typedef struct {
    int x, y;
    int height, top_height;
} sprite_world_position;

extern int num_world_sprites;
extern sprite_world_position world_sprite_positions[];

float get_height_at_point_for_sprites(float px, float py, int return_ceil);
float get_height_at_point(float px, float py, float pz, int return_ceil, int check_middle_sprite);
void* my_malloc(long long unsigned int bytes, char* for_str);
void* my_calloc(long long unsigned int bytes, char* for_str);


typedef struct {
    float x;
    float y;
} Vector2;

#define RAD2DEG (57.29577f)
#define RED (0xFFFF0000)
#define WHITE (0xFFFFFFFF)

#endif