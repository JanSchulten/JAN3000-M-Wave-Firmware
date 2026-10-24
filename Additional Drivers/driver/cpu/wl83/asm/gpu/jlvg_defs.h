

#ifndef __JLVG_DEFS_H__
#define __JLVG_DEFS_H__

#include <assert.h>
#include <stddef.h>

/* Maximum / minimum values */
#define JLVG_MAX_INT  (0x7fffffff)
#define JLVG_MIN_INT (-0x7fffffff-1)


/* Portable function definitions */

#define JLVG_SQRT   (float)sqrtf
#define JLVG_COS    (float)cosf
#define JLVG_SIN    (float)sinf
#define JLVG_ACOS   (float)acosf
#define JLVG_ASIN   (float)asinf
#define JLVG_ATAN   (float)atanf
#define JLVG_FLOOR  (float)floorf
#define JLVG_CEIL   (float)ceilf
#define JLVG_ASSERT assert


/* Helper macros */
#define JLVG_PI 3.141592654f
#define JLVG_DEG2RAD(a) (a * PI / 180.0f)
#define JLVG_RAD2DEG(a) (a * 180.0f / PI)
#define JLVG_ABS(a) ((a < 0.0f) ? -a : a)
#define JLVG_MAX(a,b) ((a > b) ? a : b)
#define JLVG_MIN(a,b) ((a < b) ? a : b)
#define JLVG_NEARZERO(a) (a >= -0.0001 && a < 0.0001)
#define JLVG_SWAP(a,b) {float t=a; a=b; b=t;}
#define JLVG_CLAMP(a,min,max) {if (a<min) a=min; if (a>max) a=max; }


#endif /* __JLVG_DEFS_H__ */
