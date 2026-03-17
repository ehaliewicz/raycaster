#include "common.h"
#include <xmmintrin.h>

static const float sin_table[256] = {
    0.00000000f, 0.00615995f, 0.01231966f, 0.01847890f, 0.02463745f, 0.03079506f, 0.03695150f, 0.04310654f, 
    0.04925994f, 0.05541147f, 0.06156091f, 0.06770800f, 0.07385253f, 0.07999425f, 0.08613294f, 0.09226836f, 
    0.09840028f, 0.10452846f, 0.11065268f, 0.11677270f, 0.12288829f, 0.12899922f, 0.13510525f, 0.14120615f, 
    0.14730170f, 0.15339165f, 0.15947579f, 0.16555388f, 0.17162568f, 0.17769097f, 0.18374952f, 0.18980109f, 
    0.19584547f, 0.20188241f, 0.20791169f, 0.21393308f, 0.21994636f, 0.22595129f, 0.23194764f, 0.23793520f, 
    0.24391372f, 0.24988299f, 0.25584278f, 0.26179286f, 0.26773300f, 0.27366299f, 0.27958259f, 0.28549159f, 
    0.29138975f, 0.29727685f, 0.30315267f, 0.30901699f, 0.31486959f, 0.32071024f, 0.32653871f, 0.33235480f, 
    0.33815827f, 0.34394892f, 0.34972651f, 0.35549083f, 0.36124167f, 0.36697879f, 0.37270199f, 0.37841105f, 
    0.38410575f, 0.38978587f, 0.39545121f, 0.40110153f, 0.40673664f, 0.41235632f, 0.41796034f, 0.42354851f, 
    0.42912061f, 0.43467642f, 0.44021574f, 0.44573836f, 0.45124406f, 0.45673264f, 0.46220388f, 0.46765759f, 
    0.47309356f, 0.47851157f, 0.48391142f, 0.48929292f, 0.49465584f, 0.50000000f, 0.50532518f, 0.51063119f, 
    0.51591783f, 0.52118488f, 0.52643216f, 0.53165947f, 0.53686660f, 0.54205336f, 0.54721955f, 0.55236497f, 
    0.55748944f, 0.56259275f, 0.56767472f, 0.57273514f, 0.57777383f, 0.58279060f, 0.58778525f, 0.59275760f, 
    0.59770746f, 0.60263464f, 0.60753895f, 0.61242020f, 0.61727822f, 0.62211282f, 0.62692381f, 0.63171101f, 
    0.63647424f, 0.64121331f, 0.64592806f, 0.65061830f, 0.65528385f, 0.65992453f, 0.66454018f, 0.66913061f, 
    0.67369564f, 0.67823512f, 0.68274886f, 0.68723669f, 0.69169844f, 0.69613395f, 0.70054304f, 0.70492555f, 
    0.70928131f, 0.71361015f, 0.71791192f, 0.72218645f, 0.72643357f, 0.73065313f, 0.73484497f, 0.73900892f, 
    0.74314483f, 0.74725253f, 0.75133189f, 0.75538273f, 0.75940492f, 0.76339828f, 0.76736268f, 0.77129796f, 
    0.77520398f, 0.77908057f, 0.78292761f, 0.78674494f, 0.79053241f, 0.79428989f, 0.79801723f, 0.80171428f, 
    0.80538092f, 0.80901699f, 0.81262237f, 0.81619691f, 0.81974048f, 0.82325295f, 0.82673417f, 0.83018403f, 
    0.83360239f, 0.83698911f, 0.84034407f, 0.84366715f, 0.84695821f, 0.85021714f, 0.85344380f, 0.85663808f, 
    0.85979985f, 0.86292900f, 0.86602540f, 0.86908895f, 0.87211951f, 0.87511698f, 0.87808125f, 0.88101219f, 
    0.88390971f, 0.88677369f, 0.88960401f, 0.89240058f, 0.89516329f, 0.89789203f, 0.90058670f, 0.90324720f, 
    0.90587342f, 0.90846527f, 0.91102265f, 0.91354546f, 0.91603360f, 0.91848699f, 0.92090552f, 0.92328911f, 
    0.92563766f, 0.92795109f, 0.93022931f, 0.93247223f, 0.93467977f, 0.93685184f, 0.93898836f, 0.94108925f,
    0.94315443f, 0.94518383f, 0.94717736f, 0.94913494f, 0.95105652f, 0.95294200f, 0.95479132f, 0.95660442f,
    0.95838122f, 0.96012165f, 0.96182564f, 0.96349314f, 0.96512409f, 0.96671840f, 0.96827604f, 0.96979694f,
    0.97128103f, 0.97272827f, 0.97413860f, 0.97551197f, 0.97684832f, 0.97814760f, 0.97940977f, 0.98063477f,
    0.98182256f, 0.98297310f, 0.98408634f, 0.98516223f, 0.98620075f, 0.98720184f, 0.98816547f, 0.98909161f,
    0.98998021f, 0.99083125f, 0.99164470f, 0.99242051f, 0.99315867f, 0.99385914f, 0.99452190f, 0.99514692f,
    0.99573418f, 0.99628365f, 0.99679532f, 0.99726917f, 0.99770518f, 0.99810333f, 0.99846360f, 0.99878599f,
    0.99907048f, 0.99931706f, 0.99952572f, 0.99969645f, 0.99982925f, 0.99992411f, 0.99998103f, 1.00000000f
};

static float sin_90(float t) {
    // t is 0.0 .. 1.0 representing 0..90 degrees
    float idx = t * 255.0f;
    int i = (int)idx;
    float frac = idx - i;
    if (i >= 255) return 1.0f;
    return sin_table[i] + frac * (sin_table[i+1] - sin_table[i]);
}

