#ifndef SIXDOF_H
#define SIXDOF_H

#include "common.h"

typedef struct {
    union {
        struct { 
            float x,y;
        };
        float vals[2];
    };
} float2;

typedef struct {
    float x,y,z;
} float3;

typedef struct {
    float x,y,z,w;
} float4;
typedef struct {
    float x,y,z,u,v;
} float5;

typedef struct {
    int x, y;
} int2;

#define mk_int2(nx,ny) ((int2){.x=(nx),.y=(ny),})
#define mk_float2(nx,ny) ((float2){.x=(nx),.y=(ny),})
#define mk_float3(nx,ny,nz) ((float3){.x=(nx),.y=(ny),.z=(nz)})
#define mk_float4(nx,ny,nz,nw) ((float4){.x=(nx),.y=(ny),.z=(nz),.w=(nw)})
#define mk_float5(nx,ny,nz,nu,nv) ((float5){.x=(nx),.y=(ny),.z=(nz),.u=(nu),.v=(nv)})


typedef struct {
    float2 min_screen;
    float2 max_screen;
    float2 cam_local_plane_ray_min;
    float2 cam_local_plane_ray_max;
    int next_free_pixel_min;
    int next_free_pixel_max;
    int ray_count;
    int index;
} segment;

typedef struct {
    float3 forward, right, up, pos;
    float2 dims;
    float near_clip, far_clip, fov;
} camera;

typedef struct {
    float a,b,c,d;
    float e,f,g,h;
    float i,j,k,l;
    float m,n,o,p;
} mat4;

typedef struct {
    union {
        struct {
            float a,b,c;
            float d,e,f;
            float g,h,i;
        };
        float m[3][3];
    };
} mat3;

#define mk_mat4(na,nb,nc,nd,ne,nf,ng,nh,ni,nj,nk,nl,nm,nn,no,np) \
    ((mat4){.a=na,.b=nb,.c=nc,.d=nd,.e=ne,.f=nf,.g=ng,.h=nh,.i=ni,.j=nj,.k=nk,.l=nl,.m=nm,.n=nn,.o=no,.p=np}) 

#define mk_mat3(na,nb,nc,nd,ne,nf,ng,nh,ni) \
    ((mat3){.a=na,.b=nb,.c=nc,.d=nd,.e=ne,.f=nf,.g=ng,.h=nh,.i=ni}) 


float2 float2_lerp(float2 a, float2 b, float c);
float2 float2_sub(float2 a, float2 b);
float2 float2_normalize(float2 a);
float3 float3_normalize(float3 a);
mat4 get_world_to_screen_matrix(camera cam);
float3 calc_vanishing_point_world(camera cam);
float2 project_vanishing_point_world_to_screen(camera cam, float3 vp_world);
camera mk_camera(
    float fov,
    float posx, float posy, float posz, 
    float pitch, float yaw, float roll,
    float render_width, float render_height, 
    float near_clip_plane, float far_clip_plane
);
segment get_segment_parameters(
    int segment_index, camera cam,
    float2 screen_vp, float dist_to_other_end,
    float2 neutral, int primary_axis, int world_y_max,
    int max_ray_count);

float3 adjust_screen_pixel_for_mesh(float2 screen_pixel, float2 screen_size);

extern s64 segment_raycast_finished[4];

void launch_parallel_raycast_segments(    
    u32* ray_buffers[2],
    float* z_buffers[2],
    segment segs[4],
    camera cam,
    mat4 world_to_screen_mat,
    level* this_level, 
    int seg_buffer_heights[2],
    int edit_id_render_enabled,
    int editor_mode_enabled,
    int flash_frame,
    int editor_selected_map_idx,
    editor_selected_thing editor_selected_side
);

void join_6dof_raycast();
void init_6dof_module();

extern int draw_only_first_element, draw_only_second_element;

int point_in_north_east(float px, float pz);
int point_in_north_west(float px, float pz);

#endif