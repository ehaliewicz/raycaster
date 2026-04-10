#include <stdio.h>
#include "6dof.h"
#include "common.h"
#include "draw.h"
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

float3 float3_rotate(float3 v, float3 axis, float angle) {
    // v' = v*cos + (axis x v)*sin + axis*(axis . v)*(1 - cos)
    float c = my_cosf(angle);
    float s = my_sinf(angle);
    return float3_add(
        float3_add(
            float3_mul_float(v, c),
            float3_mul_float(float3_cross_float3(axis, v), s)
        ),
        float3_mul_float(axis, float3_dot_float3(axis, v) * (1.0f - c))
    );
}

void rotate_camera_descent_style(camera *cam, float delta_yaw, float delta_pitch) {
    // Yaw around local up (not world up)
    cam->forward = float3_rotate(cam->forward, cam->up, delta_yaw);
    cam->right   = float3_rotate(cam->right,   cam->up, delta_yaw);

    // Pitch around local right
    cam->forward = float3_rotate(cam->forward, cam->right, delta_pitch);
    cam->up      = float3_rotate(cam->up,      cam->right, delta_pitch);

    // Re-orthogonalize
    cam->forward = float3_normalize(cam->forward);
    cam->right   = float3_normalize(float3_cross_float3(cam->forward, cam->up));
    cam->up      = float3_normalize(float3_cross_float3(cam->right, cam->forward));
}

void get_pitch_yaw(camera *cam, float *pitch, float *yaw) {
    *yaw   = my_atan2f(cam->forward.x, cam->forward.z);  // rotation around Y in world space
    *pitch = my_asinf(cam->forward.y);                    // up/down tilt
}

mat4 get_world_to_camera_matrix(camera cam) {
    return mk_mat4(
        cam.right.x, cam.up.x, -cam.forward.x, 0.0f,
        cam.right.y, cam.up.y, -cam.forward.y, 0.0f,
        cam.right.z, cam.up.z, -cam.forward.z, 0.0f,
        -float3_dot_float3(cam.right, cam.pos), 
        -float3_dot_float3(cam.up, cam.pos), 
        float3_dot_float3(cam.forward, cam.pos), 1.0f
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
u8 per_thread_seen_pixel_cache[NUM_THREADS*4][PIXEL_CACHE_SIZE]; // up to 2048 pixels

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
    u32* pix_arr_col, float* z_buf_col, int seg_buffer_height,
    u32* texture,
    int edit_id_render_enabled,
    edit_wall_id wall_id
) {
    u32 flat_col;
    int use_flat_color = 0;
    if(edit_id_render_enabled) {
        use_flat_color = 1;
        flat_col = (0xFF<<24)|wall_id.full_val;
    } else {
        if(texture == textures[SKYBOX_TEX_IDX]) {
            use_flat_color = 1;
            flat_col = 0xFFB7CEFA;
        }
    }


    // calculate the position in the ray buffer  (y is depth in camera space)
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

    int original_ray_buffer_bounds_min = my_floorf(ray_buffer_bounds_float_min);
    //original_ray_buffer_bounds_max = ray_buffer_bounds_float_max

    int num_pix = (ray_buffer_bounds_float_max+1) - ray_buffer_bounds_float_min;
    float d_one_over_z = (one_over_z_bot-one_over_z_top) / num_pix;
    float du_over_z_per_pix = (u_over_z_bot-u_over_z_top)/num_pix;
    float dv_over_z_per_pix = (v_over_z_bot-v_over_z_top)/num_pix;


    // round to an integer position
    int ray_buffer_bounds_min = my_floorf(ray_buffer_bounds_float_min);
    int ray_buffer_bounds_max = my_floorf(ray_buffer_bounds_float_max);
    int y0 = ray_buffer_bounds_min;
    int y1 = ray_buffer_bounds_max;

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


        int dy = seg_buffer_height-1;
        float z = one_over_z_top;
       

        int clipped_y0 = ray_buffer_bounds_min;
        int clipped_y1 = ray_buffer_bounds_max;
        float cur_z = 1.0f / (one_over_z_top + d_one_over_z * (ray_buffer_bounds_min-y0));

        if(use_flat_color) {
            for (int y = ray_buffer_bounds_min; y < ray_buffer_bounds_max+1; y++) {
                if (is_pixel_set(seen_pixel_cache, y) == 0) {
                    mark_pixel(seen_pixel_cache, y);
                    pix_arr_col[(dy-y)] = flat_col;
                    z_buf_col[(dy-y)] = FAR_PLANE_DIST;
                }        
            }
        } else {
            for (int y = ray_buffer_bounds_min; y < ray_buffer_bounds_max+1; y++) {
                int dy_for_depth = y - y0;
                float next_inv_z = one_over_z_top + d_one_over_z * (1+dy_for_depth);
                float next_z = 1.0/next_inv_z;
                if (is_pixel_set(seen_pixel_cache, y) == 0) {
                    mark_pixel(seen_pixel_cache, y);
                    float depth_scale = cur_z*RECIP_DARK_DIST;
                    float inv_depth_scale = 1.0f - depth_scale;
                    const float mult = 1.0f;

                    float mult_by_inv_depth = mult * inv_depth_scale;
                    float u_over_z = u_over_z_top + (dy_for_depth * du_over_z_per_pix);
                    float v_over_z = v_over_z_top + (dy_for_depth * dv_over_z_per_pix);

                    float u = u_over_z * cur_z * 32.0f;
                    float v = v_over_z * cur_z * 32.0f;
                    int iu = (int)u & 31;
                    int iv = (int)v & 31;
                    
                    u32 scaled_fog_r = (depth_scale * fog_r);
                    u32 scaled_fog_g = (depth_scale * fog_g);
                    u32 scaled_fog_b = (depth_scale * fog_b);

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
                    u32 lit_texel = 0xFF000000|(intr<<16)|(intg<<8)|intb;

                    pix_arr_col[(dy-y)] = lit_texel;
                    z_buf_col[(dy-y)] = cur_z;
                }
         
                cur_z = next_z;
            }
        }
    }
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
    int past_far_clip;
} ray_step_t;

ray_step_t step_ray(ray_t ray, float2 cam_pos_xz, float2 norm_ray_direction, int rem_x_steps, int rem_y_steps, float far_clip) {
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
        .rem_y_steps = rem_y_steps,
        .past_far_clip = crossed_boundary_distance >= far_clip
    });
}
float cross2(float ax, float ay, float bx, float by) {
    return ax * by - ay * bx;
}

float clip_min(float3 p_min, float3 p_max, float frustum) {
    float frustum_inv = 1.0 / frustum;
    float c0 = cross2(1.0f, frustum_inv, p_max.x, p_max.z);
    float c1 = cross2(1.0f, frustum_inv, p_min.x, p_min.z);
    return 1.0f - (c0 / (c0 - c1));
}

float clip_max(float3 p_min, float3 p_max, float frustum) {
    float frustum_inv = 1.0f / frustum;
    float c0 = cross2(1.0f, frustum_inv, p_max.x, p_max.z);
    float c1 = cross2(1.0f, frustum_inv, p_min.x, p_min.z);
    return c1 / (c1 - c0);
}

