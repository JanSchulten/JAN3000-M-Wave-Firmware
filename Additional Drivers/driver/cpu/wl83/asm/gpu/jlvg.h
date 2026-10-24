/**
 * @file jlvg.h
 * @author xuhui (xuhui@zh-jieli.com)
 * @brief
 * @version 0.1
 * @date 2024-03-14
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef __JLVG_H__
#define __JLVG_H__



#include "jlvg_path.h"
#include "jlvg_math.h"
#include "jlvg_mem.h"
#include "jlvg_image.h"
#include "jlvg_paint.h"
#include "jlvg_utils.h"
#include "jlvg_defs.h"


typedef void jlvg_t;

/**
 * @brief   Create a jlvg  instance
 *
 * @return jlvg_t*
 */
jlvg_t *jlvg_create();

/**
 * @brief Initialize the jlvg instance
 *
 * @param jlvg
 */
void jlvg_init(jlvg_t *jlvg);

/**
 * @brief   Uninitialize a jlvg instance
 *
 * @param jlvg  The jlvg handle
 *
 */
void jlvg_deinit(jlvg_t *jlvg);

/**
 * @brief   Uninitialize a jlvg instance and free its memory
 *
 * @param jlvg  The jlvg handle
 */
void jlvg_free(jlvg_t *jlvg);

//res

/**
 * @brief   Add a pointer to res_list, and the pointer will be freed when jlvg_end_frame called
 *
 * @param jlvg  The jlvg handle
 *
 * @param p     The pointer will be freed
 *
 * @return int
 */
int jlvg_res_list_add(jlvg_t *jlvg, void *p);

/**
 * @brief   Free all pointer in res_list
 *
 * @param jlvg  The jlvg handle
 * @return int
 */
int jlvg_res_list_freeall(jlvg_t *jlvg);

//op list

/**
 * @brief   Add a op to op_list
 *
 * @param jlvg  The jlvg handle
 * @param op    The op to added to the op_list
 * @return int
 */
int jlvg_op_list_add(jlvg_t *jlvg, void *op);

/**
 * @brief   Free all op in op_list
 *
 * @param jlvg  The jlvg handle
 * @return int
 */
int jlvg_op_list_freeall(jlvg_t *jlvg);

//path list

/**
 * @brief Add path to path_list
 *
 * @param jlvg  The jlvg handle
 * @param path  The path to be added
 * @return int
 */
int jlvg_path_list_add(jlvg_t *jlvg, void *path);

/**
 * @brief Free all path in path_list
 *
 * @param jlvg  The jlvg handle
 * @return int
 */
int jlvg_path_list_freeall(jlvg_t *jlvg);

/**
 * @brief Start to draw to the surface
 *
 * @param jlvg  The jlvg handle
 * @param surface   The surface to draw
 * @return int
 */
int jlvg_start_frame(jlvg_t *jlvg, jlvg_surface_t *surface);

/**
 * @brief Finish drawing to the surface, all op cached in op_list will be flushed
 *
 * @param jlvg  The jlvg handle
 * @return int
 */
int jlvg_end_frame(jlvg_t *jlvg);


/**
 * @brief Clear the surface
 *
 * @param jlvg  The jlvg handle
 * @param area  The area to clear
 * @param color The clear color
 * @return int
 */
int jlvg_clear(jlvg_t *jlvg, jlvg_rect_t *area, jlvg_color_t *color);

/**
 * @brief Fill path to the surface
 *
 * @param jlvg  The jlvg handle
 * @param area  The area to fill
 * @param path  The path to fill
 * @param paint     The fill paint
 * @param user2surface  The user to surface transform matrix
 * @param surface2paint The surface to paint transform matrix
 * @param blend_mode    The blend mode
 * @param fill_rule     The fill rule
 * @return int
 */
int jlvg_fill_path(jlvg_t *jlvg, jlvg_rect_t *area, jlvg_path_t *path, jlvg_paint_t *paint, jlvg_matrix_t *user2surface, jlvg_matrix_t *surface2paint, uint8_t blend_mode, jlvg_hw_fillrule_t fill_rule);

/**
 * @brief Draw image to the surface
 *
 * @param jlvg  The jlvg handle
 * @param area  The area to draw
 * @param image  The drawing image
 * @param surface2image     The surface to image transform matrix
 * @param blend_mode    The blend mode
 * @return int
 */
int jlvg_draw_image(jlvg_t *jlvg, jlvg_rect_t *area, jlvg_image_t *image, jlvg_matrix_t *surface2image, uint8_t blend_mode);

/**
 * @brief Draw image to the surface with paint
 *
 * @param jlvg  The jlvg handle
 * @param area  The drawing area
 * @param image     The drawing image
 * @param paint     The drawing paint
 * @param surface2image     The surface to image transform matrix
 * @param surface2paint     The suface to paint transform matrix
 * @param blend_mode    The blend mode
 * @param stencil   1 for stencil mode and 0 for multiply mode
 * @return int
 */
int jlvg_draw_image_with_paint(jlvg_t *jlvg, jlvg_rect_t *area, jlvg_image_t *image, jlvg_paint_t *paint, jlvg_matrix_t *surface2image, jlvg_matrix_t *surface2paint, uint8_t blend_mode, uint8_t stencil);

/**
 * @brief Set msaa level
 *
 * @param jlvg  The jlvg handle
 * @param msaa_lvl  Msaa level to set
 * @return int
 */
int jlvg_set_msaa_lvl(jlvg_t *jlvg, uint8_t msaa_lvl);

/**
 * @brief Get msaa level
 *
 * @param jlvg  The jlvg handle
 * @return int  msaa level
 */
int jlvg_get_msaa_lvl(jlvg_t *jlvg);

/**
 * @brief Set global alpha  [0,255]
 *
 * @param jlvg  The jlvg handle
 * @param gb_alpha  Global alpha to set
 * @return int
 */
int jlvg_set_global_alpha(jlvg_t *jlvg, uint8_t gb_alpha);

/**
 * @brief Get the global alpha
 *
 * @param jlvg  The jlvg handle
 * @return int  Global alpha
 */
uint8_t jlvg_get_global_alpha(jlvg_t *jlvg);

#endif