
#ifndef __WL83_NVDLA__
#define __WL83_NVDLA__

#define np_base            0xfc0000

//............. 0x0000 - 0x0fff............

typedef struct {
    ___RO ___u32 HW_VERSION;;
    ___RO ___u32 GLB_DESC;
    ___RO ___u32 CIF_DESC;
    ___RO ___u32 CIF_CAP_INCOMPAT;
    ___RO ___u32 CIF_CAP_COMPAT;
    ___RO ___u32 CIF_BASE_WIDTH;
    ___RO ___u32 CIF_BASE_LATENCY;
    ___RO ___u32 CIF_BASE_BURST_LENGTH_MAX;
    ___RO ___u32 CIF_BASE_MEM_ADDR_WIDTH;
    ___RO ___u32 CDMA_DESC;
    ___RO ___u32 CDMA_CAP_INCOMPAT;
    ___RO ___u32 CDMA_CAP_COMPAT;
    ___RO ___u32 CDMA_BASE_FEATURE_TYPES;
    ___RO ___u32 CDMA_BASE_WEIGHT_TYPES;
    ___RO ___u32 CDMA_BASE_ATOMIC_C;
    ___RO ___u32 CDMA_BASE_ATOMIC_K;
    ___RO ___u32 CDMA_BASE_ATOMIC_M;
    ___RO ___u32 CDMA_BASE_CBUF_BANK_NUM;
    ___RO ___u32 CDMA_BASE_CBUF_BANK_WIDTH;
    ___RO ___u32 CDMA_BASE_CBUF_BANK_DEPTH;
    ___RO ___u32 CDMA_MULTI_BATCH_MAX;
    ___RO ___u32 CDMA_IMAGE_IN_FORMATS_PACKED;
    ___RO ___u32 CDMA_IMAGE_IN_FORMATS_SEMI;
    ___RO ___u32 CBUF_DESC;
    ___RO ___u32 CBUF_CAP_INCOMPAT;
    ___RO ___u32 CBUF_CAP_COMPAT;
    ___RO ___u32 CBUF_BASE_CBUF_BANK_NUM;
    ___RO ___u32 CBUF_BASE_CBUF_BANK_WIDTH;
    ___RO ___u32 CBUF_BASE_CBUF_BANK_DEPTH;
    ___RO ___u32 CBUF_BASE_CDMA_ID;
    ___RO ___u32 CSC_DESC;
    ___RO ___u32 CSC_CAP_INCOMPAT;
    ___RO ___u32 CSC_CAP_COMPAT;
    ___RO ___u32 CSC_BASE_FEATURE_TYPES;
    ___RO ___u32 CSC_BASE_WEIGHT_TYPES;
    ___RO ___u32 CSC_BASE_ATOMIC_C;
    ___RO ___u32 CSC_BASE_ATOMIC_K;
    ___RO ___u32 CSC_BASE_ATOMIC_M;
    ___RO ___u32 CSC_BASE_CBUF_BANK_NUM;
    ___RO ___u32 CSC_BASE_CBUF_BANK_WIDTH;
    ___RO ___u32 CSC_BASE_CBUF_BANK_DEPTH;
    ___RO ___u32 CSC_BASE_CDMA_ID;
    ___RO ___u32 CSC_MULTI_BATCH_MAX;
    ___RO ___u32 CMAC_A_DESC;
    ___RO ___u32 CMAC_A_CAP_INCOMPAT;
    ___RO ___u32 CMAC_A_CAP_COMPAT;
    ___RO ___u32 CMAC_A_BASE_FEATURE_TYPES;
    ___RO ___u32 CMAC_A_BASE_WEIGHT_TYPES;
    ___RO ___u32 CMAC_A_BASE_ATOMIC_C;
    ___RO ___u32 CMAC_A_BASE_ATOMIC_K;
    ___RO ___u32 CMAC_A_BASE_CDMA_ID;
    ___RO ___u32 CMAC_B_DESC;
    ___RO ___u32 CMAC_B_CAP_INCOMPAT;
    ___RO ___u32 CMAC_B_CAP_COMPAT;
    ___RO ___u32 CMAC_B_BASE_FEATURE_TYPES;
    ___RO ___u32 CMAC_B_BASE_WEIGHT_TYPES;
    ___RO ___u32 CMAC_B_BASE_ATOMIC_C;
    ___RO ___u32 CMAC_B_BASE_ATOMIC_K;
    ___RO ___u32 CMAC_B_BASE_CDMA_ID;
    ___RO ___u32 CACC_DESC;
    ___RO ___u32 CACC_CAP_INCOMPAT;
    ___RO ___u32 CACC_CAP_COMPAT;
    ___RO ___u32 CACC_BASE_FEATURE_TYPES;
    ___RO ___u32 CACC_BASE_WEIGHT_TYPES;
    ___RO ___u32 CACC_BASE_ATOMIC_C;
    ___RO ___u32 CACC_BASE_ATOMIC_K;
    ___RO ___u32 CACC_BASE_CDMA_ID;
    ___RO ___u32 CACC_MULTI_BATCH_MAX;
    ___RO ___u32 SDP_RDMA_DESC;
    ___RO ___u32 SDP_RDMA_CAP_INCOMPAT;
    ___RO ___u32 SDP_RDMA_CAP_COMPAT;
    ___RO ___u32 SDP_RDMA_BASE_ATOMIC_M;
    ___RO ___u32 SDP_RDMA_BASE_SDP_ID;
    ___RO ___u32 SDP_DESC;
    ___RO ___u32 SDP_CAP_INCOMPAT;
    ___RO ___u32 SDP_CAP_COMPAT;
    ___RO ___u32 SDP_BASE_FEATURE_TYPES;
    ___RO ___u32 SDP_BASE_WEIGHT_TYPES;
    ___RO ___u32 SDP_BASE_CDMA_ID;
    ___RO ___u32 SDP_MULTI_BATCH_MAX;
    ___RO ___u32 SDP_BS_THROUGHPUT;
    ___RO ___u32 SDP_BN_THROUGHPUT;
    ___RO ___u32 SDP_EW_THROUGHPUT;
    ___RO ___u32 PDP_RDMA_DESC;
    ___RO ___u32 PDP_RDMA_CAP_INCOMPAT;
    ___RO ___u32 PDP_RDMA_CAP_COMPAT;
    ___RO ___u32 PDP_RDMA_BASE_ATOMIC_M;
    ___RO ___u32 PDP_RDMA_BASE_PDP_ID;
    ___RO ___u32 PDP_DESC;
    ___RO ___u32 PDP_CAP_INCOMPAT;
    ___RO ___u32 PDP_CAP_COMPAT;
    ___RO ___u32 PDP_BASE_FEATURE_TYPES;
    ___RO ___u32 PDP_BASE_THROUGHPUT;
    ___RO ___u32 CDP_RDMA_DESC;
    ___RO ___u32 CDP_RDMA_CAP_INCOMPAT;
    ___RO ___u32 CDP_RDMA_CAP_COMPAT;
    ___RO ___u32 CDP_RDMA_BASE_ATOMIC_M;
    ___RO ___u32 CDP_RDMA_BASE_CDP_ID;
    ___RO ___u32 CDP_DESC;
    ___RO ___u32 CDP_CAP_INCOMPAT;
    ___RO ___u32 CDP_CAP_COMPAT;
    ___RO ___u32 CDP_BASE_FEATURE_TYPES;
    ___RO ___u32 CDP_BASE_THROUGHPUT;
    ___RO ___u32 END_OF_LIST;
} JL_NVDLA_CFGROM_TypeDef;

