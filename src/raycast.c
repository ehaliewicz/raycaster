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

#include "common.h"


int draw_editor_buffer = 0;
int editor_mode_enabled = 0;
int editor_selected_map_idx = -1;
wall_side editor_selected_side;


edit_wall_id edit_id_buffer[FP_SCREEN_WIDTH*FP_SCREEN_HEIGHT];

void handle_click(int render_x, int render_y) {
    edit_wall_id id = edit_id_buffer[(FP_SCREEN_WIDTH-1-render_x)*FP_SCREEN_HEIGHT+(render_y)];
    editor_selected_map_idx = id.cell_idx>>2;
    editor_selected_side = id.side>>2;
}



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


Color color_lut[5] = {
    {255,255,255,255},
    {255,0,0,255},
    {0,255,0,255},
    {0,255,255,255},
    {255,0,255,255},
};

u8* flat_textures[6];

u8* textures[NUM_TEXTURES];
u8* decals[NUM_DECALS];



level levels[1] = {
    {
        .start_x = 2,
        .start_y = 2,
        .start_z = 2,
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

void update_player(float frame_time, Vector2 mouse_delta) {
    float y = sin(player_ang);
    float x = cos(player_ang);
    float strafe_left_x = -y;
    float strafe_left_y = x;
    float strafe_right_x = y;
    float strafe_right_y = -x;
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
    if(IsKeyDown(KEY_A)) {
        float new_player_x = player_x + move_speed*strafe_left_x;
        float new_player_y = player_y + move_speed*strafe_left_y;
        if(!collides(new_player_x, player_y, cur_level)) {
            player_x = new_player_x;
        }
        if(!collides(player_x, new_player_y,cur_level)) {
            player_y = new_player_y;
        }
    }
    if(IsKeyDown(KEY_D)) {
        float new_player_x = player_x + move_speed*strafe_right_x;
        float new_player_y = player_y + move_speed*strafe_right_y;
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

    if(IsKeyDown(KEY_LEFT)) {
        player_ang += 0.0035f*frame_time;
    }
    if(IsKeyDown(KEY_RIGHT)) {
        player_ang -= 0.0035f*frame_time;
    }
    if(!editor_mode_enabled) {
        if(mouse_delta.y != 0) {
            pitch -= mouse_delta.y;
        }
        if(mouse_delta.x != 0) {
            player_ang -= mouse_delta.x*.0017f;
        }
    }
    if (IsKeyDown(KEY_I)) {
        pitch += 1*frame_time;
    } else if (IsKeyDown(KEY_K)) {
        pitch -= 1*frame_time;
    } else if (IsKeyPressed(KEY_SPACE)) {
        pitch = 0;
    }
    pitch = CLAMP(pitch, -(FP_SCREEN_HEIGHT/2), FP_SCREEN_HEIGHT/2);

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
#include "thread.h"

#define MAP_SAVE_FILE "./map_save"
int main(void) {
  
    const int screenWidth = OUTPUT_WIDTH;
    const int screenHeight = OUTPUT_HEIGHT;

    InitWindow(screenWidth, screenHeight, "raycast");

    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(1000);

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
        Vector2 mouse_delta;
        if(editor_mode_enabled) {
            ShowCursor();
        } else {
            HideCursor();
            mouse_delta = GetMouseDelta();
            SetMousePosition(OUTPUT_WIDTH/2, OUTPUT_HEIGHT/2);
        }
        //printf("Mouse dx dy %f,%f\n", mouse_delta.x, mouse_delta.y);
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

            //thread_pool_add_work()
            //thread_args[0].frame = frame;
            //thread_args[0].finished = 0;
            //thread_args[0].output = 
            draw_first_person_level(draw_img.data, edit_id_buffer, 
                0, FP_SCREEN_WIDTH, 
                frame, 
                &levels[cur_level_idx], player_x, player_y, player_z, player_ang, pitch,
                editor_mode_enabled, editor_selected_map_idx, editor_selected_side
            );

            //thread_pool_add_work(thread_pool, &draw_first_person_level, &thread_args[0]);
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
            //printf("px py %f %f\n", player_x, player_y);
        } EndDrawing();
        float frame_time_ms = GetFrameTime()*1000.0f;
        update_player(frame_time_ms, mouse_delta);
        printf("%.2ffps\n", 1000.0f/frame_time_ms);
        //printf("%.3fms\n", frame_time_ms);
        frame++;
    }


    levels[cur_level_idx].start_x = player_x;
    levels[cur_level_idx].start_y = player_y;
    if(!SaveFileData(MAP_SAVE_FILE, levels, sizeof(levels))) {
        printf("Error saving file :(\n");
    }
}