typedef struct {
    int clipped;
    float min_lerp, max_lerp;
} world_bounds_clipping;

world_bounds_clipping get_world_bounds_clipping_cam_space(float3 p_min, float3 p_max, float frustum_bound_min, float frustum_bound_max) {
    // returns (clipped: bool, min_lerp: float, max_lerp: float)
    float min_lerp, max_lerp;
    if (p_min.x > p_min.z * frustum_bound_max) {
        if (p_max.x > p_max.z * frustum_bound_max) {
            return ((world_bounds_clipping){.clipped=1,.min_lerp=0.0f,.max_lerp=1.0f}); //True, 0.0, 1.0  # both above frustum
        }
        min_lerp = clip_min(p_min, p_max, frustum_bound_max);
        if (p_max.x < p_max.z * frustum_bound_min) {
            max_lerp = clip_max(p_min, p_max, frustum_bound_min);
        } else {
            max_lerp = 1.0f;
        }
    } else if (p_max.x > p_max.z * frustum_bound_max) {
        max_lerp = clip_max(p_min, p_max, frustum_bound_max);
        if (p_min.x < p_min.z * frustum_bound_min) {
            min_lerp = clip_min(p_min, p_max, frustum_bound_min);
        } else {
            min_lerp = 0.0f;
        }
    } else {
        if (p_min.x < p_min.z * frustum_bound_min) {
            if (p_max.x < p_max.z * frustum_bound_min) {
                return ((world_bounds_clipping){.clipped=1, .min_lerp=0.0f, .max_lerp=1.0f});   // both below frustum
            }
            min_lerp = clip_min(p_min, p_max, frustum_bound_min);
            max_lerp = 1.0;
        } else if (p_max.x < p_max.z * frustum_bound_min) {
            max_lerp = clip_max(p_min, p_max, frustum_bound_min);
            min_lerp = 0.0f;
        } else {
            min_lerp = 0.0f;
            max_lerp = 1.0f;
        }
    }
    return ((world_bounds_clipping){.clipped=0, .min_lerp=min_lerp, .max_lerp=max_lerp});
}


typedef struct {
    float start_v;
    float end_v;
} wall_v_coords;

wall_v_coords calc_v_coords(float world_y0, float world_y1, pegging_type peg_type, int repeat_tex) {

    float units = repeat_tex ? my_fabsf(world_y1 - world_y0): 1.0f;
    float start_v = 0.0f;
    float end_v = 0.0f;
    if(peg_type == BOTTOM_PEGGED) {
        start_v = 1.0f - units;
        end_v = 1.0f;
    } else {
        end_v = units;
    }
    return ((wall_v_coords) {
        .start_v = start_v,
        .end_v = end_v
    });
}

int frustum_cull = 0;


int point_in_north_east(float px, float pz) {
    float subz = pz - my_floorf(pz);
    float subx = px - my_floorf(px);
    return subz >= (1.0f-subx);
}

int point_in_north_west(float px, float pz) {
    float subz = pz - my_floorf(pz);
    float subx = px - my_floorf(px);
    return (subz >= subx);
}