#define JL_NVDLA_CFGROM_BASE             (np_base + map_adr(0x00, 0x00))
#define JL_NVDLA_CFGROM                  ((JL_NVDLA_CFGROM_TypeDef*)JL_NVDLA_CFGROM_BASE)

//............. 0x1000 - 0x1fff............

typedef struct {
    ___RO ___u32 S_NVDLA_HW_VERSION;
    ___RW ___u32 S_INTR_MASK;
    ___RW ___u32 S_INTR_SET;
    ___RW ___u32 S_INTR_STATUS;
} JL_NVDLA_GLB_TypeDef;

#define JL_NVDLA_GLB_BASE                (np_base + map_adr(0x10, 0x00))
#define JL_NVDLA_GLB                     ((JL_NVDLA_GLB_TypeDef*)JL_NVDLA_GLB_BASE)

//............. 0x2000 - 0x2fff............

typedef struct {
    ___RW ___u32 CFG_RD_WEIGHT_0;
    ___RW ___u32 CFG_RD_WEIGHT_1;
    ___RW ___u32 CFG_RD_WEIGHT_2;
    ___RW ___u32 CFG_WR_WEIGHT_0;
    ___RW ___u32 CFG_WR_WEIGHT_1;
    ___RW ___u32 CFG_OUTSTANDING_CNT;
    ___RW ___u32 STATUS;
} JL_NVDLA_MCIF_TypeDef;

