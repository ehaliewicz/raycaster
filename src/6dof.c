//#include <stdio.h>
#include "6dof.h"
#include "common.h"
#include "my_defs.h"
#include "raycast.h"
#include "resources.h"

#include "platform.h"

float2 float2_mul_float2(float2 a, float2 b) {
    return mk_float2((a.x*b.x), (a.y*b.y));
}

float2 float2_add_float(float2 a, float b) {
    return mk_float2((a.x+b), (a.y+b));
}

float2 float2_sub_float2(float2 a, float2 b) {
    return mk_float2((a.x-b.x), (a.y-b.y));
}

float2 float2_add_float2(float2 a, float2 b) {
    return mk_float2((a.x+b.x), (a.y+b.y));
}

int2 int2_add_int2(int2 a, int2 b) {
    return mk_int2((a.x+b.x), (a.y+b.y));
}

float3 float3_add_float3(float3 a, float3 b) {
    return mk_float3((a.x+b.x), (a.y+b.y), (a.z+b.z));
}

float2 float2_lerp(float2 a, float2 b, float c) {
    return mk_float2(
        lerp(a.x, b.x, c),
        lerp(a.y, b.y, c)
    );
}

float3 float3_lerp(float3 a, float3 b, float c) {
    return mk_float3(
        lerp(a.x, b.x, c),
        lerp(a.y, b.y, c),
        lerp(a.z, b.z, c)
    );
}

float4 float4_lerp(float4 a, float4 b, float c) {
    return mk_float4(
        lerp(a.x, b.x, c),
        lerp(a.y, b.y, c),
        lerp(a.z, b.z, c),
        lerp(a.w, b.w, c)
    );
}

float2 float2_mul_float(float2 a, float b) {
    return mk_float2((a.x*b), (a.y*b));
}

float2 float2_frac(float2 a) {
    return float2_sub_float2(a, mk_float2(my_floorf(a.x), my_floorf(a.y)));
}

float3 float3_mul_float(float3 a, float b) {
    return mk_float3((a.x*b), (a.y*b), (a.z*b));
}

float4 float4_mul_float(float4 a, float b) {
    return mk_float4((a.x*b), (a.y*b), (a.z*b), (a.w*b));
}

float float2_dot_float2(float2 a, float2 b) {
    return a.x*b.x + a.y*b.y;
}

