#ifndef RESOURCES_H
#define RESOURCES_H


#define NUM_TEXTURES 7
#define SKYBOX_TEX_IDX 15
#define NUM_SPRITES 30
//#define NUM_DECALS 4
//#define BLANK_DECAL_IDX 0

#define EMPTY_SPRITE_INDEX 64
#define INVENTORY_BOX_SPRITE 21
#define WHISKEY_SPRITE_INDEX 24
#define REVOLVER_SPRITE_INDEX 25
#define REVOLVER_FIRST_PERSON_SPRITE_INDEX 26
#define SMOKE_PARTICLE_IDX 27
#define YOU_DIED_IDX 28
#define YOU_WIN_IDX 29

#define GUNSHOT_WAV "resources\\gunshot.wav"

int load_resources();
extern const float sprite_scales[];
extern u32* camera_texture;

extern u32** textures;
extern u32** sprites;

extern u8* rle_tex_top_skips;
extern u8* rle_spr_top_skips;


#endif 