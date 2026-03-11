#include "common.h"
#include "my_defs.h"
#include "platform_win.h"
#include "resources.h"
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    TEXTURE,
    SPRITE
} asset_type;
typedef struct {
    const char* name;
    const asset_type type;
} asset;

u32* camera_texture = NULL;


const char* texture_assets[] = {
    "flat_tex0",
    "flat_tex1",
    "wall_tex0",
    "wall_tex1",
    "bookshelf",
    "grass",
    "church"
};

const char* sprite_assets[] = {
    "tree",
    "moss",
    "chandelier",
    "glass_window2",
    "glass_window3",
    "fence",
    "bush",
    "glass_window4",
    "wall_tex0",
    "GATO1",
    "GATO2",
    "GATO3",
    "GATO4",
    "GATO5",
    "GATO6",
    "GATO7",
    "GATO8",
    "GATO9",
    "GATO10",
    "fox1",
    "fox2",
    "inventory_box",
};


typedef enum {
    STANDARD_TEX,
    RLE_TEX,
    TRANSPARENT_TEX
} tex_type;

typedef struct {
    int num_runs;
    u8 run_skips[32];
    u8 run_lens[32];
    u32 run_cols[32];
} rle_column;

typedef struct {
    rle_column columns[32];
} rle_tex;


u32** textures;
u32** sprites;
u8* rle_tex_top_skips;
u8* rle_spr_top_skips;

tex_type* texture_types;

void load_resources() {

    const int num_sprite_assets = ((sizeof(sprite_assets)) / sizeof(char*));
    const int num_texture_assets = ((sizeof(texture_assets)) / sizeof(char*));
    const int num_assets = num_sprite_assets+num_texture_assets;


    int tex_idx = 0;
    int sprite_idx = 0;
    size_t tex_num_bytes = sizeof(u8)*4*TEX_SIZE*TEX_SIZE;
    size_t tex_num_pixels = sizeof(u32)*TEX_SIZE*TEX_SIZE;
    
    textures = my_malloc(sizeof(u32*)*16, "texture pointer array");
    sprites = my_malloc(sizeof(u32*)*NUM_SPRITES, "sprite pointer array");
    rle_tex_top_skips = my_calloc(sizeof(u8)*32*16, "texture skip array");
    rle_spr_top_skips = my_calloc(sizeof(u8)*32*NUM_SPRITES, "sprite skip array");

    //u32* backing_texture_data = my_calloc(sizeof(u32)*tex_num_pixels*(NUM_TEXTURES+NUM_SPRITES), "assets");
    char buf[64] = {'r','e','s','o','u','r','c','e','s','/'};
    for(int asset_idx = 0; asset_idx < num_assets; asset_idx++) {
        int is_texture = (asset_idx < num_texture_assets);
        const char* asset_name = is_texture ? texture_assets[asset_idx] : sprite_assets[asset_idx-num_texture_assets];
        int i = 0;
        while(asset_name[i] != '\0') { 
            buf[10+i] = asset_name[i]; i++;
        };
        buf[10+i++] = '.';
        buf[10+i++] = 't';
        buf[10+i++] = 'g';
        buf[10+i++] = 'a';
        buf[10+i++] = '\0';



        debug_printf("Loading %s...\n", buf);
        u8* tex_data = platform_load_image(buf, 32, 32);

        u32* pix_data = (u32*)tex_data;
        int got_non_zero_alpha = 0;
        int got_zero_alpha = 0;

        for(int i = 0; i < 32*32; i++) {
            u32 texel = pix_data[i];
            u32 texel_a = ((texel >> 24) & 0xFF);
            u32 texel_r = ((texel >> 16) & 0xFF);
            u32 texel_g = ((texel >> 8) & 0xFF);
            u32 texel_b = ((texel >> 0) & 0xFF);
            got_non_zero_alpha += (texel_a != 255 && texel_a != 0);
            got_zero_alpha += (texel_a == 0);
            float fa = texel_a/255.0f;
            texel_r *= fa;
            texel_g *= fa;
            texel_b *= fa;
            pix_data[i] = (texel_a<<24) | (texel_r<<16) | (texel_g<<8) | (texel_b);
        }
        debug_printf("pct zero alpha %f pct non-zero alpha %f\n", got_zero_alpha/(32.0f*32.0f), got_non_zero_alpha/(32.0f*32.0f));
        //if(got_zero_alpha && !got_non_zero_alpha) {
        //}

        if(!got_non_zero_alpha && (got_zero_alpha > (0.3*32.0f*32.0f))) {
            debug_printf("RLE TEXTURE!!!!\n");
            // rows are drawn as vertical columns on-screen
            for(int row = 0; row < 32; row++) {
                int base_idx = row*32;
                int skip_tex = 0;
                for(int col = 0; col < 32; col++) {
                    u32 texel = pix_data[base_idx+col];
                    u32 texel_a = ((texel >> 24) & 0xFF);
                    if(texel_a != 0) {
                        break;
                    }
                    skip_tex++;
                }
                if(is_texture) {
                    //rle_tex_top_skips[tex_idx*32+row] = skip_tex;
                } else {
                    rle_spr_top_skips[sprite_idx*32+row] = skip_tex;
                }
            }
        } else {
            debug_printf("NORMAL TEXTURE :(\n");
        }

        
        //if(is_rle_worthy(tex_data)) {
        //    tex_data = convert_to_rle(tex_data);
        //}
        
        if(tex_data == NULL) {
            debug_printf("ERROR LOADING ASSET %s\n", buf);
            exit(1);
        }
        debug_printf("Loaded.\n");

        //u32* data_ptr = backing_texture_data+(tex_num_pixels*asset_idx);
        //my_memcpy(data_ptr, tex_data, tex_num_bytes);

        debug_printf("Copied\n");

        
        //platform_unload_image(tex_data);
        if(is_texture) {
            textures[tex_idx++] = (u32*)tex_data;//data_ptr;
        } else {
            sprites[sprite_idx++] = (u32*)tex_data;//data_ptr;
        }
    }

    //camera_texture = my_calloc(32*32*sizeof(u32), "camera texture");
    //textures[tex_idx++] = camera_texture;


    debug_printf("Loading skybox tga\n");
    u8* skybox_tex_data = platform_load_image("resources/skybox.tga", SKYBOX_TEX_HEIGHT, SKYBOX_TEX_WIDTH);
    //skybox = my_calloc(4*SKYBOX_TEX_WIDTH*SKYBOX_TEX_HEIGHT, "skybox");
    //my_memcpy(skybox, skybox_tex_data, 4*SKYBOX_TEX_WIDTH*SKYBOX_TEX_HEIGHT);
    textures[SKYBOX_TEX_IDX] = (u32*)skybox_tex_data;
    //platform_unload_image(skybox_tex_data);
    debug_printf("Loaded\n");
    //Image height_tex = LoadImage("resources/flat_tex_heightmap.tga");
    //my_memcpy(heightmap, height_tex.data, 32*32);
    //UnloadImage(height_tex);
}