#define JL_NVDLA_MCIF_BASE               (np_base + map_adr(0x20, 0x00))
#define JL_NVDLA_MCIF                    ((JL_NVDLA_MCIF_TypeDef*)JL_NVDLA_MCIF_BASE)

//............. 0x3000 - 0x3fff............

typedef struct {
    ___RW ___u32 S_STATUS;
    ___RW ___u32 S_POINTER;
    ___RW ___u32 S_ARBITER;
    ___RW ___u32 S_CBUF_FLUSH_STATUS;
    ___RW ___u32 D_OP_ENABLE;
    ___RW ___u32 D_MISC_CFG;
    ___RW ___u32 D_DATAIN_FORMAT;
    ___RW ___u32 D_DATAIN_SIZE_0;
    ___RW ___u32 D_DATAIN_SIZE_1;
    ___RW ___u32 D_DATAIN_SIZE_EXT_0;
    ___RW ___u32 D_PIXEL_OFFSET;
    ___RW ___u32 D_DAIN_RAM_TYPE;
    ___RW ___u32 D_DAIN_ADDR_HIGH_0;
    ___RW ___u32 D_DAIN_ADDR_LOW_0;
    ___RW ___u32 D_DAIN_ADDR_HIGH_1;
    ___RW ___u32 D_DAIN_ADDR_LOW_1;
    ___RW ___u32 D_LINE_STRIDE;
    ___RW ___u32 D_LINE_UV_STRIDE;
    ___RW ___u32 D_SURF_STRIDE;
    ___RW ___u32 D_DAIN_MAP;
    ___RW ___u32 D_RESERVED_X_CFG;
    ___RW ___u32 D_RESERVED_Y_CFG;
    ___RW ___u32 D_BATCH_NUMBER;
    ___RW ___u32 D_BATCH_STRIDE;
    ___RW ___u32 D_ENTRY_PER_SLICE;
    ___RW ___u32 D_FETCH_GRAIN;
    ___RW ___u32 D_WEIGHT_FORMAT;
    ___RW ___u32 D_WEIGHT_SIZE_0;
    ___RW ___u32 D_WEIGHT_SIZE_1;
    ___RW ___u32 D_WEIGHT_RAM_TYPE;
    ___RW ___u32 D_WEIGHT_ADDR_HIGH;
    ___RW ___u32 D_WEIGHT_ADDR_LOW;
    ___RW ___u32 D_WEIGHT_BYTES;
    ___RW ___u32 D_WGS_ADDR_HIGH;
    ___RW ___u32 D_WGS_ADDR_LOW;
    ___RW ___u32 D_WMB_ADDR_HIGH;
    ___RW ___u32 D_WMB_ADDR_LOW;
    ___RW ___u32 D_WMB_BYTES;
    ___RW ___u32 D_MEAN_FORMAT;
    ___RW ___u32 D_MEAN_GLOBAL_0;
    ___RW ___u32 D_MEAN_GLOBAL_1;
    ___RW ___u32 D_CVT_CFG;
    ___RW ___u32 D_CVT_OFFSET;
    ___RW ___u32 D_CVT_SCALE;
    ___RW ___u32 D_CONV_STRIDE;
    ___RW ___u32 D_ZERO_PADDING;
    ___RW ___u32 D_ZERO_PADDING_VALUE;
    ___RW ___u32 D_BANK;
    ___RW ___u32 D_NAN_FLUSH_TO_ZERO;
    ___RW ___u32 D_NAN_INPUT_DATA_NUM;
    ___RW ___u32 D_NAN_INPUT_WEIGHT_NUM;
    ___RW ___u32 D_INF_INPUT_DATA_NUM;
    ___RW ___u32 D_INF_INPUT_WEIGHT_NUM;
    ___RW ___u32 D_PERF_ENABLE;
    ___RW ___u32 D_PERF_DAT_READ_STALL;
    ___RW ___u32 D_PERF_WT_READ_STALL;
    ___RW ___u32 D_PERF_DAT_READ_LATENCY;
    ___RW ___u32 D_PERF_WT_READ_LATENCY;
    ___RW ___u32 D_CYA;
} JL_NVDLA_CDMA_TypeDef;

