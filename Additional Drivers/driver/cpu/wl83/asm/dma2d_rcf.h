
#ifndef __DMA2D_RCF_H__
#define __DMA2D_RCF_H__


struct dma2d_public_con0l {
    volatile unsigned int width:      14;
        volatile unsigned int unuse0:     2;
        volatile unsigned int high:       14;
        volatile unsigned int unuse1:     2;
    };

    struct dma2d_public_con0h {
    volatile unsigned int m2m_mode:   1;
        volatile unsigned int blend_en:   1;
        volatile unsigned int blend_mode: 1;
        volatile unsigned int unuse0:     29;
    };

    struct dma2d_fg_con0l {
    volatile unsigned int color_mode: 5;
        volatile unsigned int rle_en:     1;
        volatile unsigned int paint_mode: 1;
        volatile unsigned int inpfc_lr:   1;
        volatile unsigned int inpfc_rbs:  1;
        volatile unsigned int color_key:  1;
        volatile unsigned int alpha_inv:  1;
        volatile unsigned int alpha_mode: 2;
        volatile unsigned int alpha:      8;
        volatile unsigned int endian:     1;
        volatile unsigned int unuse0:     10;
    };

    struct dma2d_fg_con0h {
    volatile unsigned int color:      32;
    };

    struct dma2d_fg_xyinitl {
    volatile unsigned int xinit:      14;
        volatile unsigned int unuse0:     2;
        volatile unsigned int yinit:      14;
        volatile unsigned int unuse1:     2;
    };

    struct dma2d_fg_xyinith {
    volatile unsigned int instride:   17;
        volatile unsigned int unuse0:     15;
    };

    struct dma2d_fg_baseadrl {
    volatile unsigned int base_adr:   32;
    };

    struct dma2d_fg_baseadrh {
    volatile unsigned int image_size:     22;
        volatile unsigned int unuse0 :      10;
    };

    struct dma2d_fg_clutl {
    volatile unsigned int en:         1;
        volatile unsigned int color_mode: 5;
        volatile unsigned int len:        11;
        volatile unsigned int unuse0:     15;
    };

    struct dma2d_fg_cluth {
    volatile unsigned int adr:        32;
    };

    struct dma2d_bg_con0l {
    volatile unsigned int color_mode: 5;
        volatile unsigned int unuse0:     1;
        volatile unsigned int paint_mode: 1;
        volatile unsigned int inpfc_lr:   1;
        volatile unsigned int inpfc_rbs:  1;
        volatile unsigned int color_key:  1;
        volatile unsigned int alpha_inv:  1;
        volatile unsigned int alpha_mode: 2;
        volatile unsigned int alpha:      8;
        volatile unsigned int endian:     1;
        volatile unsigned int unuse1:     10;
    };

    struct dma2d_bg_con0h {
    volatile unsigned int color:      32;
    };

    struct dma2d_bg_xyinitl {
    volatile unsigned int xinit:      14;
        volatile unsigned int unuse0:     2;
        volatile unsigned int yinit:      14;
        volatile unsigned int unuse1:     2;
    };

    struct dma2d_bg_xyinith {
    volatile unsigned int instride:   17;
        volatile unsigned int unuse0:     15;
    };

    struct dma2d_bg_baseadrl {
    volatile unsigned int base_adr:   32;
    };

    struct dma2d_bg_baseadrh {
    volatile unsigned int unuse0:     32;
    };

    struct dma2d_bg_clutl {
    volatile unsigned int en:         1;
        volatile unsigned int color_mode: 5;
        volatile unsigned int len:        11;
        volatile unsigned int unuse0:     15;
    };

    struct dma2d_bg_cluth {
    volatile unsigned int adr:        32;
    };

    struct dma2d_out_con0l {
    volatile unsigned int color_mode: 5;
        volatile unsigned int outpfc_lr:  1;
        volatile unsigned int outpfc_rbs: 1;
        volatile unsigned int alpha_inv:  1;
        volatile unsigned int dth_en:     1;
        volatile unsigned int endian:     1;   //ONLY RGB565
        volatile unsigned int unuse0:     5;
        volatile unsigned int outstride:  17;
    };

    struct dma2d_out_con0h {
    volatile unsigned int base_adr:   32;
    };

    struct dma2d_out_con1l {
    volatile unsigned int ypnd_set:   14;
        volatile unsigned int ypnd_en:   1;
        volatile unsigned int unuse:   17;
    };
    struct dma2d_out_con1h {
    volatile unsigned int unuse:   32;
    };
    struct dma2d_task_con0l {
    volatile unsigned int task_adr:   32;
    };

    struct dma2d_task_con0h {
    volatile unsigned int ypnd_set:   14;
        volatile unsigned int ypnd_en:    1;
        volatile unsigned int unuse:      17;
    };

    struct dma2d_rcf {
    struct dma2d_public_con0l public_con0l;
    struct dma2d_public_con0h public_con0h;
    struct dma2d_fg_con0l     fg_con0l;
    struct dma2d_fg_con0h     fg_con0h;
    struct dma2d_fg_xyinitl   fg_xyinitl;
    struct dma2d_fg_xyinith   fg_xyinith;
    struct dma2d_fg_baseadrl  fg_base_adr;
    struct dma2d_fg_baseadrh  fg_rle;
    struct dma2d_fg_clutl     fg_clutl;
    struct dma2d_fg_cluth     fg_cluth;
    struct dma2d_bg_con0l     bg_con0l;
    struct dma2d_bg_con0h     bg_con0h;
    struct dma2d_bg_xyinitl   bg_xyinitl;
    struct dma2d_bg_xyinith   bg_xyinith;
    struct dma2d_bg_baseadrl  bg_base_adr;
    struct dma2d_bg_baseadrh  bg_unuse0;
    struct dma2d_bg_clutl     bg_clutl;
    struct dma2d_bg_cluth     bg_cluth;
    struct dma2d_out_con0l    out_con0l;
    struct dma2d_out_con0h    out_con0h;
    // struct dma2d_out_con1l    out_con1l;
    // struct dma2d_out_con1h    out_con1h;
    struct dma2d_task_con0l   task_adr;
    struct dma2d_task_con0h   ypnd;
};

struct dma2d_rcf32 {
    volatile unsigned int public_con0l;
    volatile unsigned int public_con0h;
    volatile unsigned int fg_con0l;
    volatile unsigned int fg_con0h;
    volatile unsigned int fg_xyinitl;
    volatile unsigned int fg_xyinith;
    volatile unsigned int fg_base_adr;
    volatile unsigned int fg_rle;
    volatile unsigned int fg_clutl;
    volatile unsigned int fg_cluth;
    volatile unsigned int bg_con0l;
    volatile unsigned int bg_con0h;
    volatile unsigned int bg_xyinitl;
    volatile unsigned int bg_xyinith;
    volatile unsigned int bg_base_adr;
    volatile unsigned int bg_unuse0;
    volatile unsigned int bg_clutl;
    volatile unsigned int bg_cluth;
    volatile unsigned int out_con0l;
    volatile unsigned int out_con0h;
    // volatile unsigned int out_con1l;
    // volatile unsigned int out_con1h;
    volatile unsigned int task_adr;
    volatile unsigned int ypnd;
};



#endif