void execute_rays_in_segment(
    u32* ray_buffer, float* z_buffer, u8* seen_pixel_cache,
    int ray_buffer_base_offset,
    segment seg,
    int start_ray, int end_ray,
    camera cam,
    mat4 world_to_screen_mat,
    int axis_mapped_to_y,
    level* this_level, int seg_buffer_height, int edit_id_render_enabled,
    int flash_frame, int editor_mode_enabled, int editor_selected_map_idx, editor_selected_thing editor_selected_side
) { 
    float world_max_y = MAX_WALL_HEIGHT;
    float one_over_world_max_y = 1.0f / world_max_y;
    float camera_pos_y_normalized = cam.pos.y * one_over_world_max_y;

    u8* cur_level_floor = this_level->floor;
    u8* cur_level_ceil = this_level->ceil;
    u8* cur_level_upper_floor = this_level->upper_floor;
    u8* cur_level_upper_ceil = this_level->upper_ceil;

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
        float* z_buf_column = &z_buffer[(ray_column_idx*seg_buffer_height)];

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

        float frustum_bounds_min = original_next_free_pix_min - 0.501f;
        float frustum_bounds_max = original_next_free_pix_max + 0.501f;

        #define FLOAT_EPS (1.1920929e-07f)
        float frustum_dir_max_world = FLOAT_EPS;
        float frustum_dir_min_world = FLOAT_EPS;

        int needs_fill = 0;

        while(1) {
            if(cur_intersection_distance >= far_clip) {
                needs_fill = 1;
                break;
            }
            if(x_steps < 0 || y_steps < 0) {
                needs_fill = 1;
                break;
            }

            int map_idx = map_z*MAP_SIZE+map_x;

            float floor_height = cur_level_floor[map_idx]/8.0f;
            float upper_floor_height = cur_level_upper_floor[map_idx]/8.0f;
            float ceil_height = cur_level_ceil[map_idx]/8.0f;
            float upper_ceil_height = cur_level_upper_ceil[map_idx]/8.0f;
            float floor_anchor = this_level->floor_anchor[map_idx]/8.0f;
            float ceil_anchor = this_level->ceil_anchor[map_idx]/8.0f;
            

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

            //int span_idx_start = top_down ? 0 : num_col_spans-1;
            //int span_idx_end = top_down ? num_col_spans : -1;
            //int iteration_direction = top_down ? 1 : -1;

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
            ray_step_t next_step = step_ray(ray, cam_pos_xz, norm_ray_direction, x_steps, y_steps, cam.far_clip);
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



            u8 lower_diag_wall_tex = this_level->ldtex[map_idx];
            u8 lower_diag_light_level = this_level->ld_light[map_idx];

            u8 floor_texture = this_level->ftex[map_idx];
            u8 floor_light_level = this_level->f_light[map_idx];

            u8 upper_floor_texture = this_level->uftex[map_idx];
            u8 upper_floor_light_level = this_level->uf_light[map_idx];

            u8 upper_diag_wall_tex = this_level->udtex[map_idx];
            u8 upper_diag_light_level = this_level->ud_light[map_idx];

            u8 ceil_texture = this_level->ctex[map_idx];
            u8 ceil_light_level = this_level->c_light[map_idx];

            u8 upper_ceil_texture = this_level->uctex[map_idx];
            u8 upper_ceil_light_level = this_level->uc_light[map_idx];

            cell_types upper_cell_type = this_level->upper_cell_types[map_idx];
            cell_types lower_cell_type = this_level->lower_cell_types[map_idx];

            int lower_step_slope =  (lower_cell_type == SLOPE_X || lower_cell_type == SLOPE_Y);
            int lower_step_diag = (lower_cell_type == NE_TO_SW_DIAG || lower_cell_type == NW_TO_SE_DIAG || lower_cell_type == THIN_WALL_X || lower_cell_type == THIN_WALL_Y);
            int upper_step_slope =  (upper_cell_type == SLOPE_X || upper_cell_type == SLOPE_Y);

            float first_floor_height = floor_height;
            float second_floor_height = upper_floor_height;
            u8 first_floor_texture = floor_texture;
            u8 second_floor_texture = upper_floor_texture;
            u8 first_floor_light_level = floor_light_level;
            u8 second_floor_light_level = upper_floor_light_level;
            editor_selected_thing first_floor_side = WALL_SIDE_TOP;
            editor_selected_thing second_floor_side = WALL_SIDE_UPPER_TOP;

            float first_ceil_height = ceil_height;
            float second_ceil_height = upper_ceil_height;
            u8 first_ceil_texture = ceil_texture;
            u8 second_ceil_texture = upper_ceil_texture;
            u8 first_ceil_light_level = ceil_light_level;
            u8 second_ceil_light_level = upper_ceil_light_level;
            editor_selected_thing first_ceil_side = WALL_SIDE_BOTTOM;
            editor_selected_thing second_ceil_side = WALL_SIDE_UPPER_BOTTOM;


            int selected_cur_map_idx = editor_selected_map_idx == map_idx;
 
            // miscellaneous stuff for diagonal draw order sorting 
            float ray_subx = ray_origin_x - my_floorf(ray_origin_x); 
            float ray_subz = ray_origin_z - my_floorf(ray_origin_z);
            float hit_subx = hit_x - my_floorf(hit_x);
            float hit_subz = hit_z - my_floorf(hit_z);
            
            // WEST->EAST
            // -x -> +x

            // NORTH->SOUTH
            // +z -> -z

            int draw_second_floor = 0;

            int enters_right_side = (step_x == -1) && (side == VERTICAL_SIDE);
            int enters_left_side = (step_x == 1) && (side == VERTICAL_SIDE);
            int enters_bot_side = (step_z == -1) && (side == HORIZONTAL_SIDE);
            int enters_top_side = (step_z == 1) && (side == HORIZONTAL_SIDE);
            int exits_right_side = (step_x == 1) && (side == VERTICAL_SIDE);
            int exits_left_side = (step_x == -1) && (side == VERTICAL_SIDE);
            int exits_bot_side = (step_z == 1) && (side == HORIZONTAL_SIDE);
            int exits_top_side = (step_z == -1) && (side == HORIZONTAL_SIDE);


            int hit_top_half = (hit_subz <= 0.5f);
            int hit_bot_half = !hit_top_half;
            int hit_right_half = (hit_subx >= 0.5f);
            int hit_left_half = !hit_right_half;

            float next_hit_subx = (next_hit_x - my_floorf(next_hit_x));
            float next_hit_subz = (next_hit_z - my_floorf(next_hit_z));
            int next_hit_right_half = (next_hit_subx >= 0.5f);
            int next_hit_left_half = !next_hit_right_half;
            int next_hit_top_half = (next_hit_subz <= 0.5f);
            int next_hit_bot_half = !next_hit_top_half;

            


            int enters_top_side_right_half = (enters_top_side && hit_right_half);
            int enters_bot_side_right_half = (enters_bot_side && hit_right_half);
            int enters_left_side_top_half = (enters_left_side && hit_top_half);
            int enters_right_side_top_half = (enters_right_side && hit_top_half);

            int exits_top_side_right_half = (exits_top_side && next_hit_right_half);
            int exits_bot_side_right_half = (exits_bot_side && next_hit_right_half);
            int exits_top_side_left_half = (exits_top_side && next_hit_left_half);
            int exits_bot_side_left_half = (exits_bot_side && next_hit_left_half);

            
            int exits_left_side_top_half = (exits_left_side && next_hit_top_half);
            int exits_right_side_top_half = (exits_right_side && next_hit_top_half);
            int exits_left_side_bot_half = (exits_left_side && next_hit_bot_half);
            int exits_right_side_bot_half = (exits_right_side && next_hit_bot_half);



            if(lower_cell_type == NE_TO_SW_DIAG || lower_cell_type ==  NW_TO_SE_DIAG || 
                lower_cell_type == THIN_WALL_X || lower_cell_type == THIN_WALL_Y) { //} || lower_cell_type == DOOR_Y) {
                    
                int draw_upper_first = 0;
                if(lower_cell_type == NE_TO_SW_DIAG) {
                    draw_upper_first = (enters_left_side || enters_top_side);
                    
                    if(draw_upper_first) {
                        draw_second_floor = (exits_right_side || exits_bot_side);
                    } else {
                        draw_second_floor = (exits_left_side || exits_top_side);
                    }

                } else if (lower_cell_type == NW_TO_SE_DIAG) {
                    // this seems to work perfectly
                    // for whatever reason, this had to be flipped from NE enter..
                    draw_upper_first = (enters_right_side || enters_top_side);
                    if(draw_upper_first) {
                        draw_second_floor = (exits_left_side || exits_bot_side);
                    } else {
                        draw_second_floor = (exits_right_side || exits_top_side);
                    }

                } else if (lower_cell_type == THIN_WALL_X) {
                    draw_upper_first = (enters_right_side || enters_top_side_right_half || enters_bot_side_right_half);
                    if(draw_upper_first) {
                        draw_second_floor = (exits_left_side || exits_bot_side_left_half || exits_top_side_left_half);
                    } else {
                        draw_second_floor = (exits_right_side || exits_top_side_right_half || exits_bot_side_right_half);
                    }
                } else if (lower_cell_type == THIN_WALL_Y) {
                    draw_upper_first = (enters_top_side || enters_left_side_top_half || enters_right_side_top_half);
                    if(draw_upper_first) {
                        draw_second_floor = (exits_bot_side || exits_left_side_bot_half || exits_right_side_bot_half);
                    } else {
                        draw_second_floor = (exits_top_side || exits_left_side_top_half || exits_right_side_top_half);
                    }
                }

                if(draw_upper_first) {
                    first_floor_height = upper_floor_height;
                    second_floor_height = floor_height;
                    first_floor_texture = upper_floor_texture;
                    second_floor_texture = floor_texture;
                    first_floor_light_level = upper_floor_light_level;
                    second_floor_light_level = floor_light_level;
                    first_floor_side = WALL_SIDE_UPPER_TOP;
                    second_floor_side = WALL_SIDE_TOP;
                }
            } else if (lower_step_slope) {
                first_floor_texture = upper_floor_texture;
            }

            if(upper_cell_type == NE_TO_SW_DIAG || upper_cell_type ==  NW_TO_SE_DIAG || 
                upper_cell_type == THIN_WALL_X || upper_cell_type == THIN_WALL_Y) {
                // handle diagonal stuff               
                int draw_upper_first = 0;
                if(lower_cell_type == NE_TO_SW_DIAG) {
                    draw_upper_first = (enters_left_side || enters_top_side);
                    
                    if(draw_upper_first) {
                        draw_second_floor = (exits_right_side || exits_bot_side);
                    } else {
                        draw_second_floor = (exits_left_side || exits_top_side);
                    }

                } else if (lower_cell_type == NW_TO_SE_DIAG) {
                    // this seems to work perfectly
                    // for whatever reason, this had to be flipped from NE enter..
                    draw_upper_first = (enters_right_side || enters_top_side);
                    if(draw_upper_first) {
                        draw_second_floor = (exits_left_side || exits_bot_side);
                    } else {
                        draw_second_floor = (exits_right_side || exits_top_side);
                    }

                } else if (lower_cell_type == THIN_WALL_X) {
                    draw_upper_first = (enters_right_side || enters_top_side_right_half || enters_bot_side_right_half);
                    if(draw_upper_first) {
                        draw_second_floor = (exits_left_side || exits_bot_side_left_half || exits_top_side_left_half);
                    } else {
                        draw_second_floor = (exits_right_side || exits_top_side_right_half || exits_bot_side_right_half);
                    }
                } else if (lower_cell_type == THIN_WALL_Y) {
                    draw_upper_first = (enters_top_side || enters_left_side_top_half || enters_right_side_top_half);
                    if(draw_upper_first) {
                        draw_second_floor = (exits_bot_side || exits_left_side_bot_half || exits_right_side_bot_half);
                    } else {
                        draw_second_floor = (exits_top_side || exits_left_side_top_half || exits_right_side_top_half);
                    }
                }


                if(draw_upper_first) {
                    first_ceil_height = upper_ceil_height;
                    second_ceil_height = ceil_height;
                    first_ceil_texture = upper_ceil_texture;
                    second_ceil_texture = ceil_texture;
                    first_ceil_light_level = upper_ceil_light_level;
                    second_ceil_light_level = ceil_light_level;
                    first_ceil_side = WALL_SIDE_UPPER_BOTTOM;
                    second_ceil_side = WALL_SIDE_BOTTOM;
                }
            } else if (upper_step_slope) {
                first_ceil_texture = upper_ceil_texture;
            }
            
            slope_heights floor_slope = get_slope_heights_6dof(in_start_cell, map_x, map_z, next_map_x, next_map_z,
                hit_x, hit_z, next_hit_x, next_hit_z, side, lower_cell_type, step_x, step_z,
                ray_origin_x, ray_origin_z, first_floor_height, second_floor_height
            );
            slope_heights ceil_slope = get_slope_heights_6dof(in_start_cell, map_x, map_z, next_map_x, next_map_z,
                hit_x, hit_z, next_hit_x, next_hit_z, side, upper_cell_type, step_x, step_z,
                ray_origin_x, ray_origin_z, first_ceil_height, second_ceil_height
            );

            if(lower_step_slope) {
                first_floor_height = floor_slope.start_height;
            }
            if(upper_step_slope) {
                first_ceil_height = ceil_slope.start_height;
            }




            // draw front wall for floor
            if(!in_start_cell && floor_anchor != first_floor_height) {
                //float bot_u = (element_bounds_max-element_bounds_min)/1.0f;
                float world_y0 = first_floor_height;
                float world_y1 = floor_anchor;
                float portion_top = world_y0 * one_over_world_max_y;
                float portion_bot = world_y1 * one_over_world_max_y;
                //int R (world_y1-world_y0)/8.0f; // texel repetition


                float3 cam_space_front_top = float3_lerp(cam_space_min_last, cam_space_max_last, world_y0*one_over_world_max_y);
                float3 cam_space_front_bot = float3_lerp(cam_space_min_last, cam_space_max_last, world_y1*one_over_world_max_y);
                float xt = cam_space_front_top.x;
                float yt = cam_space_front_top.y;
                float zt = cam_space_front_top.z;

                float xb = cam_space_front_bot.x;
                float yb = cam_space_front_bot.y;
                float zb = cam_space_front_bot.z;

                wall_v_coords wall_tex_coords = calc_v_coords(world_y0, world_y1, BOTTOM_PEGGED, REPEAT_TEX);
                float5 top = mk_float5(xt,yt,zt,wall_u, wall_tex_coords.start_v);
                float5 bot = mk_float5(xb,yb,zb,wall_u, wall_tex_coords.end_v);
                clip_res wall_clipped = clip_homogeneous_camera_space_line(
                    top, bot
                );
                
                
                if(wall_clipped.on_screen) {

                    new_screen_bounds bnds = fill_raybuffer_column(
                        wall_clipped.top, wall_clipped.bot,
                        cur_next_free_pix_min, cur_next_free_pix_max,
                        original_next_free_pix_min, original_next_free_pix_max,
                        seen_pixel_cache, ray_column, z_buf_column, seg_buffer_height, 
                        textures[lower_wall_tex],
                        edit_id_render_enabled || (editor_mode_enabled && flash_frame && selected_cur_map_idx && editor_selected_side == lower_intersect_wall_side),
                        MAP_CELL_EDIT_ID(map_idx, lower_intersect_wall_side) 
                    );

                    cur_next_free_pix_min = bnds.top;
                    cur_next_free_pix_max = bnds.bot;

                    if(cur_next_free_pix_min > cur_next_free_pix_max) {
                        break_ray_loop = 1;
                        break;
                    }
                }
            }


            // draw front wall for ceiling
            if(!in_start_cell && ceil_anchor != first_ceil_height) {
                float world_y0 = ceil_anchor;
                float world_y1 = first_ceil_height;
                float portion_top = world_y0 * one_over_world_max_y;
                float portion_bot = world_y1 * one_over_world_max_y;


                float3 cam_space_front_top = float3_lerp(cam_space_min_last, cam_space_max_last, portion_top);
                float3 cam_space_front_bot = float3_lerp(cam_space_min_last, cam_space_max_last, portion_bot);
                float xt = cam_space_front_top.x;
                float yt = cam_space_front_top.y;
                float zt = cam_space_front_top.z;

                float xb = cam_space_front_bot.x;
                float yb = cam_space_front_bot.y;
                float zb = cam_space_front_bot.z;

                wall_v_coords wall_tex_coords = calc_v_coords(world_y0, world_y1, TOP_PEGGED, REPEAT_TEX);
                float5 top = mk_float5(xt,yt,zt,wall_u, wall_tex_coords.start_v);
                float5 bot = mk_float5(xb,yb,zb,wall_u, wall_tex_coords.end_v);
                clip_res wall_clipped = clip_homogeneous_camera_space_line(
                    top, bot
                );
                
                
                if(wall_clipped.on_screen) {

                    new_screen_bounds bnds = fill_raybuffer_column(
                        wall_clipped.top, wall_clipped.bot,
                        cur_next_free_pix_min, cur_next_free_pix_max,
                        original_next_free_pix_min, original_next_free_pix_max,
                        seen_pixel_cache, ray_column, z_buf_column, seg_buffer_height, 
                        textures[lower_wall_tex],
                        edit_id_render_enabled || (editor_mode_enabled && flash_frame && selected_cur_map_idx && editor_selected_side == upper_intersect_wall_side),
                        MAP_CELL_EDIT_ID(map_idx, upper_intersect_wall_side)
                    );

                    cur_next_free_pix_min = bnds.top;
                    cur_next_free_pix_max = bnds.bot;

                    if(cur_next_free_pix_min > cur_next_free_pix_max) {
                        break_ray_loop = 1;
                        break;
                    }
                }
            }


            int lower_hits_diag = 0;
            diag_intersect lower_diag_intersect;
            float3 plane_ray_dir_times_diag_dist;
            float3 cam_space_min_diag, cam_space_max_diag;
            
            // draw floor
            if(lower_cell_type == NORMAL_CELL || lower_step_slope || lower_step_diag) {

                // TODO handle diags, doors, half walls
                float floor_start_height = first_floor_height;
                float floor_end_height = first_floor_height;
                if(lower_step_slope) {
                    floor_end_height = floor_slope.end_height;
                }

                float portion_exit = floor_end_height * one_over_world_max_y;
                float portion_enter = floor_start_height * one_over_world_max_y;

                clip_res floor_clipped;

                float3 flat_enter_cam_space = float3_lerp(cam_space_min_last, cam_space_max_last, portion_enter);
                float3 flat_exit_cam_space;
                if(lower_step_diag) {
                    lower_hits_diag = calc_diag_hit(&lower_diag_intersect, ray_dir_x, ray_dir_z, ray_origin_x, ray_origin_z, map_x, map_z, lower_cell_type);
                    float exit_u = exit_flat_u;
                    float exit_v = exit_flat_v;
                    if(lower_hits_diag) { 
                        plane_ray_dir_times_diag_dist = float3_mul_float(plane_dir, lower_diag_intersect.diag_perp_dist);
                        cam_space_min_diag = float3_add_float3(plane_bot, plane_ray_dir_times_diag_dist);
                        cam_space_max_diag = float3_add_float3(plane_top, plane_ray_dir_times_diag_dist);
                        flat_exit_cam_space = float3_lerp(cam_space_min_diag, cam_space_max_diag, portion_exit);
                        exit_u = lower_diag_intersect.mid_flat_u;
                        exit_v = lower_diag_intersect.mid_flat_v;
                    } else {
                        flat_exit_cam_space = float3_lerp(cam_space_min_next, cam_space_max_next, portion_exit);
                    }
                    
                    float ax = flat_exit_cam_space.x;
                    float ay = flat_exit_cam_space.y;
                    float az = flat_exit_cam_space.z;

                    float bx = flat_enter_cam_space.x;
                    float by = flat_enter_cam_space.y;
                    float bz = flat_enter_cam_space.z;

                    floor_clipped = clip_homogeneous_camera_space_line(
                        mk_float5(ax,ay,az, exit_u, exit_v),
                        mk_float5(bx,by,bz, flat_u, flat_v)
                    );
                } else {
                    flat_exit_cam_space = float3_lerp(cam_space_min_next, cam_space_max_next, portion_exit);
                    float ax = flat_exit_cam_space.x;
                    float ay = flat_exit_cam_space.y;
                    float az = flat_exit_cam_space.z;

                    float bx = flat_enter_cam_space.x;
                    float by = flat_enter_cam_space.y;
                    float bz = flat_enter_cam_space.z;

                    floor_clipped = clip_homogeneous_camera_space_line(
                        mk_float5(ax,ay,az, exit_flat_u, exit_flat_v),
                        mk_float5(bx,by,bz, flat_u, flat_v)
                    );
                }
                
                          
                if(floor_clipped.on_screen) {
                    u32* floor_tex = textures[first_floor_texture]; 
                    new_screen_bounds bnds =  fill_raybuffer_column(
                        floor_clipped.top, floor_clipped.bot,
                        cur_next_free_pix_min, cur_next_free_pix_max,
                        original_next_free_pix_min, original_next_free_pix_max,
                        seen_pixel_cache, ray_column, z_buf_column, seg_buffer_height, 
                        floor_tex, 
                        edit_id_render_enabled || (editor_mode_enabled && flash_frame && selected_cur_map_idx && editor_selected_side == first_floor_side),
                        MAP_CELL_EDIT_ID(map_idx, first_floor_side)
                    );

                    cur_next_free_pix_min = bnds.top;
                    cur_next_free_pix_max = bnds.bot;

                    if(cur_next_free_pix_min > cur_next_free_pix_max) {
                        break_ray_loop = 1;
                        break;
                    }
                }
            }

            if(lower_hits_diag) {
                // draw diag walls
                

                float portion_top = second_floor_height * one_over_world_max_y;
                float portion_bot = first_floor_height * one_over_world_max_y;
                
                float3 top_diag_wall = float3_lerp(cam_space_min_diag, cam_space_max_diag, portion_top);
                float3 bot_diag_wall = float3_lerp(cam_space_min_diag, cam_space_max_diag, portion_bot);

                float xt = top_diag_wall.x;
                float yt = top_diag_wall.y;
                float zt = top_diag_wall.z;

                float xb = bot_diag_wall.x;
                float yb = bot_diag_wall.y;
                float zb = bot_diag_wall.z;
                

                wall_v_coords wall_tex_coords = calc_v_coords(second_floor_height, first_floor_height, BOTTOM_PEGGED, REPEAT_TEX);
                float5 top = mk_float5(xt,yt,zt,lower_diag_intersect.diag_wall_u, wall_tex_coords.start_v);
                float5 bot = mk_float5(xb,yb,zb,lower_diag_intersect.diag_wall_u, wall_tex_coords.end_v);

                clip_res diag_wall_clipped = clip_homogeneous_camera_space_line(
                    top, bot
                );

                if(diag_wall_clipped.on_screen) {
                    
                    new_screen_bounds bnds = fill_raybuffer_column(
                        diag_wall_clipped.top, diag_wall_clipped.bot,
                        cur_next_free_pix_min, cur_next_free_pix_max,
                        original_next_free_pix_min, original_next_free_pix_max,
                        seen_pixel_cache, ray_column, z_buf_column, seg_buffer_height, 
                        textures[lower_diag_wall_tex], 
                        edit_id_render_enabled || (editor_mode_enabled && flash_frame && selected_cur_map_idx && editor_selected_side == WALL_SIDE_LOWER_DIAG),
                        MAP_CELL_EDIT_ID(map_idx, WALL_SIDE_LOWER_DIAG)
                    );

                    cur_next_free_pix_min = bnds.top;
                    cur_next_free_pix_max = bnds.bot;

                    if(cur_next_free_pix_min > cur_next_free_pix_max) {
                        break_ray_loop = 1;
                        break;
                    }

                }

                // draw upper floor
                if(draw_second_floor) {
                    float exit_u = exit_flat_u;
                    float exit_v = exit_flat_v;
                    float enter_u = lower_diag_intersect.mid_flat_u;
                    float enter_v = lower_diag_intersect.mid_flat_v;
                    float3 flat_enter_cam_space = float3_lerp(cam_space_min_diag, cam_space_max_diag, portion_top);
                    float3 flat_exit_cam_space = float3_lerp(cam_space_min_next, cam_space_max_next, portion_top);

                    float ax = flat_exit_cam_space.x;
                    float ay = flat_exit_cam_space.y;
                    float az = flat_exit_cam_space.z;

                    float bx = flat_enter_cam_space.x;
                    float by = flat_enter_cam_space.y;
                    float bz = flat_enter_cam_space.z;

                    clip_res upper_floor_clipped = clip_homogeneous_camera_space_line(
                        mk_float5(ax,ay,az, exit_u, exit_v),
                        mk_float5(bx,by,bz, enter_u, enter_v)
                    );

                    if(upper_floor_clipped.on_screen) {
                        
                        new_screen_bounds bnds = fill_raybuffer_column(
                            upper_floor_clipped.top, upper_floor_clipped.bot,
                            cur_next_free_pix_min, cur_next_free_pix_max,
                            original_next_free_pix_min, original_next_free_pix_max,
                            seen_pixel_cache, ray_column, z_buf_column, seg_buffer_height, 
                            textures[second_floor_texture], 
                            edit_id_render_enabled || (editor_mode_enabled && flash_frame && selected_cur_map_idx && editor_selected_side == second_floor_side),
                            MAP_CELL_EDIT_ID(map_idx, second_floor_side)
                        );

                        cur_next_free_pix_min = bnds.top;
                        cur_next_free_pix_max = bnds.bot;

                        if(cur_next_free_pix_min > cur_next_free_pix_max) {
                            break_ray_loop = 1;
                            break;
                        }
                    }
                }
            }

            // draw ceiling
            if (upper_cell_type == NORMAL_CELL || upper_step_slope) {
                // TODO: handle doors, diags, half walls
                float ceil_start_height = first_ceil_height;
                float ceil_end_height = first_ceil_height;
                if(upper_step_slope) {
                    ceil_end_height = ceil_slope.end_height;
                }
                
                float portion_exit = ceil_end_height * one_over_world_max_y;
                float portion_enter = ceil_start_height * one_over_world_max_y;


                float3 flat_enter_cam_space = float3_lerp(cam_space_min_last, cam_space_max_last, portion_enter);
                float3 flat_exit_cam_space = float3_lerp(cam_space_min_next, cam_space_max_next, portion_exit);
            
                // for ceilings, the 'enter' is above in screen space
                // so it's swapped vs floors
                float ax = flat_enter_cam_space.x;
                float ay = flat_enter_cam_space.y;
                float az = flat_enter_cam_space.z;

                float bx = flat_exit_cam_space.x;
                float by = flat_exit_cam_space.y;
                float bz = flat_exit_cam_space.z;

                // for ceilings, we f
                clip_res ceil_clipped = clip_homogeneous_camera_space_line(
                    mk_float5(ax,ay,az, flat_u, flat_v),
                    mk_float5(bx,by,bz, exit_flat_u, exit_flat_v)
                );

                if(ceil_clipped.on_screen) {
                    u32* ceil_tex = upper_step_slope ? textures[this_level->uctex[map_idx]] : textures[this_level->ctex[map_idx]];
                    new_screen_bounds bnds = fill_raybuffer_column(
                        ceil_clipped.top, ceil_clipped.bot,
                        cur_next_free_pix_min, cur_next_free_pix_max,
                        original_next_free_pix_min, original_next_free_pix_max,
                        seen_pixel_cache, ray_column, z_buf_column, seg_buffer_height, 
                        ceil_tex, 
                        edit_id_render_enabled || (editor_mode_enabled && flash_frame && selected_cur_map_idx && editor_selected_side == first_ceil_side),
                        MAP_CELL_EDIT_ID(map_idx, first_ceil_side)
                    );

                    if(ceil_tex != textures[SKYBOX_TEX_IDX]) {
                        cur_next_free_pix_min = bnds.top;
                        cur_next_free_pix_max = bnds.bot;
                    }
                    if(cur_next_free_pix_min > cur_next_free_pix_max) {
                        break_ray_loop = 1;
                        break;
                    }
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
            if(next_step.past_far_clip) {
                needs_fill = 1;
                break;
            }
        }
        
        if(needs_fill) {
            needs_fill--;
            for(int y = cur_next_free_pix_min; y <= cur_next_free_pix_max; y++) {
                if(is_pixel_set(seen_pixel_cache, y) == 0) {
                    ray_column[(seg_buffer_height-1-y)] = 0xFFB7CEFA;
                    z_buf_column[(seg_buffer_height-1-y)] = 1.0f;
                }
            }
            //printf("whoa!\n");
        }
        
    }
}


float3 adjust_screen_pixel_for_mesh(float2 screen_pixel, float2 screen_size) { 
        return (float3){.x=2.0f * screen_pixel.x / screen_size.x - 1.0f, .y = 2.0f * screen_pixel.y/screen_size.y - 1.0f, .z=0.5f};
}


#define MIN_RAYS_PER_THREAD 16

typedef struct {
    u32* ray_buffer; float* z_buffer; u8* seen_pixel_cache;
    int ray_buffer_base_offset;
    segment seg;
    int start_ray; int end_ray;
    camera cam;
    mat4 world_to_screen_mat;
    int axis_mapped_to_y;
    level* this_level;
    int seg_buffer_height;
    int edit_id_render_enabled;
    int flash_frame;
    int editor_selected_map_idx;
    editor_selected_thing editor_selected_side;
    int editor_mode_enabled;
} thread_params;


#include <stdio.h>
s64 segment_thread_counters[4];

void execute_rays_in_segment_wrapper(void* p) {
    thread_params* tp = (thread_params*)p;
    execute_rays_in_segment(
        tp->ray_buffer, tp->z_buffer, tp->seen_pixel_cache, tp->ray_buffer_base_offset, tp->seg, 
        tp->start_ray, tp->end_ray, tp->cam, tp->world_to_screen_mat,
        tp->axis_mapped_to_y, tp->this_level, tp->seg_buffer_height, tp->edit_id_render_enabled,
        tp->flash_frame, tp->editor_mode_enabled, tp->editor_selected_map_idx, tp->editor_selected_side
    );
    // decrement counter
    //printf("woo, seg %i decrementing :)\n", tp->seg.index);
    InterlockedDecrement64(&segment_thread_counters[tp->seg.index]);
}


typedef struct {
    u32* ray_buffers[2];
    float* z_buffers[2];
    segment segments[4];
    camera cam;
    mat4 world_to_screen_mat;
    level* this_level;
    int seg_buffer_heights[2];
    int edit_id_render_enabled;
    int flash_frame;
    int editor_selected_map_idx;
    int editor_mode_enabled;
    editor_selected_thing editor_selected_side;
} frame_params;

jobpool* raycast_6dof_pool = NULL;
static jobpool* raycast_6dof_manager_pool = NULL;
static thread_params raycast_6dof_parms[NUM_THREADS*4];
static frame_params full_frame_params;

int next_task_idx = 0;

s64 segment_raycast_finished[4];
void parallel_raycast_segment_wrapper(
    void* arg_var
    ) {
    frame_params* tp = (frame_params*)arg_var;

    camera cam = tp->cam;
    mat4 world_to_screen_mat = tp->world_to_screen_mat;
    int *seg_buffer_heights = tp->seg_buffer_heights;
    level* this_level = tp->this_level;

    for(int seg_idx = 0; seg_idx < 4; seg_idx++) {
        segment seg = tp->segments[seg_idx];
        if(seg.ray_count == 0) {
            //printf("segment %i has no rays\n", seg_idx);
            segment_thread_counters[seg_idx] = 0;
            continue;
        }
        int ray_buffer_base_offset = (seg_idx&1) ? (tp->segments[seg_idx-1].ray_count) : 0;
        u32* ray_buffer = tp->ray_buffers[(seg_idx>>1)];
        float* z_buffer = tp->z_buffers[(seg_idx>>1)];

        int axis_mapped_to_y = (seg_idx > 1) ? 0 : 1;
        int seg_buffer_height = seg_buffer_heights[(seg_idx>>1)];
        int edit_id_render_enabled = tp->edit_id_render_enabled;
        int flash_frame = tp->flash_frame;
        int editor_mode_enabled = tp->editor_mode_enabled;
        int editor_selected_map_idx = tp->editor_selected_map_idx;
        editor_selected_thing editor_selected_side = tp->editor_selected_side;


        int used_threads = NUM_THREADS;
        int rays_per_thread = seg.ray_count / used_threads;
        //if(rays_per_thread < MIN_RAYS_PER_THREAD) {
        //    used_threads = seg.ray_count / MIN_RAYS_PER_THREAD;
        //    rays_per_thread = seg.ray_count / used_threads;
        //}
        segment_thread_counters[seg_idx] = used_threads;


        for(int i = 0; i < used_threads; i++) {
            raycast_6dof_parms[next_task_idx].ray_buffer = ray_buffer;
            raycast_6dof_parms[next_task_idx].z_buffer = z_buffer;
            raycast_6dof_parms[next_task_idx].seen_pixel_cache = per_thread_seen_pixel_cache[next_task_idx];
            raycast_6dof_parms[next_task_idx].ray_buffer_base_offset = ray_buffer_base_offset;
            raycast_6dof_parms[next_task_idx].seg = seg;
            raycast_6dof_parms[next_task_idx].cam = cam;
            raycast_6dof_parms[next_task_idx].world_to_screen_mat = world_to_screen_mat;
            raycast_6dof_parms[next_task_idx].axis_mapped_to_y = axis_mapped_to_y;
            raycast_6dof_parms[next_task_idx].this_level = this_level;
            raycast_6dof_parms[next_task_idx].seg_buffer_height = seg_buffer_height;
            raycast_6dof_parms[next_task_idx].edit_id_render_enabled = edit_id_render_enabled;
            raycast_6dof_parms[next_task_idx].flash_frame = flash_frame;
            raycast_6dof_parms[next_task_idx].editor_selected_map_idx = editor_selected_map_idx;
            raycast_6dof_parms[next_task_idx].editor_selected_side = editor_selected_side;
            raycast_6dof_parms[next_task_idx].editor_mode_enabled = editor_mode_enabled;



            raycast_6dof_parms[next_task_idx].start_ray = (rays_per_thread*i);
            raycast_6dof_parms[next_task_idx].end_ray = (i == used_threads-1) ? seg.ray_count : MIN(seg.ray_count, (rays_per_thread*(i+1)));

            //printf("seg %i, %i/%i, add task %i\n", seg_idx, i, NUM_THREADS, next_task_idx);
            platform_add_task(
                raycast_6dof_pool,
                execute_rays_in_segment_wrapper,
                &raycast_6dof_parms[next_task_idx]
            );
            next_task_idx++;
        }

    }

    int segs_complete = 0;
    //int cnt = 0;
    while(segs_complete != 4) {
        for(int seg_idx = 0; seg_idx < 4; seg_idx++) {
            if(segment_thread_counters[seg_idx] == 0) {
                //printf("orchestrator: segment %i is done\n", seg_idx);
                InterlockedAdd64(&segment_raycast_finished[seg_idx], 1);
                segs_complete++;
                segment_thread_counters[seg_idx] = -1;
            }
        }
    }
    //printf("join raycast pool\n");
    platform_join_threadpool(raycast_6dof_pool); // let the raycast tasks finish up
    //printf("join done\n");
    next_task_idx = 0;
}


typedef struct {
    int seg_idx; int ray_in_seg_start; int ray_in_seg_end;
} raycast_task;

typedef struct {
    int num_tasks;
    raycast_task raycast_task[4];
} thread_task_bundle;



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
) {
    full_frame_params.ray_buffers[0] = ray_buffers[0];
    full_frame_params.ray_buffers[1] = ray_buffers[1];
    full_frame_params.z_buffers[0] = z_buffers[0];
    full_frame_params.z_buffers[1] = z_buffers[1];

    full_frame_params.segments[0] = segs[0];
    full_frame_params.segments[1] = segs[1];
    full_frame_params.segments[2] = segs[2];
    full_frame_params.segments[3] = segs[3];

    full_frame_params.cam = cam;
    full_frame_params.world_to_screen_mat = world_to_screen_mat;
    full_frame_params.this_level = this_level;
    full_frame_params.seg_buffer_heights[0] = seg_buffer_heights[0];
    full_frame_params.seg_buffer_heights[1] = seg_buffer_heights[1];
    full_frame_params.edit_id_render_enabled = edit_id_render_enabled;
    full_frame_params.flash_frame = flash_frame;
    full_frame_params.editor_mode_enabled = editor_mode_enabled;
    full_frame_params.editor_selected_map_idx = editor_selected_map_idx;
    full_frame_params.editor_selected_side = editor_selected_side;
    platform_add_task(
        raycast_6dof_manager_pool,
        parallel_raycast_segment_wrapper,
        &full_frame_params
    );
}