#define JL_NVDLA_CDMA_BASE                (np_base + map_adr(0x30, 0x00))
#define JL_NVDLA_CDMA                     ((JL_NVDLA_CDMA_TypeDef*)JL_NVDLA_CDMA_BASE)

//............. 0x4000 - 0x4fff............

typedef struct {
    ___RW ___u32 S_STATUS;
    ___RW ___u32 S_POINTER;
    ___RW ___u32 D_OP_ENABLE;
    ___RW ___u32 D_MISC_CFG;
    ___RW ___u32 D_DATAIN_FORMAT;
    ___RW ___u32 D_DATAIN_SIZE_EXT_0;
    ___RW ___u32 D_DATAIN_SIZE_EXT_1;
    ___RW ___u32 D_BATCH_NUMBER;
    ___RW ___u32 D_POST_Y_EXTENSION;
    ___RW ___u32 D_ENTRY_PER_SLICE;
    ___RW ___u32 D_WEIGHT_FORMAT;
    ___RW ___u32 D_WEIGHT_SIZE_EXT_0;
    ___RW ___u32 D_WEIGHT_SIZE_EXT_1;
    ___RW ___u32 D_WEIGHT_BYTES;
    ___RW ___u32 D_WMB_BYTES;
    ___RW ___u32 D_DATAOUT_SIZE_0;
    ___RW ___u32 D_DATAOUT_SIZE_1;
    ___RW ___u32 D_ATOMICS;
    ___RW ___u32 D_RELEASE;
    ___RW ___u32 D_CONV_STRIDE_EXT;
    ___RW ___u32 D_DILATION_EXT;
    ___RW ___u32 D_ZERO_PADDING;
    ___RW ___u32 D_ZERO_PADDING_VALUE;
    ___RW ___u32 D_BANK;
    ___RW ___u32 D_PRA_CFG;
    ___RW ___u32 D_CYA;
} JL_NVDLA_CSC_TypeDef;

#define JL_NVDLA_CSC_BASE                 (np_base + map_adr(0x40, 0x00))
#define JL_NVDLA_CSC                      ((JL_NVDLA_CSC_TypeDef*)JL_NVDLA_CSC_BASE)

//............. 0x5000 - 0x5fff............

typedef struct {
    ___RW ___u32 S_STATUS;
    ___RW ___u32 S_POINTER;
    ___RW ___u32 D_OP_ENABLE;
    ___RW ___u32 D_MISC_CFG;
} JL_NVDLA_CMAC_A_TypeDef;

#define JL_NVDLA_CMAC_A_BASE              (np_base + map_adr(0x50, 0x00))
#define JL_NVDLA_CMAC_A                   ((JL_NVDLA_CMAC_A_TypeDef*)JL_NVDLA_CMAC_A_BASE)

//............. 0x6000 - 0x6fff............

typedef struct {
    ___RW ___u32 S_STATUS;
    ___RW ___u32 S_POINTER;
    ___RW ___u32 D_OP_ENABLE;
    ___RW ___u32 D_MISC_CFG;
} JL_NVDLA_CMAC_B_TypeDef;

#define JL_NVDLA_CMAC_B_BASE              (np_base + map_adr(0x60, 0x00))
#define JL_NVDLA_CMAC_B                   ((JL_NVDLA_CMAC_B_TypeDef*)JL_NVDLA_CMAC_B_BASE)

//............. 0x7000 - 0x7fff............

typedef struct {
    ___RW ___u32 S_STATUS;
    ___RW ___u32 S_POINTER;
    ___RW ___u32 D_OP_ENABLE;
    ___RW ___u32 D_MISC_CFG;
    ___RW ___u32 D_DATAOUT_SIZE_0;
    ___RW ___u32 D_DATAOUT_SIZE_1;
    ___RW ___u32 D_DATAOUT_ADDR;
    ___RW ___u32 D_BATCH_NUMBER;
    ___RW ___u32 D_LINE_STRIDE;
    ___RW ___u32 D_SURF_STRIDE;
    ___RW ___u32 D_DATAOUT_MAP;
    ___RW ___u32 D_CLIP_CFG;
    ___RW ___u32 D_OUT_SATURATION;
    ___RW ___u32 D_CYA;
} JL_NVDLA_CACC_TypeDef;

#define JL_NVDLA_CACC_BASE              (np_base + map_adr(0x70, 0x00))
#define JL_NVDLA_CACC                   ((JL_NVDLA_CACC_TypeDef*)JL_NVDLA_CACC_BASE)

