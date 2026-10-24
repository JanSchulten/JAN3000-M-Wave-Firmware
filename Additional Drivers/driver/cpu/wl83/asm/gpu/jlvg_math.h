

#ifndef __JLVG_MATH_H__
#define __JLVG_MATH_H__

#include <stdint.h>

typedef struct {

    float m[3][3];

} jlvg_matrix_t;

typedef struct {

    float m[3][4];

} jlvg_matrix3x4_t;

typedef struct {

    float x;
    float y;

} jlvg_point2_t;

typedef struct {

    float x;
    float y;
    float z;
} jlvg_point3_t;


float jlvg_sinf(float x);
float jlvg_cosf(float x);
jlvg_matrix_t *jlvg_matrix_create();
void jlvg_matrix_free(jlvg_matrix_t *m);
void jlvg_matrix_set_identity(jlvg_matrix_t *m);
void jlvg_matrix_set_translate(jlvg_matrix_t *m, float tx, float ty);
void jlvg_matrix_set_scale(jlvg_matrix_t *m, float sx, float sy);
void jlvg_matrix_set_shear(jlvg_matrix_t *m, float shx, float shy);
void jlvg_matrix_set_rotate(jlvg_matrix_t *m, float a);
void jlvg_matrix_multiply(jlvg_matrix_t *d, jlvg_matrix_t *s0, jlvg_matrix_t *s1);
void jlvg_matrix_multiply34_33(jlvg_matrix3x4_t *out, jlvg_matrix3x4_t *mat_34, jlvg_matrix_t *mat_33);
void jlvg_matrix_mul_const(jlvg_matrix_t *m, float f);
int jlvg_matrix_is_affine(jlvg_matrix_t *m);
int jlvg_matrix_invert(jlvg_matrix_t *dm, jlvg_matrix_t *m);


void jlvg_matrix_translate(jlvg_matrix_t *m, float tx, float ty);
void jlvg_matrix_scale(jlvg_matrix_t *m, float sx, float sy);
void jlvg_matrix_scale_vector3(jlvg_matrix_t *out, float x_scale, float y_scale, float z_scale);
void jlvg_matrix_shear(jlvg_matrix_t *m, float shx, float shy);
void jlvg_matrix_rotate(jlvg_matrix_t *m, float a);
void jlvg_matrix_rotate_vector3(jlvg_matrix_t *m, jlvg_point3_t k, float a);

void jlvg_transform_point2(jlvg_point2_t *dp, jlvg_matrix_t *m, jlvg_point2_t *sp);

void jlvg_point2_add(jlvg_point2_t *d, jlvg_point2_t *s0, jlvg_point2_t *s1);
void jlvg_point2_sub(jlvg_point2_t *d, jlvg_point2_t *s0, jlvg_point2_t *s1);
void jlvg_point2_mul(jlvg_point2_t *d, jlvg_point2_t *s0, jlvg_point2_t *s1);
float jlvg_point2_len(jlvg_point2_t *s0, jlvg_point2_t *s1);
void jlvg_matrix_get_camera(jlvg_matrix3x4_t *camera, float x, float y, float z, float width, float height, float centre_x, float centre_y, float fov_x, float fov_y);
void jlvg_matrix_get_cube(jlvg_matrix_t *out, int index);
void jlvg_point3_get_cube(jlvg_point3_t *out, int index);
void jlvg_transform_3dto2d(jlvg_matrix3x4_t mat_34, jlvg_point3_t in_3, jlvg_point2_t *out_2);

#endif