void join_6dof_raycast() {
    //printf("join manager pool\n");
    platform_join_threadpool(raycast_6dof_manager_pool);
    //printf("done\n");
}

void init_6dof_module() {
    raycast_6dof_pool = platform_init_threadpool(NUM_THREADS);
    raycast_6dof_manager_pool = platform_init_threadpool(1);
}



void initialize_segments(float2 screen_vp, camera cam, segment segments[4]) {
    
    int max_top_down_rays  = RENDER_WIDTH + 2*RENDER_HEIGHT;
    int max_left_right_rays = 2*RENDER_WIDTH + RENDER_HEIGHT;
    
    if (screen_vp.y < cam.dims.y) {
        // seg0 is top segment
        segments[0] = get_segment_parameters(
            0,
            cam, screen_vp, cam.dims.y - screen_vp.y, 
            ((float2){.x=0.0f, .y = 1.0f}), 1, MAX_WALL_HEIGHT, max_top_down_rays);
    }

    if(screen_vp.y > 0) {
        // seg1 is bottom
        segments[1] = get_segment_parameters(
            1,
            cam, screen_vp, screen_vp.y,
            ((float2){.x=0.0f, .y=-1.0f}), 1, MAX_WALL_HEIGHT, max_top_down_rays
        );
    }

    if(screen_vp.x < cam.dims.x) {
        // seg2 is right
        segments[2] = get_segment_parameters(
            2,
            cam, screen_vp,  cam.dims.x - screen_vp.x, 
            ((float2){.x=1.0f, .y=0.0f}), 0, MAX_WALL_HEIGHT, max_left_right_rays);
    }

    if (screen_vp.x > 0) {
        // seg3 is left
        segments[3] = get_segment_parameters(
            3,
            cam, screen_vp, screen_vp.x, 
            ((float2){.x=-1.0f, .y=0.0f}), 0, MAX_WALL_HEIGHT, max_left_right_rays);
    }
}



