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
    union {
        struct {
            float x,y,z;
        };
        float vals[3];
    };
} float3;

typedef struct {
    float x,y,z,w;
} float4;

#define mk_float2(nx,ny) ((float2){.x=(nx),.y=(ny),})
#define mk_float3(nx,ny,nz) ((float3){.x=(nx),.y=(ny),.z=(nz)})
#define mk_float4(nx,ny,nz,nw) ((float4){.x=(nx),.y=(ny),.z=(nz),.w=(nw)})


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
    float a,b,c;
    float d,e,f;
    float g,h,i;
} mat3;

#define mk_mat4(na,nb,nc,nd,ne,nf,ng,nh,ni,nj,nk,nl,nm,nn,no,np) \
    ((mat4){.a=na,.b=nb,.c=nc,.d=nd,.e=ne,.f=nf,.g=ng,.h=nh,.i=ni,.j=nj,.k=nk,.l=nl,.m=nm,.n=nn,.o=no,.p=np}) 

#define mk_mat3(na,nb,nc,nd,ne,nf,ng,nh,ni) \
    ((mat3){.a=na,.b=nb,.c=nc,.d=nd,.e=ne,.f=nf,.g=ng,.h=nh,.i=ni}) 


float2 float2_lerp(float2 a, float2 b, float c);
float2 float2_normalize(float2 a);
float3 float3_normalize(float3 a);
mat4 get_world_to_screen_matrix(camera cam);
float3 calc_vanishing_point_world(camera cam);
float2 project_vanishing_point_world_to_screen(camera cam, float3 vp_world);
camera mk_camera(
    float posx, float posy, float posz, 
    float pitch, float yaw,
    float render_width, float render_height, 
    float near_clip_plane, float far_clip_plane
);
segment get_segment_parameters(
    int segment_index, camera cam,
    float2 screen_vp, float dist_to_other_end,
    float2 neutral, int primary_axis, int world_y_max,
    int max_ray_count);

void execute_rays_in_segment(
    u32* ray_buffer,
    int ray_buffer_base_offset,
    segment seg,
    camera cam,
    mat4 world_to_screen_mat,
    int axis_mapped_to_y,
    level* this_level
);

#endif