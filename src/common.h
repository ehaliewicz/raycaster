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

#define NUM_CELL_TYPES 3
#define NUM_TEXTURES 6
#define SKYBOX_TEX_IDX 15
#define NUM_DECALS 4

#define MAP_SIZE 32

#define MAX_WALL_HEIGHT 32
#define FOV (90.0f*.0174f)

#define OUTPUT_WIDTH (1024)
#define OUTPUT_HEIGHT (768)
#define FP_SCREEN_WIDTH (OUTPUT_WIDTH/2)
#define FP_SCREEN_HEIGHT (OUTPUT_HEIGHT/2)


#define TEX_SIZE (32)
#define SKYBOX_TEX_HEIGHT (256)
#define SKYBOX_TEX_WIDTH (1024)
#define NEAR_PLANE_DIST (0.01f)

#define NUM_LIGHT_LEVELS 4

#define DARK_DIST 64.0f 
//32.0f
#define DARK_DIST_FIXED (32<<16)
#define RECIP_DARK_DIST ((int)(65536.0f/32.0f))

#define SKYBOX_U_PER_PIX (((float)SKYBOX_TEX_WIDTH)/FP_SCREEN_WIDTH)
#define SKYBOX_V_PER_PIX (((float)SKYBOX_TEX_HEIGHT/2)/FP_SCREEN_HEIGHT)


#ifdef DEBUG
    #define NUM_THREADS 1
#else
    #define NUM_THREADS 4
#endif 
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

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define CLAMP(x,a,b) MIN(MAX(x,a),b)

typedef enum {
    NORMAL_CELL = 0,
    NE_TO_SW_DIAG=1,
    NW_TO_SE_DIAG=2,
} cell_types;


typedef enum {
    DARK = 0,
    NEUTRAL = 1,
    BRIGHT = 2,
    FLICKER = 3
} light_levels;



extern u8* textures[16];
extern u8* decals[NUM_DECALS];


typedef struct {
    u32 alpha:8;
    u32 cell_idx:16;
    u32 side:8;
} edit_wall_id;

extern float player_ang;
extern int pitch;


#endif