void draw_segments(
    float2 screen_vp, 
    camera cam, 
    segment segments[4], 
    unsigned int seg_tex_handles[2]) {

        
    int max_top_down_rays  = RENDER_WIDTH + 2*RENDER_HEIGHT;
    int max_left_right_rays = 2*RENDER_WIDTH + RENDER_HEIGHT;
    
    float3 seg_v0 = adjust_screen_pixel_for_mesh(screen_vp, cam.dims);

    float scales[4] = {
        ((float)segments[0].ray_count) / max_top_down_rays,
        ((float)segments[1].ray_count) / max_top_down_rays,
        ((float)segments[2].ray_count) / max_left_right_rays,
        ((float)segments[3].ray_count) / max_left_right_rays
    };
    float offsets[4] = {
        0.0f, scales[0], 0.0f, scales[2],
    };
    float3 seg0_v1 = adjust_screen_pixel_for_mesh(segments[0].max_screen, cam.dims);
    float3 seg0_v2 = adjust_screen_pixel_for_mesh(segments[0].min_screen, cam.dims);

    float3 seg1_v1 = adjust_screen_pixel_for_mesh(segments[1].max_screen, cam.dims);
    float3 seg1_v2 = adjust_screen_pixel_for_mesh(segments[1].min_screen, cam.dims);

    // seg2_v1 should match up with seg0_v1
    float3 seg2_v1 = adjust_screen_pixel_for_mesh(segments[2].max_screen, cam.dims);
    // seg2_v2 should match up with seg1_v1
    float3 seg2_v2 = adjust_screen_pixel_for_mesh(segments[2].min_screen, cam.dims);

    // seg3_v1 should match up with seg0_v2
    float3 seg3_v1 = adjust_screen_pixel_for_mesh(segments[3].max_screen, cam.dims);
    // seg3_v2 should match up with seg1_v2
    float3 seg3_v2 = adjust_screen_pixel_for_mesh(segments[3].min_screen, cam.dims);


    float seg3_v1_slope = (segments[3].max_screen.y/segments[3].max_screen.x);
    float seg0_v2_slope = (segments[0].min_screen.y/segments[0].min_screen.x);



    float seg01_attributes[7*6] = {
        seg_v0.x, seg_v0.y, seg_v0.z, 0.0f, 0.0f, 1.0f, 0,
        seg0_v1.x, seg0_v1.y, seg0_v1.z, 1.0f, 0.0f, 0.0f, 0,
        seg0_v2.x, seg0_v2.y, seg0_v2.z, 0.0f, 1.0f, 0.0f, 0,

        seg_v0.x, seg_v0.y, seg_v0.z, 0.0f, 0.0f, 1.0f, 1,
        seg1_v1.x, seg1_v1.y, seg1_v1.z, 1.0f, 0.0f, 0.0f, 1,
        seg1_v2.x, seg1_v2.y, seg1_v2.z, 0.0f, 1.0f, 0.0f, 1,
    };    
    float seg23_attributes[7*6] = {
        seg_v0.x, seg_v0.y, seg_v0.z, 0.0f, 0.0f, 1.0f, 2,
        seg2_v1.x, seg2_v1.y, seg2_v1.z, 1.0f, 0.0f, 0.0f, 2,
        seg2_v2.x, seg2_v2.y, seg2_v2.z, 0.0f, 1.0f, 0.0f, 2,

        seg_v0.x, seg_v0.y, seg_v0.z, 0.0f, 0.0f, 1.0f, 3,
        seg3_v1.x, seg3_v1.y, seg3_v1.z, 1.0f, 0.0f, 0.0f, 3,
        seg3_v2.x, seg3_v2.y, seg3_v2.z, 0.0f, 1.0f, 0.0f, 3,
    };
    
    //platform_draw_texture(
    //    seg_tex_handles[0]
    //);

    platform_draw_segments(
        2,
        seg_tex_handles[0], 
        0,  // good enough for seg01 :)
        seg01_attributes,
        offsets, scales
    );
    platform_draw_segments(
        2,
        seg_tex_handles[1],
        2, // good enough for seg23 :) 
        seg23_attributes,
        offsets, scales
    );
}