//............. 0x8000 - 0x8fff............

typedef struct {
    ___RW ___u32 S_STATUS;
    ___RW ___u32 S_POINTER;
    ___RW ___u32 D_OP_ENABLE;
    ___RW ___u32 D_DATA_CUBE_WIDTH;
    ___RW ___u32 D_DATA_CUBE_HEIGHT;
    ___RW ___u32 D_DATA_CUBE_CHANNEL;
    ___RW ___u32 D_SRC_BASE_ADDR_LOW;
    ___RW ___u32 D_SRC_BASE_ADDR_HIGH;
    ___RW ___u32 D_SRC_LINE_STRIDE;
    ___RW ___u32 D_SRC_SURFACE_STRIDE;
    ___RW ___u32 D_BRDMA_CFG;
    ___RW ___u32 D_BS_BASE_ADDR_LOW;
    ___RW ___u32 D_BS_BASE_ADDR_HIGH;
    ___RW ___u32 D_BS_LINE_STRIDE;
    ___RW ___u32 D_BS_SURFACE_STRIDE;
    ___RW ___u32 D_BS_BATCH_STRIDE;
    ___RW ___u32 D_NRDMA_CFG;
    ___RW ___u32 D_BN_BASE_ADDR_LOW;
    ___RW ___u32 D_BN_BASE_ADDR_HIGH;
    ___RW ___u32 D_BN_LINE_STRIDE;
    ___RW ___u32 D_BN_SURFACE_STRIDE;
    ___RW ___u32 D_BN_BATCH_STRIDE;
    ___RW ___u32 D_ERDMA_CFG;
    ___RW ___u32 D_EW_BASE_ADDR_LOW;
    ___RW ___u32 D_EW_BASE_ADDR_HIGH;
    ___RW ___u32 D_EW_LINE_STRIDE;
    ___RW ___u32 D_EW_SURFACE_STRIDE;
    ___RW ___u32 D_EW_BATCH_STRIDE;
    ___RW ___u32 D_FEATURE_MODE_CFG;
    ___RW ___u32 D_SRC_DMA_CFG;
    ___RW ___u32 D_STATUS_NAN_INPUT_NUM;
    ___RW ___u32 D_STATUS_INF_INPUT_NUM;
    ___RW ___u32 D_PERF_ENABLE;
    ___RW ___u32 D_PERF_MRDMA_READ_STALL;
    ___RW ___u32 D_PERF_BRDMA_READ_STALL;
    ___RW ___u32 D_PERF_NRDMA_READ_STALL;
    ___RW ___u32 D_PERF_ERDMA_READ_STALL;
} JL_NVDLA_SDP_RDMA_TypeDef;

#define JL_NVDLA_SDP_RDMA_BASE            (np_base + map_adr(0x80, 0x00))
#define JL_NVDLA_SDP_RDMA                 ((JL_NVDLA_SDP_RDMA_TypeDef*)JL_NVDLA_SDP_RDMA_BASE)

//............. 0x9000 - 0x9fff............