float sin_deg(float deg) {
    float d = deg - (int)(deg / 360.0f) * 360.0f;
    if (d < 0) d += 360.0f;

    if (d < 90)  return  sin_90(d / 90.0f);
    if (d < 180) return  sin_90((180.0f - d) / 90.0f);
    if (d < 270) return -sin_90((d - 180.0f) / 90.0f);
    return -sin_90((360.0f - d) / 90.0f);
}

float cos_deg(float deg) { return sin_deg(deg + 90.0f); }
float my_sinf(float x) { return sin_deg(x * 57.29577951f); }
float my_cosf(float x) { return cos_deg(x * 57.29577951f); }

float my_tanf(float x) { return my_sinf(x) / my_cosf(x); }

float my_atanf(float x) {
    float a = x < 0 ? -x : x;
    float r = (a > 1.0f) ? 1.0f/a : a;
    float s = r * r;
    float t = s * (0.0776509570923569f * s - 0.287434475393028f);
    t = (t + 0.995181681698119f) * r;
    if (a > 1.0f) t = 1.5707963268f - t;
    return x < 0 ? -t : t;
}

float my_atan2f(float y, float x) {
    if (x > 0.0f)  return my_atanf(y / x);
    if (x < 0.0f)  return my_atanf(y / x) + (y >= 0 ? 3.14159265f : -3.14159265f);
    if (y > 0.0f)  return  1.5707963268f;
    if (y < 0.0f)  return -1.5707963268f;
    return 0.0f; // undefined but whatever
}

float my_fabsf(float x) {
    return x < 0.0f ? -x : x;
}

float my_floorf(float x) {
    int i = (int)x;
    return (float)(i - (x < 0.0f && x != (float)i));
}

void *my_memset(void *dst, int c, unsigned int n) {
    unsigned char *p = dst;
    while (n--) *p++ = (unsigned char)c;
    return dst;
}

void *my_memcpy(void *dst, const void *src, unsigned int n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void *my_memmove(void *dst, const void *src, unsigned int n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    if (d < s) while (n--) *d++ = *s++;
    else { d += n; s += n; while (n--) *--d = *--s; }
    return dst;
}



float my_sqrtf(float x) {
    if (x < 0) return -1.0f;
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x)));
}

//#define debug_printf debug_printf

void nop_printf(const char* fmt, ...) {
}


s32 max_int32(s32 a, s32 b) {
    s32 branch1,
             branch2,
             branch3,
             branch4,
             notBranched,
             signA = a >> 31 & 1,                   // Get the sign of a
             signB = b >> 31 & 1;                   // Get the sign of b
    
    branch1 = signA & (~signB & 1);                 // branch1 = (signA && !signB)
    branch2 = signB & (~signA & 1);                 // branch2 = (signB && !signA)
    notBranched = (~branch1) & (~branch2) & 1;      // notBranched = (!branch1 && !branch2)
    branch3 = notBranched & ((a - b) >> 31 & 1);    // branch3 = (notBranched && a < b)
    branch4 = notBranched & (~branch3) & 1;         // branch4 = (notBranched && b > a)
    
    return (
        (b * (branch1 | branch3)) +
        (a * (branch2 | branch4))
    );
}

s32 min_int32(s32 a, s32 b)
{
    s32 branch1,
             branch2,
             branch3,
             branch4,
             notBranched,
             signA = a >> 31 & 1,                   // Get the sign of a
             signB = b >> 31 & 1;                   // Get the sign of b
    
    /*
    
    The logic here is essentially:
    
    if a is negative and b is positive:
        return a
    elif b is negative and a is positive:
        return b
    elif a - b is negative:
        return a
    elif b - a is negative:
        return b
    
    */
    
    branch1 = signA & (~signB & 1);                 // branch1 = (signA && !signB)
    branch2 = signB & (~signA & 1);                 // branch2 = (signB && !signA)
    notBranched = (~branch1) & (~branch2) & 1;      // notBranched = (!branch1 && !branch2)
    branch3 = notBranched & ((a - b) >> 31 & 1);    // branch3 = (notBranched && a < b)
    branch4 = notBranched & (~branch3) & 1;         // branch4 = (notBranched && b > a)
    
    return (
        (a * (branch1 | branch3)) +
        (b * (branch2 | branch4))
    );
}


float lerp(float start, float end, float amount) {
    float result = start + amount*(end - start);

    return result;
}


float lerp_colors(u32 col1, u32 col2, float amount) {
    float a1 = ((col1>>24)&0xFF)/255.0f;
    float r1 = ((col1>>16)&0xFF)/255.0f;
    float g1 = ((col1>>8)&0xFF)/255.0f;
    float b1 = ((col1>>0)&0xFF)/255.0f;

    float a2 = ((col2>>24)&0xFF)/255.0f;
    float r2 = ((col2>>16)&0xFF)/255.0f;
    float g2 = ((col2>>8)&0xFF)/255.0f;
    float b2 = ((col2>>0)&0xFF)/255.0f;
    float lr = lerp(r1, r2, amount);
    float lg = lerp(g1, g2, amount);
    float lb = lerp(b1, b2, amount);
    float la = lerp(a1, a2, amount);

    int ia = la*255.0f;
    int ir = lr*255.0f;
    int ig = lg*255.0f;
    int ib = lb*255.0f;
    ia &= 0xFF;
    ir &= 0xFF;
    ig &= 0xFF;
    ib &= 0xFF;

    
    return (ia<<24)|(ir<<16)|(ig<<8)|ib;

}