void transpose_and_upload_seg(u32* transpose_buf, int transpose_seg_idx, segment segments[4], u32* seg_src_buf, int seg_upload_handle) {
    segment seg = segments[transpose_seg_idx];
    int src_height = (transpose_seg_idx < 2) ? RENDER_HEIGHT : RENDER_WIDTH;
    int seg_height = ((seg.next_free_pixel_max+1) - seg.next_free_pixel_min);
    int seg_width = seg.ray_count;
    int y_offset = src_height-1 - seg.next_free_pixel_max;


    int x_offset = (transpose_seg_idx & 1) ? (segments[transpose_seg_idx-1].ray_count) : 0;

    int x1 = x_offset; int y1 = y_offset;
    int w = seg_width; int h = seg_height;

    for(int x = 0; x < w; x++) {
        for(int y = 0; y < h; y++) {
            u32 rgba = seg_src_buf[(x1 + x) * src_height + (y + y1)];
            transpose_buf[x*seg_height+y] = rgba;
        }
    }
    platform_update_texture(seg_upload_handle, transpose_buf, y1, x1, h, w);
}

void transpose_and_upload_z_buf(u32* transpose_buf, int transpose_seg_idx, segment segments[4], float* seg_src_buf, int seg_upload_handle) {
    segment seg = segments[transpose_seg_idx];
    int src_height = (transpose_seg_idx < 2) ? RENDER_HEIGHT : RENDER_WIDTH;
    int seg_height = ((seg.next_free_pixel_max+1) - seg.next_free_pixel_min);
    int seg_width = seg.ray_count;
    int y_offset = src_height-1 - seg.next_free_pixel_max;


    int x_offset = (transpose_seg_idx & 1) ? (segments[transpose_seg_idx-1].ray_count) : 0;

    int x1 = x_offset; int y1 = y_offset;
    int w = seg_width; int h = seg_height;
    //memset(transpose_buf, 0xFF, (w*h*4));
    for(int x = 0; x < w; x++) {
        for(int y = 0; y < h; y++) {
            float z = seg_src_buf[(x1 + x) * src_height + (y + y1)];
            int gray_z = z*8.0;
            u32 rgba = (0xFF<<24)|(gray_z<<16)|(gray_z<<8)|(gray_z<<0);
            transpose_buf[x*seg_height+y] = rgba;
        }
    }
    platform_update_texture(seg_upload_handle, transpose_buf, y1, x1, h, w);
}