typedef struct {
    ___RW ___u32 S_STATUS;
    ___RW ___u32 S_POINTER;
    ___RW ___u32 S_LUT_ACCESS_CFG;
    ___RW ___u32 S_LUT_ACCESS_DATA;
    ___RW ___u32 S_LUT_CFG;
    ___RW ___u32 S_LUT_INFO;
    ___RW ___u32 S_LUT_LE_START;
    ___RW ___u32 S_LUT_LE_END;
    ___RW ___u32 S_LUT_LO_START;
    ___RW ___u32 S_LUT_LO_END;
    ___RW ___u32 S_LUT_LE_SLOPE_SCALE;
    ___RW ___u32 S_LUT_LE_SLOPE_SHIFT;
    ___RW ___u32 S_LUT_LO_SLOPE_SCALE;
    ___RW ___u32 S_LUT_LO_SLOPE_SHIFT;
    ___RW ___u32 D_OP_ENABLE;
    ___RW ___u32 D_DATA_CUBE_WIDTH;
    ___RW ___u32 D_DATA_CUBE_HEIGHT;
    ___RW ___u32 D_DATA_CUBE_CHANNEL;
    ___RW ___u32 D_DST_BASE_ADDR_LOW;
    ___RW ___u32 D_DST_BASE_ADDR_HIGH;
    ___RW ___u32 D_DST_LINE_STRIDE;
    ___RW ___u32 D_DST_SURFACE_STRIDE;
    ___RW ___u32 D_DP_BS_CFG;
    ___RW ___u32 D_DP_BS_ALU_CFG;
    ___RW ___u32 D_DP_BS_ALU_SRC_VALUE;
    ___RW ___u32 D_DP_BS_MUL_CFG;
    ___RW ___u32 D_DP_BS_MUL_SRC_VALUE;
    ___RW ___u32 D_DP_BN_CFG;
    ___RW ___u32 D_DP_BN_ALU_CFG;
    ___RW ___u32 D_DP_BN_ALU_SRC_VALUE;
    ___RW ___u32 D_DP_BN_MUL_CFG;
    ___RW ___u32 D_DP_BN_MUL_SRC_VALUE;
    ___RW ___u32 D_DP_EW_CFG;
    ___RW ___u32 D_DP_EW_ALU_CFG;
    ___RW ___u32 D_DP_EW_ALU_SRC_VALUE;
    ___RW ___u32 D_DP_EW_ALU_CVT_OFFSET_VALUE;
    ___RW ___u32 D_DP_EW_ALU_CVT_SCALE_VALUE;
    ___RW ___u32 D_DP_EW_ALU_CVT_TRUNCATE_VALUE;
    ___RW ___u32 D_DP_EW_MUL_CFG;
    ___RW ___u32 D_DP_EW_MUL_SRC_VALUE;
    ___RW ___u32 D_DP_EW_MUL_CVT_OFFSET_VALUE;
    ___RW ___u32 D_DP_EW_MUL_CVT_SCALE_VALUE;
    ___RW ___u32 D_DP_EW_MUL_CVT_TRUNCATE_VALUE;
    ___RW ___u32 D_DP_EW_TRUNCATE_VALUE;
    ___RW ___u32 D_FEATURE_MODE_CFG;
    ___RW ___u32 D_DST_DMA_CFG;
    ___RW ___u32 D_DST_BATCH_STRIDE;
    ___RW ___u32 D_DATA_FORMAT;
    ___RW ___u32 D_CVT_OFFSET;
    ___RW ___u32 D_CVT_SCALE;
    ___RW ___u32 D_CVT_SHIFT;
    ___RW ___u32 D_STATUS;
    ___RW ___u32 D_STATUS_NAN_INPUT_NUM;
    ___RW ___u32 D_STATUS_INF_INPUT_NUM;
    ___RW ___u32 D_STATUS_NAN_OUTPUT_NUM;
    ___RW ___u32 D_PERF_ENABLE;
    ___RW ___u32 D_PERF_WDMA_WRITE_STALL;
    ___RW ___u32 D_PERF_LUT_UFLOW;
    ___RW ___u32 D_PERF_LUT_OFLOW;
    ___RW ___u32 D_PERF_OUT_SATURATION;
    ___RW ___u32 D_PERF_LUT_HYBRID;
    ___RW ___u32 D_PERF_LUT_LE_HIT;
    ___RW ___u32 D_PERF_LUT_LO_HIT;
} JL_NVDLA_SDP_TypeDef;

#define JL_NVDLA_SDP_BASE                 (np_base + map_adr(0x90, 0x00))
#define JL_NVDLA_SDP                      ((JL_NVDLA_SDP_TypeDef*)JL_NVDLA_SDP_BASE)

//............. 0xa000 - 0xafff............

typedef struct {
    ___RW ___u32 S_STATUS;
    ___RW ___u32 S_POINTER;
    ___RW ___u32 D_OP_ENABLE;
    ___RW ___u32 D_DATA_CUBE_IN_WIDTH;
    ___RW ___u32 D_DATA_CUBE_IN_HEIGHT;
    ___RW ___u32 D_DATA_CUBE_IN_CHANNEL;
    ___RW ___u32 D_FLYING_MODE;
    ___RW ___u32 D_SRC_BASE_ADDR_LOW;
    ___RW ___u32 D_SRC_BASE_ADDR_HIGH;
    ___RW ___u32 D_SRC_LINE_STRIDE;
    ___RW ___u32 D_SRC_SURFACE_STRIDE;
    ___RW ___u32 D_SRC_RAM_CFG;
    ___RW ___u32 D_DATA_FORMAT;
    ___RW ___u32 D_OPERATION_MODE_CFG;
    ___RW ___u32 D_POOLING_KERNEL_CFG;
    ___RW ___u32 D_POOLING_PADDING_CFG;
    ___RW ___u32 D_PARTIAL_WIDTH_IN;
    ___RW ___u32 D_PERF_ENABLE;
    ___RW ___u32 D_PERF_READ_STALL;
    ___RW ___u32 D_CYA;
} JL_NVDLA_PDP_RDMA_TypeDef;