float float3_dot_float3(float3 a, float3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

float2 float2_set_axis(float2 v, int axis, float val) {
    if (axis == 0) {
        return mk_float2(val, v.y);
    } else {
        return mk_float2(v.x, val);
    }
}

const float k_epsilon_normal_sqrt = .0000000000001f;
float float2_angle(float2 from, float2 to) {
    float frmSqrMag = from.x*from.x + from.y*from.y;
    float toSqrMag = to.x*to.x + to.y*to.y;
    float denom = my_sqrtf(frmSqrMag * toSqrMag);
    if (denom < k_epsilon_normal_sqrt) {
        return 0.0f;
    }
    
    float dot = float2_dot_float2(from, to) * (1.0f/ denom);
    dot = CLAMP(dot, -1.0f, 1.0f);
    return my_acosf(dot) * 57.29f;
}
    

float float2_signed_angle(float2 from, float2 to) {
    float unsigned_angle = float2_angle(from, to);
    float sgn = my_signumf(from.x * to.y - from.y * to.x);
    return unsigned_angle * sgn;
}

float3 float3_add(float3 a, float3 b) {
    return mk_float3((a.x+b.x), (a.y+b.y), (a.z+b.z));
}
float3 float3_sub(float3 a, float3 b) {
    return mk_float3((a.x-b.x), (a.y-b.y), (a.z-b.z));
}

float2 float2_normalize(float2 a) {
    float len = my_sqrtf(a.x*a.x + a.y*a.y);
    return mk_float2((a.x/len), (a.y/len));
}

float3 float3_normalize(float3 a) {
    float len = my_sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
    return mk_float3((a.x/len), (a.y/len), (a.z/len));
}

float3 float3_cross_float3(float3 a, float3 b) {
    return mk_float3(
        ((a.y * b.z) - (a.z * b.y)),
        ((a.z * b.x) - (a.x * b.z)),
        ((a.x * b.y) - (a.y * b.x))
    );
}

mat4 mk_mat4_from_scale(float3 scale) {
    return mk_mat4(
        scale.x, 0.0f, 0.0f, 0.0f,
        0.0f, scale.y, 0.0f, 0.0f,
        0.0f, 0.0f, scale.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

mat4 mk_mat4_from_translation(float3 vector) {
    return mk_mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        vector.x, vector.y, vector.z, 1.0
    );
}

mat4 get_projection_matrix(camera cam) {
    float aspect = cam.dims.x/cam.dims.y;
    float z_near = cam.near_clip;
    float z_far = cam.far_clip;
    float fov_degrees = cam.fov;

    float xy_max = z_near * my_tanf(fov_degrees * PI / 360.0f);

    float y_min = -xy_max;
    float x_min = -xy_max;

    float width = xy_max - x_min;
    float height = xy_max - y_min;
    float depth = z_far - z_near;
    float q = -(z_far + z_near) / depth;
    float qn = -2.0f * z_far * z_near / depth;

    float w = 2.0f * z_near / width;
    w = w / aspect;
    float h = (2.0f * z_near / height);

    return mk_mat4(w,    0.0f, 0.0f, 0.0f,
                   0.0f, h,    0.0f, 0.0f,
                   0.0f, 0.0f, q,    -1.0f,
                   0.0f, 0.0f, qn,   0.0f);
}

mat4 get_world_to_camera_matrix(camera cam) {
    return mk_mat4(
        cam.right.x, cam.up.x, -cam.forward.x, 0.0f,
        cam.right.y, cam.up.y, -cam.forward.y, 0.0f,
        cam.right.z, cam.up.z, -cam.forward.z, 0.0f,
        -float3_dot_float3(cam.right, cam.pos), 
        -float3_dot_float3(cam.up, cam.pos), 
        float3_dot_float3(cam.forward, cam.pos), 1.0f
        //-cam.right.dot(cam.pos), -cam.up.dot(cam.pos), cam.forward.dot(cam.pos), 1
    );
}

mat4 mat4_invert(mat4 mat) {
    // extract the elements in row-column form. (matrix is stored column first)
    float a11 = mat.a;
    float a12 = mat.b;
    float a13 = mat.c;
    float a14 = mat.d;
    float a21 = mat.e;
    float a22 = mat.f;
    float a23 = mat.g;
    float a24 = mat.h;
    float a31 = mat.i;
    float a32 = mat.j;
    float a33 = mat.k;
    float a34 = mat.l;
    float a41 = mat.m;
    float a42 = mat.n;
    float a43 = mat.o;
    float a44 = mat.p;


    float a = a33 * a44 - a34 * a43;
    float b = a32 * a44 - a34 * a42;
    float c = a32 * a43 - a33 * a42;
    float d = a31 * a44 - a34 * a41;
    float e = a31 * a43 - a33 * a41;
    float f = a31 * a42 - a32 * a41;
    float g = a23 * a44 - a24 * a43;
    float h = a22 * a44 - a24 * a42;
    float i = a22 * a43 - a23 * a42;
    float j = a23 * a34 - a24 * a33;
    float k = a22 * a34 - a24 * a32;
    float l = a22 * a33 - a23 * a32;
    float m = a21 * a44 - a24 * a41;
    float n = a21 * a43 - a23 * a41;
    float o = a21 * a34 - a24 * a31;
    float p = a21 * a33 - a23 * a31;
    float q = a21 * a42 - a22 * a41;
    float r = a21 * a32 - a22 * a31;

    float det = (a11 * (a22 * a - a23 * b + a24 * c) -
            a12 * (a21 * a - a23 * d + a24 * e) +
            a13 * (a21 * b - a22 * d + a24 * f) -
            a14 * (a21 * c - a22 * e + a23 * f));

    if (det == 0.0f) {
        //_warnings.warn("Unable to calculate inverse of singular Matrix")
        return mat;
    }

    float pdet = 1.0f / det;
    float ndet = -pdet;

    return mk_mat4(pdet * (a22 * a - a23 * b + a24 * c),
                ndet * (a12 * a - a13 * b + a14 * c),
                pdet * (a12 * g - a13 * h + a14 * i),
                ndet * (a12 * j - a13 * k + a14 * l),
                ndet * (a21 * a - a23 * d + a24 * e),
                pdet * (a11 * a - a13 * d + a14 * e),
                ndet * (a11 * g - a13 * m + a14 * n),
                pdet * (a11 * j - a13 * o + a14 * p),
                pdet * (a21 * b - a22 * d + a24 * f),
                ndet * (a11 * b - a12 * d + a14 * f),
                pdet * (a11 * h - a12 * m + a14 * q),
                ndet * (a11 * k - a12 * o + a14 * r),
                ndet * (a21 * c - a22 * e + a23 * f),
                pdet * (a11 * c - a12 * e + a13 * f),
                ndet * (a11 * i - a12 * n + a13 * q),
                pdet * (a11 * l - a12 * p + a13 * r));
}

float4 mat4_mul_float4(mat4 mat, float4 vec) {

    float x = vec.x;
    float y = vec.y;
    float z = vec.z;
    float w = vec.w;
    return mk_float4(
        (x * mat.a + y * mat.e + z * mat.i + w * mat.m),
        (x * mat.b + y * mat.f + z * mat.j + w * mat.n),
        (x * mat.c + y * mat.g + z * mat.k + w * mat.o),
        (x * mat.d + y * mat.h + z * mat.l + w * mat.p)
    );
}


mat4 mat4_mul_mat4(mat4 a, mat4 b) {
    float a11 = a.a;
    float a12 = a.b;
    float a13 = a.c;
    float a14 = a.d;
    float a21 = a.e;
    float a22 = a.f;
    float a23 = a.g;
    float a24 = a.h;
    float a31 = a.i;
    float a32 = a.j;
    float a33 = a.k;
    float a34 = a.l;
    float a41 = a.m;
    float a42 = a.n;
    float a43 = a.o;
    float a44 = a.p;

    float b11 = b.a;
    float b12 = b.b;
    float b13 = b.c;
    float b14 = b.d;
    float b21 = b.e;
    float b22 = b.f;
    float b23 = b.g;
    float b24 = b.h;
    float b31 = b.i;
    float b32 = b.j;
    float b33 = b.k;
    float b34 = b.l;
    float b41 = b.m;
    float b42 = b.n;
    float b43 = b.o;
    float b44 = b.p;

    return mk_mat4(
        (a11 * b11 + a21 * b12 + a31 * b13 + a41 * b14),   (a12 * b11 + a22 * b12 + a32 * b13 + a42 * b14),
        (a13 * b11 + a23 * b12 + a33 * b13 + a43 * b14),   (a14 * b11 + a24 * b12 + a34 * b13 + a44 * b14),
        
        (a11 * b21 + a21 * b22 + a31 * b23 + a41 * b24),   (a12 * b21 + a22 * b22 + a32 * b23 + a42 * b24),
        (a13 * b21 + a23 * b22 + a33 * b23 + a43 * b24),   (a14 * b21 + a24 * b22 + a34 * b23 + a44 * b24),
        
        (a11 * b31 + a21 * b32 + a31 * b33 + a41 * b34),   (a12 * b31 + a22 * b32 + a32 * b33 + a42 * b34),
        (a13 * b31 + a23 * b32 + a33 * b33 + a43 * b34),   (a14 * b31 + a24 * b32 + a34 * b33 + a44 * b34),

        (a11 * b41 + a21 * b42 + a31 * b43 + a41 * b44),   (a12 * b41 + a22 * b42 + a32 * b43 + a42 * b44),
        (a13 * b41 + a23 * b42 + a33 * b43 + a43 * b44),   (a14 * b41 + a24 * b42 + a34 * b43 + a44 * b44)
    );
}


mat4 get_world_to_screen_matrix(camera cam) {
    mat4 world_to_cam_matrix = get_world_to_camera_matrix(cam);
    mat4 cam_to_screen_matrix = get_projection_matrix(cam);

    mat4 world_to_screen_matrix = mat4_mul_mat4(cam_to_screen_matrix, world_to_cam_matrix);
    world_to_screen_matrix = mat4_mul_mat4(
        // scale from -1,1 to -0.5,.5
        mk_mat4_from_scale(mk_float3(0.5f, 0.5f, 1.0f)), world_to_screen_matrix 
    );
    world_to_screen_matrix = mat4_mul_mat4(
        // translate from -0.5,.5 to 0,1
        mk_mat4_from_translation(mk_float3(0.5f, 0.5f, 1.0f)), world_to_screen_matrix 
    );

    world_to_screen_matrix = mat4_mul_mat4(
        // scale from 0,1 to 0,screen
        mk_mat4_from_scale(mk_float3(cam.dims.x, cam.dims.y, 1.0f)), world_to_screen_matrix 
    );
    return world_to_screen_matrix;
}

float3 mat3_mul_float3(mat3 mat, float3 vec) {

    float x = vec.x;
    float y = vec.y;
    float z = vec.z;
    return mk_float3(
        (x * mat.a + y * mat.d + z * mat.g),
        (x * mat.b + y * mat.e + z * mat.h),
        (x * mat.c + y * mat.f + z * mat.i)
    );
}

float3 calc_vanishing_point_world(camera cam) {
    float rot_y = 1.0f * (-cam.near_clip / -cam.forward.y);
    return float3_add(cam.pos, mk_float3(0, rot_y, 0));
}

float2 project_vanishing_point_world_to_screen(camera cam, float3 vp_world) {
    float3 forward = cam.forward;
    float3 right = cam.right;
    float3 up = cam.up;

    mat4 look_matrix = mk_mat4(
        right.x, right.y, right.z, 0.0f,
        up.x, up.y, up.z, 0.0f,
        forward.x, forward.y, forward.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    mat4 scale_mat = mk_mat4_from_scale(mk_float3(1.0f, 1.0f, -1.0f)); 
    mat4 inv_look_mat = mat4_invert(look_matrix);

    mat4 view_mat = mat4_mul_mat4(scale_mat, inv_look_mat);

    mat4 proj_mat = get_projection_matrix(cam);
    mat4 local_to_screen_matrix = mat4_mul_mat4(proj_mat, view_mat);

    //view_matrix = pyglet.math.Mat4.from_scale(pyglet.math.Vec3(1,1,-1)) @ look_matrix.__invert__()
    //proj_matrix = camera.get_projection_matrix()
    //local_to_screen_matrix = proj_matrix @ view_matrix


    float3 local_pos = float3_sub(vp_world, cam.pos);

    float4 cam_pos = mat4_mul_float4(local_to_screen_matrix, mk_float4(local_pos.x, local_pos.y, local_pos.z, 1.0f));
    if (my_fabsf(cam_pos.w) < 0.001f) {
        cam_pos = mk_float4(
            (cam_pos.x), (cam_pos.y), (cam_pos.z), (my_signumf(cam_pos.w) * .001f)
        );
    }
    return float2_mul_float2(
        float2_add_float(
            float2_mul_float(
                float2_mul_float(mk_float2(cam_pos.x, cam_pos.y), 
                                1.0f/cam_pos.w),
                0.5f),
            0.5f),
        cam.dims);
}

mat3 mat3_roll(float roll) {
    float cr = my_cosf(roll), sr = my_sinf(roll);
    return (mat3){
        cr, -sr, 0.0f,
        sr,  cr, 0.0f,
        0.0f,   0.0f,  1.0f   // assuming Z is forward
    };
}

mat3 mat3_pitch(float pitch) {
    float cp = my_cosf(pitch), sp = my_sinf(pitch);
    return (mat3){
        1.0f,  0.0f,   0.0f,
        0.0f,  cp, -sp,
        0.0f,  sp,  cp
    };
}

mat3 mat3_yaw(float yaw) {
    float cy = my_cosf(yaw), sy = my_sinf(yaw);
    return (mat3){
        cy, 0.0f, sy,
        0.0f,  1.0f, 0.0f,
       -sy, 0.0f, cy
    };
}

mat3 mat3_mul_mat3(mat3 a, mat3 b) {
    mat3 result;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            result.m[row][col] = 
                a.m[row][0] * b.m[0][col] +
                a.m[row][1] * b.m[1][col] +
                a.m[row][2] * b.m[2][col];
        }
    }
    return result;
}
camera mk_camera(
    float fov,
    float posx, float posy, float posz, 
    float pitch, float yaw, float roll,
    float render_width, float render_height, 
    float near_clip_plane, float far_clip_plane
) {
    camera res;
    float3 pos = mk_float3(posx, posy/8.0f, posz);

    float3 forward = mk_float3(0.000796278066f, 0.000999994227f, 0.999994457f);
    float3 right = mk_float3(0.99999398f, 0.0f, -0.000796277716f);
    float3 up = float3_cross_float3(forward, right);

    /*
    {
        // rotate by pitch first

        float sp = my_sinf(pitch);
        float cp = my_cosf(pitch);
        float tp = 1.0f-cp;
        float rx = right.x;
        float ry = right.y;
        float rz = right.z;



        mat3 pitch_rot_mat = mk_mat3(
            (tp*rx*rx+cp), (tp*rx*ry + rz*sp), (tp*rx*rz - ry*sp),
            (tp*rx*ry - rz*sp), (tp*ry*ry + cp), (tp*ry*rz + rx*sp),
            (tp*rx*rz + ry*sp), (tp*ry*rz - rx*sp), (tp*rz*rz + cp)
        );
        forward = float3_normalize(mat3_mul_float3(pitch_rot_mat, forward));
        right = float3_normalize(mat3_mul_float3(pitch_rot_mat, right));
        up = float3_normalize(mat3_mul_float3(pitch_rot_mat, up));
    }

    {
        float sy = my_sinf(yaw);
        float cy = my_cosf(yaw);
        float ty = 1.0f-cy;
        float x = up.x;
        float y = up.y;
        float z = up.z;

        mat3 yaw_rot_mat = mk_mat3(
            //t*x*x+cy,   0.0f, sy,
            //0.0f, 1.0f, 0.0f,
            //-sy,  0.0f, cy
            ty*x*x+cy, ty*x*y + z*sy, ty*x*z - y*sy,
            ty*x*y - z*sy, ty*y*y + cy, ty*y*z + x*sy,
            ty*x*z + y*sy, ty*y*z - x*sy, ty*z*z + cy
        );

        forward = float3_normalize( mat3_mul_float3(yaw_rot_mat, forward));
        right = float3_normalize(mat3_mul_float3(yaw_rot_mat, right));
        up = float3_normalize(mat3_mul_float3(yaw_rot_mat, up));
    }
    */
    
    


    forward.x = my_cosf(pitch) * my_cosf(yaw);
    forward.y = my_sinf(pitch);
    forward.z = my_cosf(pitch) * my_sinf(yaw); 

    float up_pitch = pitch+QUARTER_CIRCLE_RADS;
    up.x = my_cosf(up_pitch) * my_cosf(yaw);
    up.y = my_sinf(up_pitch);
    up.z = my_cosf(up_pitch) * my_sinf(yaw); 
    right = float3_cross_float3(up, forward); // left hand rule?

    float cr = my_cosf(roll);
    float sr = my_sinf(roll);

    right = float3_add_float3(
        float3_mul_float(right, cr), float3_mul_float(float3_cross_float3(forward, right), sr)
    );
    up = float3_cross_float3(forward, right);

    /*
    mat3 r = mat3_mul_mat3(mat3_yaw(roll), 
    mat3_mul_mat3(mat3_pitch(pitch), mat3_roll(yaw)));
    //mat3 r = mat3_from_euler(pitch, yaw, roll);
    forward = mat3_mul_float3(r, mk_float3(0,0,1));
    up = mat3_mul_float3(r, mk_float3(0,1,0));
    right = mat3_mul_float3(r, mk_float3(1,0,0));
    */

setup_dims:;

    float2 dims = mk_float2(render_width, render_height);

    return ((camera){
        .dims = dims,
        .near_clip = near_clip_plane,
        .far_clip = far_clip_plane,
        .fov = fov,
        .forward = forward,
        .up = up,
        .right = right,
        .pos = pos
    });
}

float2 transform_screen_space_pix_to_world_pix(camera cam, float2 pix) {
    float2 scaled_pix = float2_mul_float2(pix, mk_float2(1.0f/cam.dims.x, 1.0f/cam.dims.y));
    float2 off_pix = float2_add_float(scaled_pix, -0.5f);
    
    float2 doubled_pix = float2_mul_float(off_pix, 2.0f);
    float4 homogeneous_pix = mk_float4(doubled_pix.x, doubled_pix.y, 1.0f, 1.0f);

    float3 forward = cam.forward;
    float3 right = cam.right;
    float3 up = float3_cross_float3(cam.forward, cam.right);

    mat4 lookat_matrix = mk_mat4(
        right.x, right.y, right.z, 0.0f,
        up.x, up.y, up.z, 0.0f,
        forward.x, forward.y, forward.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    mat4 proj_matrix = get_projection_matrix(cam);
    mat4 inv_proj_matrix = mat4_invert(proj_matrix);
    mat4 scale_inv_proj_matrix = mat4_mul_mat4(mk_mat4_from_scale(mk_float3(1.0f, 1.0f, -1.0f)), inv_proj_matrix); // from -1 scale z

    mat4 lookat_scale_inv_proj_matrix = mat4_mul_mat4(lookat_matrix, scale_inv_proj_matrix);
    float4 world_pix = mat4_mul_float4(lookat_scale_inv_proj_matrix, homogeneous_pix);
    float2 unprojected_world_pix = float2_mul_float(mk_float2(world_pix.x, world_pix.z), 1.0f/world_pix.w);
    return unprojected_world_pix;
}


segment get_segment_parameters(
    int segment_index, camera cam,
    float2 screen_vp, float dist_to_other_end,
    float2 neutral, int primary_axis, int world_y_max,
    int max_ray_count) {

    int secondary_axis = 1 - primary_axis;

    segment seg;
    seg.ray_count = 0;
    seg.index = segment_index;
    seg.cam_local_plane_ray_max.x = 0.0f;
    seg.cam_local_plane_ray_max.y = 0.0f;
    seg.max_screen = mk_float2(0.0f, 0.0f);
    seg.min_screen = mk_float2(0.0f, 0.0f);
    seg.next_free_pixel_max = 0;
    seg.next_free_pixel_min = 0;




    // setup the end points for the 2 45 degree angle rays
    float scmin = screen_vp.vals[secondary_axis] - dist_to_other_end;
    float2 simple_case_min = mk_float2(scmin, scmin);
    float scmax = screen_vp.vals[secondary_axis] + dist_to_other_end;
    float2 simple_case_max = mk_float2(scmax, scmax);
    float a = screen_vp.vals[primary_axis] + dist_to_other_end * my_signumf(neutral.vals[primary_axis]);

    simple_case_min = float2_set_axis(simple_case_min, primary_axis, a);
    simple_case_max = float2_set_axis(simple_case_max, primary_axis, a);


    if (simple_case_max.vals[secondary_axis] <= 0.0f || simple_case_min.vals[secondary_axis] >= cam.dims.vals[secondary_axis]) {
        return seg;
    }
    if (screen_vp.x >= 0 && screen_vp.y >= 0 && screen_vp.x <= cam.dims.x && screen_vp.y <= cam.dims.y) {
        // vp within bounds, so nothing to clamp angle wise
        seg.min_screen = simple_case_min;
        seg.max_screen = simple_case_max;
    } else {
        // vp outside of bounds, so we want to check if we can clamp the segment to the screen area
        // to prevent wasting buffer space
        float2 dir_simple_middle = float2_sub_float2(float2_lerp(simple_case_min, simple_case_max, 0.5f), screen_vp);
        float angle_left = 90.0f;
        float angle_right = -90.0f;
        float2 dir_left = mk_float2(0.0f, 0.0f);
        float2 dir_right = mk_float2(0.0f, 0.0f);

        float2 vectors[4] = {
            mk_float2(0.0f,0.0f), 
            mk_float2(0.0f, cam.dims.y), 
            mk_float2(cam.dims.x, 0.0f), 
            cam.dims
        };

        for(int i = 0; i < 4; i++) {
            float2 dirvec = float2_sub_float2(vectors[i], screen_vp);
            float2 scaled_end = float2_mul_float(dirvec, (dist_to_other_end / my_fabsf(dirvec.vals[primary_axis])));
            float angle = float2_signed_angle(neutral, dirvec);

            if (angle < angle_left) {
                angle_left = angle;
                dir_left = scaled_end;
            }
            if (angle > angle_right) {
                angle_right = angle;
                dir_right = scaled_end;
            }
        }

        float2 corner_left = float2_add_float2(dir_left, screen_vp);
        float2 corner_right = float2_add_float2(dir_right, screen_vp);

        if (angle_left < -45.0f) {
            // fallback to whatever the simple case left corner was
            float sgn = float2_signed_angle(dir_simple_middle, simple_case_max);
            if (sgn > 0.0f) {
                corner_left = simple_case_min;
            } else {
                corner_left = simple_case_max;
            }
        }
            
        if (angle_right > 45.0f) {
            float sgn = float2_signed_angle(dir_simple_middle, simple_case_max);
            if (sgn < 0.0f) {
                corner_right = simple_case_min;
            } else {
                corner_right = simple_case_max;
            }
        }


        int swap = corner_left.vals[secondary_axis] > corner_right.vals[secondary_axis];

        // todo is this correct?
        seg.min_screen = swap ? corner_right : corner_left;
        seg.max_screen = swap ? corner_left : corner_right;
        //segment.min_screen = corner_right if swap else corner_left
        //segment.max_screen = corner_left if swap else corner_right
    }

    seg.cam_local_plane_ray_min = transform_screen_space_pix_to_world_pix(cam, seg.min_screen);
    seg.cam_local_plane_ray_max = transform_screen_space_pix_to_world_pix(cam, seg.max_screen);


    int ray_count = my_roundf(seg.max_screen.vals[secondary_axis] - seg.min_screen.vals[secondary_axis]);
    seg.ray_count = CLAMP(ray_count, 0, max_ray_count); //(min(ray_count, max_ray_count), 0);
    
    return seg;
}


typedef struct {
    float3 plane_bottom, plane_top, plane_ray_direction;
} projected_plane_params;

projected_plane_params get_projected_plane_params(
    mat4 world_to_screen_mat,
    float2 ray_start, float2 ray_dir,
    float world_max_y, int y_axis
) {
    float4 plane_start_bottom = mk_float4(ray_start.x, 0.0f, ray_start.y, 1.0f);
    float4 plane_start_top = mk_float4(ray_start.x, world_max_y, ray_start.y, 1.0f);
    float4 plane_ray_direction = mk_float4(ray_dir.x, 0.0f, ray_dir.y, 0.0f);

    float4 full_plane_start_top_projected = mat4_mul_float4(world_to_screen_mat, plane_start_top);
    float4 full_plane_start_bot_projected = mat4_mul_float4(world_to_screen_mat, plane_start_bottom);
    float4 full_plane_ray_direction_projected = mat4_mul_float4(world_to_screen_mat, plane_ray_direction);
    if (y_axis == 0) {
        return ((projected_plane_params){
            mk_float3(full_plane_start_bot_projected.x, full_plane_start_bot_projected.z, full_plane_start_bot_projected.w),
            mk_float3(full_plane_start_top_projected.x, full_plane_start_top_projected.z, full_plane_start_top_projected.w),
            mk_float3(full_plane_ray_direction_projected.x, full_plane_ray_direction_projected.z, full_plane_ray_direction_projected.w)
        });
    } else {
        return ((projected_plane_params){
            mk_float3(full_plane_start_bot_projected.y, full_plane_start_bot_projected.z, full_plane_start_bot_projected.w),
            mk_float3(full_plane_start_top_projected.y, full_plane_start_top_projected.z, full_plane_start_top_projected.w),
            mk_float3(full_plane_ray_direction_projected.y, full_plane_ray_direction_projected.z, full_plane_ray_direction_projected.w)
        });
    }
}

typedef struct {
    float4 top, bot;
    u8 on_screen;
} clip_res;

clip_res clip_homogeneous_camera_space_line(
    float5 a, float5 b
) {
    
    float ax = a.x;
    float ay = a.y;
    float az = a.z;
    float au = a.u;
    float av = a.v;

    float bx = b.x;
    float by = b.y;
    float bz = b.z;
    float bu = b.u;
    float bv = b.v;

    if (ay < NEAR_PLANE_DIST) {
        if (by < NEAR_PLANE_DIST) {
            return ((clip_res){.on_screen = 0, .top=mk_float4(ax,az,au,av), .bot=mk_float4(bx,bz,bu,bv)});
        }
        float v = (by-NEAR_PLANE_DIST) / (by - ay);
        float4 clip_a = float4_lerp(mk_float4(bx,bz,bu,bv), mk_float4(ax,az,au,av), v);
        return ((clip_res){.on_screen=1, .top=clip_a, .bot=mk_float4(bx,bz,bu,bv)});
    } else if (by <= 0.0f) {
        float v = (ay-NEAR_PLANE_DIST) / (ay - by);
        float4 clip_b = float4_lerp(mk_float4(ax,az,au,av), mk_float4(bx,bz,bu,bv), v);
        return ((clip_res){.on_screen=1, .top=mk_float4(ax,az,au,av), .bot=clip_b});
    } else {
        return ((clip_res){.on_screen=1, .top=mk_float4(ax,az,au,av), .bot=mk_float4(bx,bz,bu,bv)});
    }
}

#define SWAP(type, a, b) {      \
    type tmp = (a);             \
    (a) = (b);                  \
    (b) = (tmp);                \
    } while(0)

#define PIXEL_CACHE_SIZE 256
u8 per_thread_seen_pixel_cache[NUM_THREADS][PIXEL_CACHE_SIZE]; // up to 2048 pixels

int is_pixel_set(u8 *pix_buf, int pix) {
    int byte = (pix>>3);
    int bit = (pix&0b111);
    return ((pix_buf[byte] >> bit)&1);
}
void mark_pixel(u8 *pix_buf, int pix) {
    int byte = (pix>>3);
    int bit = (pix&0b111);
    pix_buf[byte] |= (1 << bit);
}

typedef struct {
    int top, bot;
} new_screen_bounds;

#define NIGHT_FOG_COL ((255<<24)|(0<<16)|(0<<8)|(0<<0))
#define FOG_COL ((255<<24)|(103<<16)|(162<<8)|(196<<0))


new_screen_bounds fill_raybuffer_column(
    float4 cam_space_top, float4 cam_space_bot,
    int cur_next_free_pix_min, int cur_next_free_pix_max, 
    int original_next_free_pix_min, 
    int original_next_free_pix_max,
    u8* seen_pixel_cache,
    u32* pix_arr_col, int seg_buffer_height,
    u32* texture
    //int use_flat_color,
    //u32 flat_col
) {
    int use_flat_color = 0;
    u32 flat_col = 0;

    if(texture == textures[SKYBOX_TEX_IDX]) {
        use_flat_color = 1;
        flat_col = 0xFFB7CEFA;
    }


    // calculate the position in the ray buffer
    float one_over_z_top = 1.0f / cam_space_top.y;
    float one_over_z_bot = 1.0f / cam_space_bot.y;
    float u_top = cam_space_top.z;
    float v_top = cam_space_top.w;
    float u_bot = cam_space_bot.z;
    float v_bot = cam_space_bot.w;
    float u_over_z_top = u_top * one_over_z_top;
    float v_over_z_top = v_top * one_over_z_top;
    float u_over_z_bot = u_bot * one_over_z_bot;
    float v_over_z_bot = v_bot * one_over_z_bot;
    float ray_buffer_bounds_float_min = cam_space_top.x * one_over_z_top;
    float ray_buffer_bounds_float_max = cam_space_bot.x * one_over_z_bot;


    // flip min and max if necessary
    if (ray_buffer_bounds_float_max < ray_buffer_bounds_float_min) {
        SWAP(float, ray_buffer_bounds_float_max, ray_buffer_bounds_float_min);
        SWAP(float, one_over_z_bot, one_over_z_top);
        SWAP(float, u_over_z_top, u_over_z_bot);
        SWAP(float, v_over_z_top, v_over_z_bot);
    }

    int original_ray_buffer_bounds_min = my_roundf(ray_buffer_bounds_float_min);
    //original_ray_buffer_bounds_max = ray_buffer_bounds_float_max

    int num_pix = (ray_buffer_bounds_float_max+1) - ray_buffer_bounds_float_min;
    float one_over_z_per_pix = (one_over_z_bot-one_over_z_top) / num_pix;
    float du_over_z_per_pix = (u_over_z_bot-u_over_z_top)/num_pix;
    float dv_over_z_per_pix = (v_over_z_bot-v_over_z_top)/num_pix;


    // round to an integer position
    int ray_buffer_bounds_min = my_roundf(ray_buffer_bounds_float_min);
    int ray_buffer_bounds_max = my_roundf(ray_buffer_bounds_float_max);

    // check if within screen-space drawable bounds
    if (ray_buffer_bounds_max >= original_next_free_pix_min && ray_buffer_bounds_float_min <= original_next_free_pix_max) {
        // if this visible chunk touches the top screen bound
        // shrink top of frustum as much as possible
        if (ray_buffer_bounds_min <= cur_next_free_pix_min) {
            // simple and works, but doesn't continuously shrink the top
            ray_buffer_bounds_min = cur_next_free_pix_min;
            cur_next_free_pix_min = ray_buffer_bounds_max+1;
            if (ray_buffer_bounds_max >= cur_next_free_pix_min) {
                cur_next_free_pix_min = ray_buffer_bounds_max+1;
                while (cur_next_free_pix_min <= original_next_free_pix_max && is_pixel_set(seen_pixel_cache, cur_next_free_pix_min) > 0) {
                    cur_next_free_pix_min += 1;
                }
            }
        }

        // if this visible chunk touches the bottom screen bound
        // shrink bottom frustum as much as possible
        if (ray_buffer_bounds_max >= cur_next_free_pix_max) {
            ray_buffer_bounds_max = cur_next_free_pix_max;
            cur_next_free_pix_max = ray_buffer_bounds_min - 1;
            if (ray_buffer_bounds_min <= cur_next_free_pix_max) {
                cur_next_free_pix_max = ray_buffer_bounds_min-1;
                while (cur_next_free_pix_max >= original_next_free_pix_min && is_pixel_set(seen_pixel_cache, cur_next_free_pix_max) > 0) {
                    cur_next_free_pix_max -= 1;
                }
            }
        }

            
        u32 fog_r = (FOG_COL >> 16)&0xFF;
        u32 fog_g = (FOG_COL >> 8)&0xFF;
        u32 fog_b = (FOG_COL >> 0)&0xFF;

        // now draw visible portion of the chunk
        //pix_arr_col = pix_arr[col]
        //dy = len(pix_arr_col)-1
        int dy = seg_buffer_height-1;
        float z = one_over_z_top;
        if(use_flat_color) { 
            goto exit;
        }
        for (int y = ray_buffer_bounds_min; y < ray_buffer_bounds_max+1; y++) {
            //int dont_write = is_pixel_set(seen_pixel_cache, y);
            //u32 old_texel = pix_arr_col[(dy-y)];
            if (is_pixel_set(seen_pixel_cache, y) == 0) {
                mark_pixel(seen_pixel_cache, y);
                int dy_for_depth = y - original_ray_buffer_bounds_min;
                float inv_z = one_over_z_top + one_over_z_per_pix * dy_for_depth;
                float z = 1.0/inv_z;
                float depth_scale = z*RECIP_DARK_DIST;
                float inv_depth_scale = 1.0f - depth_scale;
                const float mult = 1.0f;

                float mult_by_inv_depth = mult * inv_depth_scale;
                float u_over_z = u_over_z_top + (dy_for_depth * du_over_z_per_pix);
                float v_over_z = v_over_z_top + (dy_for_depth * dv_over_z_per_pix);

                float u = u_over_z * z * 32;
                float v = v_over_z * z * 32;
                int iu = (int)u & 31;
                int iv = (int)v & 31;
                
                u32 scaled_fog_r = (depth_scale * fog_r);
                u32 scaled_fog_g = (depth_scale * fog_g);
                u32 scaled_fog_b = (depth_scale * fog_b);


                u32 lit_texel;
                if (use_flat_color) {
                    lit_texel = flat_col;
                } else {

                    u32 texel = texture[iu*32+iv];

                    u32 texel_a = ((texel >> 24) & 0xFF);
                    u32 texel_r = ((texel >> 16) & 0xFF);
                    u32 texel_g = ((texel >> 8) & 0xFF);
                    u32 texel_b = ((texel >> 0) & 0xFF);
                    float r = texel_r;
                    float g = texel_g;
                    float b = texel_b;
                    r = (r * mult_by_inv_depth);
                    g = (g * mult_by_inv_depth);
                    b = (b * mult_by_inv_depth);

                    r += scaled_fog_r;
                    g += scaled_fog_g;
                    b += scaled_fog_b;
                    u32 intr = CLAMP((int)r, 0, 0xFF);
                    u32 intg = CLAMP((int)g, 0, 0xFF);
                    u32 intb = CLAMP((int)b, 0, 0xFF);
                    lit_texel = 0xFF000000|(intr<<16)|(intg<<8)|intb;

                }
                pix_arr_col[(dy-y)] = lit_texel;
            }
                    
        }
    }
    exit:;
    return ((new_screen_bounds){.top = cur_next_free_pix_min, .bot = cur_next_free_pix_max});
}


typedef struct {
    int2 pos;
    int2 step;
    float2 dir;
    float2 start;
    float2 t_max;
    float2 t_delta;
    float2 intersection_distances;
    wall_side enter_side_flag;
} ray_t;

ray_t make_ray(float2 cam_pos_xz, float2 norm_ray_dir) {
    int2 pos = mk_int2(my_floorf(cam_pos_xz.x), my_floorf(cam_pos_xz.y));
    float eps = 0.0000001f;
    float absDirX = my_fabsf(norm_ray_dir.x);
    float absDirY = my_fabsf(norm_ray_dir.y);
    float2 t_delta = mk_float2((1.0f / MAX(eps, absDirX)), (1.0f / MAX(eps, absDirY)));
    float2 sign_dir = mk_float2(my_signumf(norm_ray_dir.x), my_signumf(norm_ray_dir.y));
    int2 step = mk_int2(((int)sign_dir.x), ((int)sign_dir.y));

    float2 t_max = float2_mul_float2(
        t_delta,
        float2_add_float(
            float2_add_float2(  // add half sign?
                float2_mul_float2(
                    sign_dir,   // times sign
                    float2_mul_float(float2_frac(cam_pos_xz), -1.0f) // negative fraction
                ),
                float2_mul_float(sign_dir, 0.5f)
            ),
            0.5f
        )
    );
    wall_side enter_side_flag;
    if ((t_max.x - t_delta.x) > (t_max.y - t_delta.y)) {
        enter_side_flag = VERTICAL_SIDE;
    } else {
        enter_side_flag = HORIZONTAL_SIDE;
    }

    float2 t_max_minus_t_delta = float2_sub_float2(t_max, t_delta);

    float2 intersection_distances = mk_float2(
        MAX(t_max_minus_t_delta.x, t_max_minus_t_delta.y),
        MIN(t_max.x, t_max.y)
    );

    return ((ray_t){
        .pos = pos,
        .step = step,
        .start = cam_pos_xz,
        .dir = norm_ray_dir,
        .t_delta = t_delta,
        .t_max = t_max,
        .intersection_distances = intersection_distances,
        .enter_side_flag = enter_side_flag
    });
}

typedef struct {
    ray_t next_ray;
    int rem_x_steps;
    int rem_y_steps;
} ray_step_t;

ray_step_t step_ray(ray_t ray, float2 cam_pos_xz, float2 norm_ray_direction, int rem_x_steps, int rem_y_steps) {
    float2 t_max = ray.t_max;
    float2 t_delta = ray.t_delta;
    int2 position = ray.pos;
    int2 step = ray.step;

    float crossed_boundary_distance;
    if(t_max.x < t_max.y) {
        crossed_boundary_distance = t_max.x;
        t_max = float2_add_float2(t_max, mk_float2(t_delta.x, 0.0f)); // step x
        position = int2_add_int2(position, mk_int2(step.x, 0)); 
        rem_x_steps -= 1;
    } else {
        crossed_boundary_distance = t_max.y;
        t_max = float2_add_float2(t_max, mk_float2(0.0f, t_delta.y));
        position = int2_add_int2(position, mk_int2(0, step.y)); // step y
        rem_y_steps -= 1;
    }

    float intersection_distances_cur = crossed_boundary_distance;
    float intersection_distances_next;
    if(t_max.x < t_max.y) {
        intersection_distances_next = t_max.x;
    } else {
        intersection_distances_next = t_max.y;
    }

    ray.pos = position;
    ray.intersection_distances = mk_float2(intersection_distances_cur, intersection_distances_next);
    ray.t_max = t_max;

    return ((ray_step_t){
        .next_ray = ray,
        .rem_x_steps = rem_x_steps,
        .rem_y_steps = rem_y_steps
    });
}


int draw_only_first_element = 0;
int draw_only_second_element = 0;

void execute_rays_in_segment(
    u32* ray_buffer, u8* seen_pixel_cache,
    int ray_buffer_base_offset,
    segment seg,
    int start_ray, int end_ray,
    camera cam,
    mat4 world_to_screen_mat,
    int axis_mapped_to_y,
    level* this_level, int seg_buffer_height
) { 
    float world_max_y = MAX_WALL_HEIGHT;
    float one_over_world_max_y = 1.0f / world_max_y;
    float camera_pos_y_normalized = cam.pos.y / world_max_y;

    u8* cur_level_floor = this_level->floor;
    u8* cur_level_ceil = this_level->ceil;
    u8* cur_level_upper_floor = this_level->upper_floor;
    u8* cur_level_upper_ceil = this_level->upper_ceil;

    //int iteration_direction = cam.forward.y < 0.0 ? 1 : -1;
    int top_down = cam.forward.y < 0.0f;


    float2 cam_pos_xz = mk_float2(cam.pos.x, cam.pos.z);

    for(int ray_segment_idx = start_ray; ray_segment_idx < end_ray; ray_segment_idx++) {
    //for(int ray_segment_idx = 0; ray_segment_idx < seg.ray_count; ray_segment_idx++) {


        float end_ray_lerp = (float)ray_segment_idx/seg.ray_count;
        float2 cam_local_plane_ray_direction = float2_lerp(seg.cam_local_plane_ray_min, seg.cam_local_plane_ray_max, end_ray_lerp);
        float2 norm_ray_direction = float2_normalize(cam_local_plane_ray_direction);
        ray_t ray = make_ray(cam_pos_xz, norm_ray_direction);

        int ray_column_idx = ray_buffer_base_offset + ray_segment_idx;

        u32* ray_column = &ray_buffer[(ray_column_idx*seg_buffer_height)];

        for(int i = 0; i < PIXEL_CACHE_SIZE; i++) { seen_pixel_cache[i] = 0; }

        float2 ray_start = cam_pos_xz;
        float2 ray_dir = norm_ray_direction;

        projected_plane_params plane = get_projected_plane_params(
            world_to_screen_mat, 
            ray_start, ray_dir, world_max_y, axis_mapped_to_y
        );


        
        float3 plane_top = plane.plane_top;
        float3 plane_bot = plane.plane_bottom;
        float3 plane_dir = plane.plane_ray_direction;

        int in_start_cell = 1;

        const int original_next_free_pix_min = seg.next_free_pixel_min;
        const int original_next_free_pix_max = seg.next_free_pixel_max;

        int map_x = ray.pos.x;
        int map_z = ray.pos.y;
        float cur_intersection_distance = ray.intersection_distances.x;
        float next_intersection_distance = ray.intersection_distances.y;
        float ray_origin_x = ray_start.x;
        float ray_origin_z = ray_start.y;
        float ray_dir_x = ray_dir.x;
        float ray_dir_z = ray_dir.y;
        int step_x = ray.step.x;
        int step_z = ray.step.y;


        int x_steps = (ray.step.x == 1 ? (MAP_SIZE-1-map_x) : (map_x));
        int y_steps = (ray.step.y == 1 ? (MAP_SIZE-1-map_z) : (map_z));//32;

        int cur_next_free_pix_min = original_next_free_pix_min;
        int cur_next_free_pix_max = original_next_free_pix_max;

        float wall_u = 0.0f;
        float default_exit_u = (ray_dir_x >= 0.0f) ? 1.0f : 0.0f;
        float default_exit_v = (ray_dir_z >= 0.0f) ? 1.0f : 0.0f;

        float default_start_u = 1.0f - default_exit_u;
        float default_start_v = 1.0f - default_exit_v;

        wall_side side = ray.enter_side_flag;

        float far_clip = cam.far_clip;
        float near_clip = cam.near_clip;


        while(1) {
            if(cur_intersection_distance >= far_clip) {
                //int dy = seg_buffer_height-1;
                //for(int y = cur_next_free_pix_min; y < cur_next_free_pix_max; y++) {
                //    ray_column[dy-y] = 0xFFB7CEFA;
                //}
                break;
            }
            if(x_steps < 0 || y_steps < 0) {
                //int dy = seg_buffer_height-1;
                //for(int y = cur_next_free_pix_min; y < cur_next_free_pix_max; y++) {
                //    ray_column[dy-y] = 0xFFB7CEFA;
                //}
                break;
            }

            int col_spans[2];
            int alt_col_spans[2];     
            int col_span_anchors[2];
            
            cell_types span_types[2];
            int map_idx = map_z*MAP_SIZE+map_x;
                col_spans[0] = this_level->ceil[map_idx];
                col_span_anchors[0] = this_level->ceil_anchor[map_idx];
                alt_col_spans[0] = this_level->upper_ceil[map_idx];
                span_types[0] = this_level->upper_cell_types[map_idx];

                col_spans[1] = this_level->floor[map_idx];
                col_span_anchors[1] = this_level->floor_anchor[map_idx];
                alt_col_spans[1] = this_level->upper_floor[map_idx];
                span_types[1] = this_level->lower_cell_types[map_idx];


            //int world_col_min = col_spans[1][1];
            //int world_col_max = col_spans[0][0];
            int num_col_spans = 2;

            

            float3 plane_ray_dir_times_cur_dist = float3_mul_float(plane_dir, cur_intersection_distance);
            float3 plane_ray_dir_times_next_dist = float3_mul_float(plane_dir, next_intersection_distance);

            float3 cam_space_min_last = float3_add_float3(plane_bot, plane_ray_dir_times_cur_dist);
            float3 cam_space_max_last = float3_add_float3(plane_top, plane_ray_dir_times_cur_dist);

            float3 cam_space_min_next = float3_add_float3(plane_bot, plane_ray_dir_times_next_dist);
            float3 cam_space_max_next = float3_add_float3(plane_top, plane_ray_dir_times_next_dist);
            
            float hit_x = ray_origin_x + ray_dir_x * cur_intersection_distance;
            float hit_z = ray_origin_z + ray_dir_z * cur_intersection_distance;
            float next_hit_x = ray_origin_x + ray_dir_x * next_intersection_distance;
            float next_hit_z = ray_origin_z + ray_dir_z * next_intersection_distance;

            int break_ray_loop = 0;
            //int top_down = iteration_direction == 1;

            int span_idx_start = top_down ? 0 : num_col_spans-1;
            int span_idx_end = top_down ? num_col_spans : -1;
            int iteration_direction = top_down ? 1 : -1;

            float flat_u, flat_v;
            u8 upper_wall_tex, lower_wall_tex;
            editor_selected_thing upper_intersect_wall_side, lower_intersect_wall_side;
            u8 lower_intersect_wall_light_level, upper_intersect_wall_light_level;
            if(side == VERTICAL_SIDE) {
                wall_u = hit_z - my_floorf(hit_z);
                flat_u = default_start_u;
                flat_v = wall_u;
                if(ray_dir_x > 0) {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_WEST;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_WEST;
                    upper_wall_tex = this_level->uwtex[map_idx];
                    lower_wall_tex = this_level->lwtex[map_idx];
                    upper_intersect_wall_light_level = this_level->uw_light[map_idx];
                    lower_intersect_wall_light_level = this_level->lw_light[map_idx];
                } else {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_EAST;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_EAST;
                    upper_wall_tex = this_level->uetex[map_idx];
                    lower_wall_tex = this_level->letex[map_idx];
                    upper_intersect_wall_light_level = this_level->ue_light[map_idx];
                    lower_intersect_wall_light_level = this_level->le_light[map_idx];
                }
            } else {
                if(ray_dir_z < 0) {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_SOUTH;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_SOUTH;
                    upper_wall_tex = this_level->ustex[map_idx];
                    lower_wall_tex = this_level->lstex[map_idx];
                    upper_intersect_wall_light_level = this_level->us_light[map_idx];
                    lower_intersect_wall_light_level = this_level->ls_light[map_idx];
                } else {
                    upper_intersect_wall_side = WALL_SIDE_UPPER_NORTH;
                    lower_intersect_wall_side = WALL_SIDE_LOWER_NORTH;
                    upper_wall_tex = this_level->untex[map_idx];
                    lower_wall_tex = this_level->lntex[map_idx];
                    upper_intersect_wall_light_level = this_level->un_light[map_idx];
                    lower_intersect_wall_light_level = this_level->ln_light[map_idx];
                }
                wall_u = hit_x - my_floorf(hit_x);
                flat_u = wall_u;
                flat_v = default_start_v;
            }
            float exit_flat_u, exit_flat_v;

            wall_side next_side;
            int prev_x_steps = x_steps;
            ray_step_t next_step = step_ray(ray, cam_pos_xz, norm_ray_direction, x_steps, y_steps);
            int next_map_x = next_step.next_ray.pos.x;
            int next_map_z = next_step.next_ray.pos.y;
            if(next_step.rem_x_steps != prev_x_steps) {
                next_side = VERTICAL_SIDE;
                exit_flat_u = default_exit_u;
                exit_flat_v = next_hit_z - my_floorf(next_hit_z);
            } else {
                next_side = HORIZONTAL_SIDE;
                exit_flat_u = next_hit_x - my_floorf(next_hit_x);
                exit_flat_v = default_exit_v;
            }

            for(int span_idx = span_idx_start; span_idx != span_idx_end; span_idx += iteration_direction) {
                if(draw_only_first_element && span_idx != 0) {
                    continue;
                }
                if(draw_only_second_element && span_idx != 0) {
                    continue;
                }
                int is_ceil = span_idx == 0;
                float element_bounds_max = is_ceil ? col_span_anchors[span_idx]/8.0f : col_spans[span_idx]/8.0f;
                float alt_element_bounds = alt_col_spans[span_idx]/8.0f;
                float element_bounds_min = (!is_ceil) ? col_span_anchors[span_idx]/8.0f : col_spans[span_idx]/8.0f;

                // for ceiling
                // max = anchor, min = ceil height
                // for floor
                // max = floor height, min = anchor

                cell_types cell_type = span_types[span_idx];
                int is_slope =  (cell_type == SLOPE_X || cell_type == SLOPE_Y);
                slope_heights slope = get_slope_heights(
                    in_start_cell, map_x, map_z, next_map_x, next_map_z, hit_x, hit_z, next_hit_x, next_hit_z,
                    side, cell_type, step_x, step_z, ray_origin_x, ray_origin_x,
                    is_ceil ? element_bounds_min : element_bounds_max,
                    alt_element_bounds
                );

                float portion_top = element_bounds_max * one_over_world_max_y;
                float portion_bot = element_bounds_min * one_over_world_max_y;
                float slope_start_portion = slope.start_height * one_over_world_max_y;
                float slope_end_portion = slope.end_height * one_over_world_max_y;


                // lerp between the cam space positions of the top and bottom of the ray plane
                // which are at the top and bottom of the world (min and max positions, eg 0 -> 64) in camera space
                float3 cam_space_front_top = float3_lerp(cam_space_min_last, cam_space_max_last, portion_top);
                float3 cam_space_front_bot = float3_lerp(cam_space_min_last, cam_space_max_last, portion_bot);

                if(cell_type == SLOPE_X || cell_type == SLOPE_Y) {
                    if(is_ceil) {
                        cam_space_front_bot = float3_lerp(cam_space_min_last, cam_space_max_last, slope_start_portion);
                    } else {
                        cam_space_front_top = float3_lerp(cam_space_min_last, cam_space_max_last, slope_start_portion);
                    }
                }

                float xt = cam_space_front_top.x;
                float yt = cam_space_front_top.y;
                float zt = cam_space_front_top.z;

                float xb = cam_space_front_bot.x;
                float yb = cam_space_front_bot.y;
                float zb = cam_space_front_bot.z;


                if(!in_start_cell) {
                    u32* wall_tex = is_ceil ? textures[upper_wall_tex] : textures[lower_wall_tex];
                    // draw front wall
                    float bot_u = (element_bounds_max-element_bounds_min)/1.0f;
                    float5 top = mk_float5(xt,yt,zt,wall_u, 0.0f);
                    float5 bot = mk_float5(xb,yb,zb,wall_u, bot_u);
                    clip_res wall_clipped = clip_homogeneous_camera_space_line(
                        top, bot
                    );
                    if(wall_clipped.on_screen) {
                        new_screen_bounds bnds = fill_raybuffer_column(
                            wall_clipped.top, wall_clipped.bot,
                            cur_next_free_pix_min, cur_next_free_pix_max,
                            original_next_free_pix_min, original_next_free_pix_max,
                            seen_pixel_cache, ray_column, seg_buffer_height, 
                            wall_tex
                            //, 0, 0
                        );

                        cur_next_free_pix_min = bnds.top;
                        cur_next_free_pix_max = bnds.bot;

                        if(cur_next_free_pix_min > cur_next_free_pix_max) {
                            break_ray_loop = 1;
                            break;
                        }
                    }
                }


                float3 flat_exit_cam_space, flat_enter_cam_space;

                // do we draw the top or bottom of this cell portion

                // really though, it's just a matter of whether it's the floor or ceiling..
                if(!is_ceil) { // portion_top < camera_pos_y_normalized) {
                    flat_exit_cam_space = float3_lerp(cam_space_min_next, cam_space_max_next, portion_top);
                    flat_enter_cam_space = cam_space_front_top;
                } else {
                    flat_exit_cam_space = float3_lerp(cam_space_min_next, cam_space_max_next, portion_bot);
                    flat_enter_cam_space = cam_space_front_bot;
                }

                if(cell_type == SLOPE_X || cell_type == SLOPE_Y) {
                    if(!is_ceil) {
                        flat_exit_cam_space = float3_lerp(cam_space_min_next, cam_space_max_next, slope_end_portion);
                    } else {
                        //flat_exit_cam_space
                        flat_exit_cam_space = float3_lerp(cam_space_min_next, cam_space_max_next, slope_end_portion);
                    }
                }

                float ax = flat_exit_cam_space.x;
                float ay = flat_exit_cam_space.y;
                float az = flat_exit_cam_space.z;

                float bx = flat_enter_cam_space.x;
                float by = flat_enter_cam_space.y;
                float bz = flat_enter_cam_space.z;

                clip_res floor_clipped = clip_homogeneous_camera_space_line(
                    mk_float5(ax,ay,az, exit_flat_u, exit_flat_v),
                    mk_float5(bx,by,bz, flat_u, flat_v)
                );

                if(floor_clipped.on_screen) {
                    u32* floor_tex = is_slope ? textures[this_level->uftex[map_idx]] : textures[this_level->ftex[map_idx]];
                    u32* ceil_tex = is_slope ? textures[this_level->uctex[map_idx]] : textures[this_level->ctex[map_idx]];
                    u32* flat_tex = (span_idx == 0) ? ceil_tex : floor_tex;
                    new_screen_bounds bnds = fill_raybuffer_column(
                        floor_clipped.top, floor_clipped.bot,
                        cur_next_free_pix_min, cur_next_free_pix_max,
                        original_next_free_pix_min, original_next_free_pix_max,
                        seen_pixel_cache, ray_column, seg_buffer_height, 
                        flat_tex
                        //, 0, 0
                    );

                    cur_next_free_pix_min = bnds.top;
                    cur_next_free_pix_max = bnds.bot;

                    if(cur_next_free_pix_min > cur_next_free_pix_max) {
                        break_ray_loop = 1;
                        break;
                    }
                }

                if(break_ray_loop) {
                    break;
                }
            }


            if(break_ray_loop) {
                break;
            }

            ray = next_step.next_ray;
            x_steps = next_step.rem_x_steps;
            y_steps = next_step.rem_y_steps;
            cur_intersection_distance = ray.intersection_distances.x;
            next_intersection_distance = ray.intersection_distances.y;
            map_x = ray.pos.x;
            map_z = ray.pos.y;
            side = next_side;
            in_start_cell = 0;
        }
        
        
    }
}


float3 adjust_screen_pixel_for_mesh(float2 screen_pixel, float2 screen_size) { 
        return (float3){.x=2.0f * screen_pixel.x / screen_size.x - 1.0f, .y = 2.0f * screen_pixel.y/screen_size.y - 1.0f, .z=0.5f};
}


#define MIN_RAYS_PER_THREAD 16

typedef struct {
    u32* ray_buffer; u8* seen_pixel_cache;
    int ray_buffer_base_offset;
    segment seg;
    int start_ray; int end_ray;
    camera cam;
    mat4 world_to_screen_mat;
    int axis_mapped_to_y;
    level* this_level;
    int seg_buffer_height;
} thread_params;


void execute_rays_in_segment_wrapper(void* p) {
    thread_params* tp = (thread_params*)p;
    execute_rays_in_segment(
        tp->ray_buffer, tp->seen_pixel_cache, tp->ray_buffer_base_offset, tp->seg, 
        tp->start_ray, tp->end_ray, tp->cam, tp->world_to_screen_mat,
        tp->axis_mapped_to_y, tp->this_level, tp->seg_buffer_height
    );
}


jobpool* raycast_6dof_pool = NULL;
static jobpool* raycast_6dof_manager_pool = NULL;
static thread_params raycast_6dof_parms[NUM_THREADS];
static thread_params frame_params;

void parallel_raycast_segment(
    //thread_params *tp

    u32* ray_buffer,
    int ray_buffer_base_offset,
    segment seg,
    camera cam,
    mat4 world_to_screen_mat,
    int axis_mapped_to_y,
    level* this_level, int seg_buffer_height    
) {
    int used_threads = NUM_THREADS;
    int rays_per_thread = seg.ray_count / used_threads;
    //if(rays_per_thread < MIN_RAYS_PER_THREAD) {
    //    used_threads = seg.ray_count / MIN_RAYS_PER_THREAD;
    //    rays_per_thread = seg.ray_count / used_threads;
    //}

    for(int i = 0; i < used_threads; i++) {
        raycast_6dof_parms[i].ray_buffer = ray_buffer;
        raycast_6dof_parms[i].seen_pixel_cache = per_thread_seen_pixel_cache[i];
        raycast_6dof_parms[i].ray_buffer_base_offset = ray_buffer_base_offset;
        raycast_6dof_parms[i].seg = seg;
        raycast_6dof_parms[i].cam = cam;
        raycast_6dof_parms[i].world_to_screen_mat = world_to_screen_mat;
        raycast_6dof_parms[i].axis_mapped_to_y = axis_mapped_to_y;
        raycast_6dof_parms[i].this_level = this_level;
        raycast_6dof_parms[i].seg_buffer_height = seg_buffer_height;

        raycast_6dof_parms[i].start_ray = (rays_per_thread*i);
        raycast_6dof_parms[i].end_ray = MIN(seg.ray_count, (rays_per_thread*(i+1)));
    }
    raycast_6dof_parms[used_threads-1].end_ray = seg.ray_count;

    for(int i = 0; i < used_threads; i++) {
        platform_add_task(
            raycast_6dof_pool,
            execute_rays_in_segment_wrapper,
            &raycast_6dof_parms[i]
        );
    }
    platform_join_threadpool(raycast_6dof_pool);
}

void parallel_raycast_segment_wrapper(
    void* arg_var
    ) {
    thread_params* tp = (thread_params*)arg_var;
    parallel_raycast_segment(
        tp->ray_buffer, tp->ray_buffer_base_offset,
        tp->seg, tp->cam, tp->world_to_screen_mat, tp->axis_mapped_to_y,
        tp->this_level, tp->seg_buffer_height    
    );
}


typedef struct {
    int seg_idx; int ray_in_seg_start; int ray_in_seg_end;
} raycast_task;

typedef struct {
    int num_tasks;
    raycast_task raycast_task[4];
} thread_task_bundle;




void launch_parallel_raycast_segment(
    u32* ray_buffer,
    int ray_buffer_base_offset,
    segment seg,
    camera cam,
    mat4 world_to_screen_mat,
    int axis_mapped_to_y,
    level* this_level, int seg_buffer_height
) {
    frame_params.ray_buffer = ray_buffer;
    frame_params.ray_buffer_base_offset = ray_buffer_base_offset;
    frame_params.seg = seg;
    frame_params.cam = cam;
    frame_params.world_to_screen_mat = world_to_screen_mat;
    frame_params.axis_mapped_to_y = axis_mapped_to_y;
    frame_params.this_level = this_level;
    frame_params.seg_buffer_height = seg_buffer_height;
    platform_add_task(
        raycast_6dof_manager_pool,
        parallel_raycast_segment_wrapper,
        &frame_params
    );
}


void join_raycast_segment() {
    platform_join_threadpool(raycast_6dof_manager_pool);
}

void init_6dof_module() {
    raycast_6dof_pool = platform_init_threadpool(NUM_THREADS);
    raycast_6dof_manager_pool = platform_init_threadpool(1);
}