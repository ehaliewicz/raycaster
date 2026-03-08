#ifndef MY_DEFS_H
#define MY_DEFS_H

float my_sinf(float x);
float my_cosf(float x);
float my_atanf(float x);
float my_atan2f(float y, float x);
float my_fabsf(float x);
float my_floorf(float x);
void *my_memset(void *dst, int c, unsigned int n);
void *my_memcpy(void *dst, const void *src, unsigned int n);
void *my_memmove(void *dst, const void *src, unsigned int n);
float my_sqrtf(float x);
void nop_printf(const char* fmt, ...);

#ifdef DDEBUG
#define debug_printf printf 
#else 
#define debug_printf nop_printf
#endif
//void debug_printf(const char* fmt, ...);
#endif