#define JL_NVDLA_PDP_RDMA_BASE            (np_base + map_adr(0xa0, 0x00))
#define JL_NVDLA_PDP_RDMA                 ((JL_NVDLA_PDP_RDMA_TypeDef*)JL_NVDLA_PDP_RDMA_BASE)

//............. 0xb000 - 0xbfff............

typedef struct {
    ___RW ___u32 S_STATUS;
    ___RW ___u32 S_POINTER;
    ___RW ___u32 D_OP_ENABLE;
    ___RW ___u32 D_DATA_CUBE_IN_WIDTH;
    ___RW ___u32 D_DATA_CUBE_IN_HEIGHT;
    ___RW ___u32 D_DATA_CUBE_IN_CHANNEL;
    ___RW ___u32 D_DATA_CUBE_OUT_WIDTH;
    ___RW ___u32 D_DATA_CUBE_OUT_HEIGHT;
    ___RW ___u32 D_DATA_CUBE_OUT_CHANNEL;
    ___RW ___u32 D_OPERATION_MODE_CFG;
    ___RW ___u32 D_NAN_FLUSH_TO_ZERO;
    ___RW ___u32 D_PARTIAL_WIDTH_IN;
    ___RW ___u32 D_PARTIAL_WIDTH_OUT;
    ___RW ___u32 D_POOLING_KERNEL_CFG;
    ___RW ___u32 D_RECIP_KERNEL_WIDTH;
    ___RW ___u32 D_RECIP_KERNEL_HEIGHT;
    ___RW ___u32 D_POOLING_PADDING_CFG;
    ___RW ___u32 D_POOLING_PADDING_VALUE_1_CFG;
    ___RW ___u32 D_POOLING_PADDING_VALUE_2_CFG;
    ___RW ___u32 D_POOLING_PADDING_VALUE_3_CFG;
    ___RW ___u32 D_POOLING_PADDING_VALUE_4_CFG;
    ___RW ___u32 D_POOLING_PADDING_VALUE_5_CFG;
    ___RW ___u32 D_POOLING_PADDING_VALUE_6_CFG;
    ___RW ___u32 D_POOLING_PADDING_VALUE_7_CFG;
    ___RW ___u32 D_SRC_BASE_ADDR_LOW;
    ___RW ___u32 D_SRC_BASE_ADDR_HIGH;
    ___RW ___u32 D_SRC_LINE_STRIDE;
    ___RW ___u32 D_SRC_SURFACE_STRIDE;
    ___RW ___u32 D_DST_BASE_ADDR_LOW;
    ___RW ___u32 D_DST_BASE_ADDR_HIGH;
    ___RW ___u32 D_DST_LINE_STRIDE;
    ___RW ___u32 D_DST_SURFACE_STRIDE;
    ___RW ___u32 D_DST_RAM_CFG;
    ___RW ___u32 D_DATA_FORMAT;
    ___RW ___u32 D_INF_INPUT_NUM;
    ___RW ___u32 D_NAN_INPUT_NUM;
    ___RW ___u32 D_NAN_OUTPUT_NUM;
    ___RW ___u32 D_PERF_ENABLE;
    ___RW ___u32 D_PERF_WRITE_STALL;
    ___RW ___u32 D_CYA;
} JL_NVDLA_PDP_TypeDef;

#define JL_NVDLA_PDP_BASE                 (np_base + map_adr(0xb0, 0x00))
#define JL_NVDLA_PDP                      ((JL_NVDLA_PDP_TypeDef*)JL_NVDLA_PDP_BASE)

//............. 0xc000 - 0xcfff............

