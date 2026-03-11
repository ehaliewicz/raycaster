#ifndef RESOURCES_H
#define RESOURCES_H


#define NUM_TEXTURES 7
#define SKYBOX_TEX_IDX 15
#define NUM_SPRITES 22
//#define NUM_DECALS 4
//#define BLANK_DECAL_IDX 0

#define EMPTY_SPRITE_INDEX 64

void load_resources();
extern u32* camera_texture;

extern u32** textures;
extern u32** sprites;

extern u8* rle_tex_top_skips;
extern u8* rle_spr_top_skips;

#define INVENTORY_BOX_SPRITE 21

#endif 