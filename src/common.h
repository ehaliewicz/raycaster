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
    //THIN_WALL_X=5,
    //THIN_WALL_Y=6,
    //HEIGHTMAP=6
} cell_types;

extern u8 heightmap[32*32];

#define NUM_CELL_TYPES 6
#define NUM_TEXTURES 6
#define NUM_DECALS 4
#define BLANK_DECAL_IDX 0
#define NUM_SPRITES 1

#define SKYBOX_TEX_IDX 15

#define MAP_SIZE 32

#define MAX_WALL_HEIGHT 32
#define FOV (cur_fov*.0174f)

#define NUM_RESOLUTIONS 6

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

#define DARK_DIST 64.0f 
//32.0f
#define DARK_DIST_FIXED (32<<16)
#define RECIP_DARK_DIST ((int)(65536.0f/32.0f))

#define SKYBOX_V_PER_PIX (((float)SKYBOX_TEX_HEIGHT/2)/FP_SCREEN_HEIGHT)


#ifdef DEBUG
    #define NUM_THREADS 1
#else
    #define NUM_THREADS 4
#endif 

typedef struct {
    int start_x, start_y, start_z; // map position on load
    u8 upper_cell_types[MAP_SIZE*MAP_SIZE]; // cell types: BLOCK, NE_TO_SW_DIAG, NW_TO_SE_DIAG, X_SLOPE, Y_SLOPE, THIN_WALL_X, THIN_WALL_Y
    u8 lower_cell_types[MAP_SIZE*MAP_SIZE];

    // base floor/ceil height, for cells with two heights, this is the height of the y+ portion of the cell
    u8 floor[MAP_SIZE*MAP_SIZE];
    u8 ceil[MAP_SIZE*MAP_SIZE]; 
    // secondary floor/ceil height, for cells with two heights, the y-1 portion of the cell
    u8 upper_floor[MAP_SIZE*MAP_SIZE]; 
    u8 upper_ceil[MAP_SIZE*MAP_SIZE];

    // upper north, east, south, and west face textures (the vertical walls of the extruded ceiling)
    u8 untex[MAP_SIZE*MAP_SIZE];
    u8 uetex[MAP_SIZE*MAP_SIZE];
    u8 ustex[MAP_SIZE*MAP_SIZE];
    u8 uwtex[MAP_SIZE*MAP_SIZE];

    // lower north, east, south, and west face textures (vertical walls of the extruded floor)
    u8 lntex[MAP_SIZE*MAP_SIZE];
    u8 letex[MAP_SIZE*MAP_SIZE];
    u8 lstex[MAP_SIZE*MAP_SIZE];
    u8 lwtex[MAP_SIZE*MAP_SIZE];

    // base floor texture
    u8 ftex[MAP_SIZE*MAP_SIZE];
    // 'upper' floor texture.  used for diagonals.  The NW side of NW_TO_SE diag, and the NE side of a NE_TO_SW diag
    u8 uftex[MAP_SIZE*MAP_SIZE];
    // base ceiling texture
    u8 ctex[MAP_SIZE*MAP_SIZE];
    // 'upper' ceil texture.  used for diagonals.  The NW side of NW_TO_SE diag, and the NE side of a NE_TO_SW diag
    u8 uctex[MAP_SIZE*MAP_SIZE];

    // upper diagonal face texture, for the visible diagonal wall of a ceiling cell
    u8 udtex[MAP_SIZE*MAP_SIZE];
    // lower diagonal face texture, for the visible diagonal wall of a floor cell
    u8 ldtex[MAP_SIZE*MAP_SIZE];
    // cell light level
    u8 light[MAP_SIZE*MAP_SIZE];
    u8 parameter[MAP_SIZE*MAP_SIZE];
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
} editor_wall_side;

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define CLAMP(x,a,b) MIN(MAX(x,a),b)



typedef enum {
    DARK = 0,
    NEUTRAL = 1,
    BRIGHT = 2,
    FLICKER = 3
} light_levels;



extern u8* textures[16];
extern u8* decals[NUM_DECALS];
extern u8* sprites[NUM_SPRITES];


typedef struct {
    union {
        u16 raw_val;
        struct {
        //u32 alpha:8;
        u16 cell_idx:12; // up to 64x64 maps
        u16 side:4;
        };
    };
} edit_wall_id;

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

#endif