typedef struct {
    ___RW ___u32 S_STATUS;
    ___RW ___u32 S_POINTER;
    ___RW ___u32 D_OP_ENABLE;
    ___RW ___u32 D_DATA_CUBE_WIDTH;
    ___RW ___u32 D_DATA_CUBE_HEIGHT;
    ___RW ___u32 D_DATA_CUBE_CHANNEL;
    ___RW ___u32 D_SRC_BASE_ADDR_LOW;
    ___RW ___u32 D_SRC_BASE_ADDR_HIGH;
    ___RW ___u32 D_SRC_LINE_STRIDE;
    ___RW ___u32 D_SRC_SURFACE_STRIDE;
    ___RW ___u32 D_SRC_DMA_CFG;
    ___RW ___u32 D_SRC_COMPRESSION_EN;
    ___RW ___u32 D_OPERATION_MODE;
    ___RW ___u32 D_DATA_FORMAT;
    ___RW ___u32 D_PERF_ENABLE;
    ___RW ___u32 D_PERF_READ_STALL;
    ___RW ___u32 D_CYA;
} JL_NVDLA_CDP_RDMA_TypeDef;

#define JL_NVDLA_CDP_RDMA_BASE            (np_base + map_adr(0xc0, 0x00))
#define JL_NVDLA_CDP_RDMA                 ((JL_NVDLA_CDP_RDMA_TypeDef*)JL_NVDLA_CDP_RDMA_BASE)

//............. 0xd000 - 0xdfff............

typedef struct {
    ___RW ___u32 S_STATUS;
    ___RW ___u32 S_POINTER;
    ___RW ___u32 S_LUT_ACCESS_CFG;
    ___RW ___u32 S_LUT_ACCESS_DATA;
    ___RW ___u32 S_LUT_CFG;
    ___RW ___u32 S_LUT_INFO;
    ___RW ___u32 S_LUT_LE_START_LOW;
    ___RW ___u32 S_LUT_LE_START_HIGH;
    ___RW ___u32 S_LUT_LE_END_LOW;
    ___RW ___u32 S_LUT_LE_END_HIGH;
    ___RW ___u32 S_LUT_LO_START_LOW;
    ___RW ___u32 S_LUT_LO_START_HIGH;
    ___RW ___u32 S_LUT_LO_END_LOW;
    ___RW ___u32 S_LUT_LO_END_HIGH;
    ___RW ___u32 S_LUT_LE_SLOPE_SCALE;
    ___RW ___u32 S_LUT_LE_SLOPE_SHIFT;
    ___RW ___u32 S_LUT_LO_SLOPE_SCALE;
    ___RW ___u32 S_LUT_LO_SLOPE_SHIFT;
    ___RW ___u32 D_OP_ENABLE;
    ___RW ___u32 D_FUNC_BYPASS;
    ___RW ___u32 D_DST_BASE_ADDR_LOW;
    ___RW ___u32 D_DST_BASE_ADDR_HIGH;
    ___RW ___u32 D_DST_LINE_STRIDE;
    ___RW ___u32 D_DST_SURFACE_STRIDE;
    ___RW ___u32 D_DST_DMA_CFG;
    ___RW ___u32 D_DST_COMPRESSION_EN;
    ___RW ___u32 D_DATA_FORMAT;
    ___RW ___u32 D_NAN_FLUSH_TO_ZERO;
    ___RW ___u32 D_LRN_CFG;
    ___RW ___u32 D_DATIN_OFFSET;
    ___RW ___u32 D_DATIN_SCALE;
    ___RW ___u32 D_DATIN_SHIFTER;
    ___RW ___u32 D_DATOUT_OFFSET;
    ___RW ___u32 D_DATOUT_SCALE;
    ___RW ___u32 D_DATOUT_SHIFTER;
    ___RW ___u32 D_NAN_INPUT_NUM;
    ___RW ___u32 D_INF_INPUT_NUM;
    ___RW ___u32 D_NAN_OUTPUT_NUM;
    ___RW ___u32 D_OUT_SATURATION;
    ___RW ___u32 D_PERF_ENABLE;
    ___RW ___u32 D_PERF_WRITE_STALL;
    ___RW ___u32 D_PERF_LUT_UFLOW;
    ___RW ___u32 D_PERF_LUT_OFLOW;
    ___RW ___u32 D_PERF_LUT_HYBRID;
    ___RW ___u32 D_PERF_LUT_LE_HIT;
    ___RW ___u32 D_PERF_LUT_LO_HIT;
    ___RW ___u32 D_CYA;
} JL_NVDLA_CDP_TypeDef;

#define JL_NVDLA_CDP_BASE            (np_base + map_adr(0xd0, 0x00))
#define JL_NVDLA_CDP                 ((JL_NVDLA_CDP_TypeDef*)JL_NVDLA_CDP_BASE)

#endif

