#include <stdio.h>
#include "6dof.h"
#include "common.h"
#include "my_defs.h"
#include "raycast.h"

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

float2 float2_mul_float(float2 a, float b) {
    return mk_float2((a.x*b), (a.y*b));
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
    float h = 2.0f * z_near / height;

    return mk_mat4(w,    0.0f, 0.0f, 0.0f,
                   0.0f, h,    0.0f, 0.0f,
                   0.0f, 0.0f, q,    -1.0f,
                   0.0f, 0.0f, qn,   0.0f);
}

mat4 get_world_to_camera_matrix(camera cam) {
    return mk_mat4(
        cam.right.vals[0], cam.up.vals[0], -cam.forward.vals[0], 0.0f,
        cam.right.vals[1], cam.up.vals[1], -cam.forward.vals[1], 0.0f,
        cam.right.vals[2], cam.up.vals[2], -cam.forward.vals[2], 0.0f,
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

    mat4 scale_mat = mk_mat4_from_scale(mk_float3(1.0f, 1.0f, 1.0f)); // swapped -1 z for 1
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
 
camera mk_camera(
    float posx, float posy, float posz, 
    float pitch, float yaw,
    float render_width, float render_height, 
    float near_clip_plane, float far_clip_plane
) {
    camera res;    
    float3 pos = mk_float3(posx, posy, posz);

    float3 forward = mk_float3(1.0f, 0.0f, 0.0f);
    float3 right = mk_float3(1.0f, 0.0f, -1.0f);
    float3 up = mk_float3(0.0f, -1.0f, 0.0f); //float3_cross_float3(forward, right);


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

    
    float sy = my_sinf(yaw);
    float cy = my_cosf(yaw);

    mat3 yaw_rot_mat = mk_mat3(
        cy,   0.0f, sy,
        0.0f, 1.0f, 0.0f,
        -sy,  0.0f, cy
    );

    forward = float3_normalize( mat3_mul_float3(yaw_rot_mat, forward));
    right = float3_normalize(mat3_mul_float3(yaw_rot_mat, right));
    up = float3_normalize(mat3_mul_float3(yaw_rot_mat, up));

    
    forward.x = my_cosf(pitch) * my_cosf(yaw);
    forward.y = my_sinf(pitch);
    forward.z = my_cosf(pitch) * my_sinf(yaw); 

    float up_pitch = pitch+QUARTER_CIRCLE_RADS;
    up.x = my_cosf(up_pitch) * my_cosf(yaw);
    up.y = my_sinf(up_pitch);
    up.z = my_cosf(up_pitch) * my_sinf(yaw); 


    // right y portion is currently ALWAYS zero, no roll
    // affected by y only
    //float right_yaw = yaw+1.5707f;
    right = float3_cross_float3(up, forward); // left hand rule?
    //right.x = my_cosf(right_yaw);
    //right.y = 0.0f;
    //right.z = my_sinf(right_yaw);

    
    // up is the forward vector, calculated with a pitch + 90 degrees
    //float up_pitch = pitch + 1.5707f;
    //up.x = my_cosf(up_pitch) * my_cosf(yaw);
    //up.y = my_sinf(up_pitch); // the UP portion, affected by pitch
    //up.z = my_cosf(up_pitch) * my_sinf(yaw); 




    float2 dims = mk_float2(render_width, render_height);
    float fov = 90.0f;

    return ((camera){
        .dims = dims,
        .near_clip = near_clip_plane,
        .far_clip = far_clip_plane,
        .fov = fov,
        .forward = forward,
        .up = up,
        .right = right,
        .pos = mk_float3(posx, posy, posz)
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
    seg.index = segment_index;


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
    float4 plane_ray_direction = mk_float4(ray_dir.x, 0.0, ray_dir.y, 0);

    float4 full_plane_start_top_projected = mat4_mul_float4(world_to_screen_mat, plane_start_top);
    float4 full_plane_start_bot_projected = mat4_mul_float4(world_to_screen_mat, plane_start_bottom);
    float4 full_plane_ray_direction_projected = mat4_mul_float4(world_to_screen_mat, plane_ray_direction);
    if (y_axis == 0) {
        return ((projected_plane_params){
            mk_float3(full_plane_start_bot_projected.x, full_plane_start_bot_projected.y, full_plane_start_bot_projected.w),
            mk_float3(full_plane_start_top_projected.x, full_plane_start_top_projected.y, full_plane_start_top_projected.w),
            mk_float3(full_plane_ray_direction_projected.x, full_plane_ray_direction_projected.y, full_plane_ray_direction_projected.w)
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
    float2 top, bot;
    u8 on_screen;
} clip_res;

clip_res clip_homogeneous_camera_space_line(
    float3 a, float3 b
) {
    
    float ax = a.x;
    float ay = a.y;
    float az = a.z;
    float bx = b.x;
    float by = b.y;
    float bz = b.z;

    if (ay <= 0.0f) {
        if (by <= 0.0f) {
            return ((clip_res){.on_screen = 0, .top=mk_float2(ax,az), .bot=mk_float2(bx,bz)});
        }
        float v = by / (by - ay);
        float2 clip_a = float2_lerp(mk_float2(bx,bz), mk_float2(ax,az), v);
        return ((clip_res){.on_screen=1, .top=clip_a, .bot=mk_float2(bx,bz)});
    } else if (by <= 0.0f) {
        float v = ay / (ay - by);
        float2 clip_b = float2_lerp(mk_float2(ax,az), mk_float2(bx,bz), v);
        return ((clip_res){.on_screen=1, .top=mk_float2(ax,az), .bot=clip_b});
    } else {
        return ((clip_res){.on_screen=1, .top=mk_float2(ax,az), .bot=mk_float2(bx,bz)});
    }
}

typedef struct {
    int drawn;
    int top, bot;
} drawn_extent;

drawn_extent fill_raybuffer_column(
    u32* ray_buffer,
    int col,
    float2 cam_space_top, float2 cam_space_bot,
    int prev_drawn_top, int prev_drawn_bot,
    u32 color
) {
    
    float ray_buffer_bounds_float_min = cam_space_top.x / cam_space_top.y;
    float ray_buffer_bounds_float_max = cam_space_bot.x / cam_space_bot.y;
    
    int ray_buffer_bounds_min = my_roundf(ray_buffer_bounds_float_min);
    int ray_buffer_bounds_max = my_roundf(ray_buffer_bounds_float_max);

    int draw_min = MAX(ray_buffer_bounds_float_min, prev_drawn_top);
    int draw_max = MIN(ray_buffer_bounds_max, prev_drawn_bot);


    int drawn = 0;
    for(int y = draw_min; y < draw_max+1; y++) {
        drawn = 1;
        ray_buffer[col*FP_SCREEN_HEIGHT+y] = col;
    }

    return ((drawn_extent){.drawn = drawn, .top = draw_min, .bot = draw_max});
}

void execute_rays_in_segment(
    u32* ray_buffer,
    int ray_buffer_base_offset,
    segment seg,
    camera cam,
    mat4 world_to_screen_mat,
    int axis_mapped_to_y,
    level* this_level
) { 
    float world_max_y = MAX_WALL_HEIGHT;
    float one_over_world_max_y = 1.0f / world_max_y;
    float camera_pos_y_normalized = cam.pos.y / world_max_y;

    u8* cur_level_floor = this_level->floor;
    u8* cur_level_ceil = this_level->ceil;
    u8* cur_level_upper_floor = this_level->upper_floor;
    u8* cur_level_upper_ceil = this_level->upper_ceil;

    float2 cam_pos_xz = mk_float2(cam.pos.x, cam.pos.z);
    for(int ray_segment_idx = 0; ray_segment_idx < seg.ray_count; ray_segment_idx++) {
        float end_ray_lerp = (float)ray_segment_idx/seg.ray_count;
        float2 cam_local_plane_ray_direction = float2_lerp(seg.cam_local_plane_ray_min, seg.cam_local_plane_ray_max, end_ray_lerp);
        float2 norm_ray_direction = float2_normalize(cam_local_plane_ray_direction);
        int ray_column = ray_buffer_base_offset + ray_segment_idx;

        float2 ray_start = cam_pos_xz;
        float2 ray_dir = norm_ray_direction;
        float ray_origin_x = ray_start.x;
        float ray_origin_y = ray_start.y;
        float ray_dir_x = ray_dir.x;
        float ray_dir_y = ray_dir.y;

        projected_plane_params plane = get_projected_plane_params(
            world_to_screen_mat, 
            ray_start, ray_dir, world_max_y, axis_mapped_to_y
        );


        
        float3 plane_top = plane.plane_top;
        float3 plane_bot = plane.plane_bottom;
        float3 plane_dir = plane.plane_ray_direction;

        int num_sprites_hit = 0;
        
        int rem_steps = MAX_STEPS;

        float perp_dist = NEAR_PLANE_DIST;

        int prev_drawn_top = seg.next_free_pixel_min;
        int prev_drawn_bot = seg.next_free_pixel_max;
        //int cur_min = 
        //int cur_max = 

        int map_x = my_floorf(ray_origin_x);
        int map_y = my_floorf(ray_origin_y);
        const int start_map_x = map_x;
        const int start_map_y = map_y;
        
        // length of ray from one x/y side to the next x/y side
        float delta_dist_x = my_fabsf(1.0f / ray_dir_x);
        float delta_dist_y = my_fabsf(1.0f / ray_dir_y);


        float flat_u = ray_origin_x - my_floorf(ray_origin_x);
        float flat_v = ray_origin_y - my_floorf(ray_origin_y);           // the u,v position of where we enter the next cell (which we use on the next iteration)
        
        int step_x = (ray_dir_x < 0) ? -1 : 1;
        float side_dist_x = (ray_dir_x < 0) ? ((ray_origin_x - map_x) * delta_dist_x) : ((map_x + 1.0f - ray_origin_x) * delta_dist_x);

        int step_y = (ray_dir_y < 0) ? -1 : 1;
        float side_dist_y = (ray_dir_y < 0) ? ((ray_origin_y - map_y) * delta_dist_y) : ((map_y + 1.0 - ray_origin_y) * delta_dist_y);


        
        int next_map_x = map_x;
        int next_map_y = map_y;
        float next_perp_dist = perp_dist;
        

        int next_side;
        wall_side side = HORIZONTAL_SIDE;
        float light_factor = 0.75f;
        float next_light_factor;
            

        for(int step = 0; 
            (step < rem_steps) && 
            (prev_drawn_top < prev_drawn_bot);
            step++,
            map_x = next_map_x,
            map_y = next_map_y,
            perp_dist = next_perp_dist,
            side = next_side,
            light_factor = next_light_factor
        ) {
            float wall_u;
            float exit_flat_u, exit_flat_v; // the u,v position of where we "exit" the current cell before stepping to the new one
            float hit_x;
            float hit_y;

            if(side_dist_x < side_dist_y) {
                side_dist_x += delta_dist_x;
                next_map_x = map_x + step_x;
                next_side = VERTICAL_SIDE;
                next_light_factor = 1.0f;
                next_perp_dist = ((next_map_x - ray_origin_x + (1 - step_x) * 0.5f) / ray_dir_x);
            } else {
                side_dist_y += delta_dist_y;
                next_map_y = map_y + step_y;
                next_side = HORIZONTAL_SIDE;
                next_light_factor = .75f;
                next_perp_dist = ((next_map_y - ray_origin_y + (1 - step_y) * 0.5f) / ray_dir_y);
            }
            if(map_x >= MAP_SIZE || map_x < 0 || map_y >= MAP_SIZE || map_y < 0) {
                break;
            }
            next_perp_dist = MAX(next_perp_dist, perp_dist);

            int map_idx = map_y * MAP_SIZE + map_x;
            int in_start_cell = (map_x == start_map_x && map_y == start_map_y);

            float3 cam_bot_cur = float3_add_float3(plane_bot, float3_mul_float(plane_dir, perp_dist));
            float3 cam_top_cur = float3_add_float3(plane_top, float3_mul_float(plane_dir, perp_dist));

            float3 cam_bot_next = float3_add_float3(plane_bot, float3_mul_float(plane_dir, next_perp_dist));
            float3 cam_top_next = float3_add_float3(plane_top, float3_mul_float(plane_dir, next_perp_dist));
            
            float ceil_height = cur_level_ceil[map_idx];
            float floor_height = cur_level_floor[map_idx];

            float max_height = MAX_WALL_HEIGHT;
            float min_height = 0.0f;

            //float portion_ceil = ceil_height * one_over_world_max_y;
            float portion_floor = floor_height * one_over_world_max_y;

            //float3 cam_ceil_top = cam_top_cur;
            //float3 cam_ceil_bot   = float3_lerp(cam_bot_cur,  cam_top_cur,  portion_ceil);

            //float3 cam_ceil_next  = float3_lerp(cam_bot_next, cam_top_next, portion_ceil);
            
            float3 cam_floor_top  = float3_lerp(cam_bot_cur,  cam_top_cur,  portion_floor);
            float3 cam_floor_bot = cam_bot_cur;

            //float3 cam_floor_next = float3_lerp(cam_bot_next, cam_top_next, portion_floor);


            
            

            // let's just draw front faces for now
            if(!in_start_cell) {
                //clip_res ceil_clipped = clip_homogeneous_camera_space_line(cam_ceil_top, cam_ceil_bot);
                /*
                if(ceil_clipped.on_screen) {
                    // fill column from top to bottom
                    drawn_extent drawn = fill_raybuffer_column(
                        ray_buffer, ray_column,
                        ceil_clipped.top, ceil_clipped.bot,
                        prev_drawn_top, prev_drawn_bot,
                        0xFF00FF00
                    );
                    if(drawn.drawn) {
                        // set prev drawn top
                        prev_drawn_top = MAX(drawn.bot, prev_drawn_top);
                    }
                }
                */

                clip_res floor_clipped = clip_homogeneous_camera_space_line(cam_floor_top, cam_floor_bot);
                if(floor_clipped.on_screen) {
                    // fill column from top to bottom
                    drawn_extent drawn = fill_raybuffer_column(
                        ray_buffer, ray_column,
                        floor_clipped.top, floor_clipped.bot,
                        prev_drawn_top, prev_drawn_bot,
                        0xFF0000FF
                    );
                    //draw_lit_fogged_clipped_textured_wall(
                    //    output, z_buffer,
                    //    (lower_wall_tex == SKYBOX_TEX_IDX),
                    //    get_texture_column(textures[lower_wall_tex], wall_u),skybox_column,
                    //    screen_x, proj_floor_first_step_height, proj_floor_anchor_height,
                    //    first_floor_height, floor_anchor, BOTTOM_PEGGED,
                    //    prev_drawn_top, prev_drawn_bot, perp_dist, light_factor, lower_intersect_wall_light_level, 
                    //   FOG_COL, REPEAT_TEX
                    //);

                    // set prev drawn bot
                    if(drawn.drawn) {
                        prev_drawn_bot = MIN(drawn.top, prev_drawn_bot);
                    }
                }
                
            }

            //prev_drawn_bot = MIN(cam_floor_top, prev_drawn_bot);
            //prev_drawn_top = MAX(cam_ceil_bot, prev_drawn_top);

            //int proj_floor_anchor_height = lerp3(cam_bot_cur)




            //float3 slab_bot_cur = float3_add_float3(plane.plane_top, float3_mul_float(plane.plane_ray_direction, dist_x));
            //float3 slab_top_cur = float3_add_float3(plane.plane_bottom, float3_mul_float(plane.plane_ray_direction, dist_x));
            

        }
    }
}