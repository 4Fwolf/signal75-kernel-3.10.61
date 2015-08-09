#define LOG_TAG "MET" 

#include "ddp_log.h"

#include <linux/met_drv.h>
#include "ddp_irq.h"
#include "ddp_reg.h"
#include "ddp_met.h"
#include "ddp_ovl.h"
#include "ddp_rdma.h"

typedef enum{
   MODE_DIRECT_LINK = 0,
   MODE_DECOUPLE    = 1,
}DDP_MODE_MET_TYPE;

enum MET_DDP_REF {
    REF_RDMA =0,
    REF_MUTEX=1,
};

#define DDP_IRQ_EER_ID 0xFFFF0000
#define DDP_IRQ_SOF_ID (DDP_IRQ_EER_ID + 1)
#define DDP_IRQ_SOF_BYTECNT_ID (DDP_IRQ_EER_ID + 2)
#define DDP_IRQ_SOF_LAYERCNT_ID (DDP_IRQ_EER_ID + 3)


#define MAX_PATH_NUM (3)
#define OVL_NUM (2)
#define RDMA_NUM (3)
#define MAX_OVL_LAYERS (4)

unsigned int met_tag_on = 0;

static unsigned int g_ovlFrameUpdate[MAX_PATH_NUM];
static unsigned int g_rdmaFrameUpdate[MAX_PATH_NUM];

static DDP_MODE_MET_TYPE g_ddp_mode[MAX_PATH_NUM] = {MODE_DIRECT_LINK,MODE_DIRECT_LINK,MODE_DECOUPLE};
static int g_ovl0_mutex = 0;
static int g_ovl1_mutex = 1;

const char* const parse_color_format (DpColorFormat fmt) {
	switch (fmt) {
	case eBGR565:
		return "eBGR565";
	case eRGB565:
		return "eRGB565";
	case eRGB888:
		return "eRGB888";
	case eBGR888:
		return "eBGR888";
	case eRGBA8888:
		return "eRGBA8888";
	case eBGRA8888:
		return "eBGRA8888";
	case eARGB8888:
		return "eARGB8888";
	case eABGR8888:
		return "eABGR8888";
	case eVYUY:
		return "eVYUY";
	case eUYVY:
		return "eUYVY";
	case eYVYU:
		return "eYVYU";
	case eYUY2:
		return "eYUY2";
	}
}

void ddp_sof_irq_met_tag(int index,DDP_MODE_MET_TYPE mode, enum MET_DDP_REF ref)
{
    int i=0;
    /* to separate rdma,mutex updata line*/
    int ref_tag_value = 1;

    unsigned int u4Width = 0, u4Height = 0;
    unsigned int u4Bpp = 0;
    unsigned int u4LayerCnt = 0;
    unsigned int u4RegVal = 0;
    unsigned int u4EnableFlags;
    char* fmt;
    char reference_tag_name[30]={'\0'};
    char layer_tag_name[30]={'\0'};
    
    static unsigned int u4EnableLayersTbl[2];
    static OVL_BASIC_STRUCT ovl_info[4];
    static RDMA_BASIC_STRUCT rdma_info;
    static unsigned int u4OvlAddr[2][4];
    static unsigned int u4RdmaAddr[3];

    u4EnableFlags = u4EnableLayersTbl[index];

    g_rdmaFrameUpdate[index] = 0;
	DDPDBG("sof_irq_met_tag:index %d, mode %d, ref %d 0x%x\n",index, mode, ref);

    /* check ovl*/
    if((MODE_DIRECT_LINK == mode && ref == REF_RDMA) || 
        (MODE_DECOUPLE == mode && ref == REF_MUTEX))
    {
        //Any layer enable bit changes , new frame refreshes
        ASSERT(index < OVL_NUM);
        u4RegVal = (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_OVL_INDEX_OFFSET*index) & 0xF);

        u4EnableLayersTbl[index] = u4RegVal;
        //g_ovlFrameUpdate[index] = 1;
        ovl_get_info(index,ovl_info);
		u4Width  = ovl_info[i].src_w;
		u4Height = ovl_info[i].src_h;
		u4Bpp = ovl_info[i].bpp;
		fmt = parse_color_format(ovl_info[i].fmt);

        for(i=0;i<4;i++)
        {
        	if(ovl_info[i].layer_en)
        	{
        		sprintf(reference_tag_name,"O%dL%d_Fps_%s_%d", index, i, fmt, u4Bpp);
        		met_tag_oneshot(DDP_IRQ_SOF_BYTECNT_ID , reference_tag_name, index*MAX_OVL_LAYERS+i);

        		u4LayerCnt += 1;
        		if ((u4OvlAddr[index][i] != ovl_info[i].addr)) {
        			u4OvlAddr[index][i] = ovl_info[i].addr;
        			sprintf(reference_tag_name,"O%dL%d_InFps_%dx%d", index, i, u4Width, u4Height);
        			met_tag_oneshot(DDP_IRQ_SOF_ID, reference_tag_name, index*MAX_OVL_LAYERS+i);
        		}
        	} else {
        		u4OvlAddr[index][i] = 0;
        	}
        }
        sprintf(layer_tag_name,"OVL%d_LayerCnt",index);
        met_tag_oneshot(DDP_IRQ_SOF_LAYERCNT_ID , layer_tag_name, u4LayerCnt);
    }
    /* check rdmal*/
    if(MODE_DECOUPLE == mode && ref == REF_RDMA)
    {
        //read rdma
        ASSERT(index < RDMA_NUM);
        u4LayerCnt = 0;
        if (DISP_REG_GET(index*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_GLOBAL_CON) & 0x1){
            rdma_get_info(index,&rdma_info);
            u4Width =rdma_info.src_w;
            u4Height = rdma_info.src_h;
            u4Bpp = rdma_info.bpp;
            if(rdma_info.addr!= u4RdmaAddr[index])
            {
                u4RdmaAddr[index] = rdma_info.addr;
                g_rdmaFrameUpdate[index] = 1;
            }
        }
        if(g_rdmaFrameUpdate[index])
        {
            sprintf(reference_tag_name,"R%d_Fps_%dx%d_%d",index, u4Width, u4Height, u4Bpp);
            DDPDBG("%s frame update\n",reference_tag_name);
            met_tag_oneshot(DDP_IRQ_SOF_ID, reference_tag_name, index*MAX_OVL_LAYERS+i);
        }
//        sprintf(layer_tag_name,"R%d_LayerCnt_%d",index, u4LayerCnt);
//        met_tag_oneshot(DDP_IRQ_SOF_LAYERCNT_ID , layer_tag_name, u4LayerCnt);
    }
    return;
}

void ddp_eof_irq_met_tag(int index,DDP_MODE_MET_TYPE mode, enum MET_DDP_REF ref)
{
    char byte_tag_name[30]={'\0'};
    char layer_tag_name[30]={'\0'};
    if ((MODE_DIRECT_LINK == mode && ref == REF_RDMA) || 
        (MODE_DECOUPLE == mode && ref == REF_MUTEX)) {
        sprintf(byte_tag_name,"ovl%d_byteCnt",index);
        sprintf(layer_tag_name,"ovl%d_LayerCnt",index);
    }else if (MODE_DECOUPLE == mode && ref == REF_RDMA){
        sprintf(byte_tag_name,"rdma%d_byteCnt",index);
        sprintf(layer_tag_name,"rdma%d_LayerCnt",index);    
    }else{
        ASSERT(0);
    }

//    met_tag_oneshot(DDP_IRQ_SOF_BYTECNT_ID ,  byte_tag_name, 0);
//    met_tag_oneshot(DDP_IRQ_SOF_LAYERCNT_ID , layer_tag_name , 0);
    return;
}

void ddp_err_irq_met_tag(const char *name)
{
    met_tag_oneshot(DDP_IRQ_EER_ID , name , 0);
    return;
}

static void met_irq_handler(DISP_MODULE_ENUM module,unsigned int reg_val)
{
    int index =0;
    int mutex_idx = 0;
    char tag_name[30]={'\0'};
    switch(module)
    {
        case DISP_MODULE_RDMA0:
        case DISP_MODULE_RDMA1:
        case DISP_MODULE_RDMA2:
            index = module - DISP_MODULE_RDMA0;
            if(reg_val&(1<<4))
            {
                sprintf(tag_name,"rdma%d_underflow",index);
                ddp_err_irq_met_tag(tag_name);
            }
            if(reg_val&(1<<3))
            {
                sprintf(tag_name,"rdma%d_abnormal",index);
                ddp_err_irq_met_tag(tag_name);
            }
            if(reg_val&(1<<2))
            {
                ddp_eof_irq_met_tag(index,g_ddp_mode[index],REF_RDMA);
            }
            if(reg_val&(1<<1))
            {
                ddp_sof_irq_met_tag(index,g_ddp_mode[index],REF_RDMA);
            }
            break;
        case DISP_MODULE_OVL0:
            index = module - DISP_MODULE_OVL0;
            if(reg_val&(1<<2))
            {
                sprintf(tag_name,"ovl%d_underrun",index);
                ddp_err_irq_met_tag(tag_name);
            }
            if(reg_val&(1<<5))
            {
                sprintf(tag_name,"ovl%d_r0_ncomplete",index);
                ddp_err_irq_met_tag(tag_name);
            }
            if(reg_val&(1<<6))
            {
                sprintf(tag_name,"ovl%d_r1_ncomplete",index);
                ddp_err_irq_met_tag(tag_name);
            }                
            if(reg_val&(1<<7))
            {
                sprintf(tag_name,"ovl%d_r2_ncomplete",index);
                ddp_err_irq_met_tag(tag_name);
            }  
            if(reg_val&(1<<8))
            {
                sprintf(tag_name,"ovl%d_r3_ncomplete",index);
                ddp_err_irq_met_tag(tag_name);
            }  
            if(reg_val&(1<<9))
            {
                sprintf(tag_name,"ovl%d_r0_underflow",index);
                ddp_err_irq_met_tag(tag_name);
            }  
            if(reg_val&(1<<10))
            {
                sprintf(tag_name,"ovl%d_r1_underflow",index);
                ddp_err_irq_met_tag(tag_name);
            }  
            if(reg_val&(1<<11))
            {
                sprintf(tag_name,"ovl%d_r2_underflow",index);
                ddp_err_irq_met_tag(tag_name);
            }  
            if(reg_val&(1<<12))
            {
                sprintf(tag_name,"ovl%d_r3_underflow",index);
                ddp_err_irq_met_tag(tag_name);
            }            
            break;
        case DISP_MODULE_MUTEX:
            /*just consider mutex1 & mutex2*/
            for (mutex_idx = 1; mutex_idx <= 2; mutex_idx++) {
                if (reg_val & (0x1<<mutex_idx)) {
                    /*ovl0 or ovl1 decouple*/
                    if ((mutex_idx == g_ovl0_mutex) && (g_ddp_mode[0] == MODE_DECOUPLE)) {
                        ddp_sof_irq_met_tag(0,g_ddp_mode[0],REF_MUTEX);
						ddp_eof_irq_met_tag(0,g_ddp_mode[0],REF_MUTEX);
                    }else if((mutex_idx == g_ovl1_mutex) && (g_ddp_mode[1] == MODE_DECOUPLE)){
                        ddp_sof_irq_met_tag(1,g_ddp_mode[1],REF_MUTEX);
						ddp_eof_irq_met_tag(1,g_ddp_mode[1],REF_MUTEX);
                    }
                }
            }
            break;            
        default:
            break;
    }
    return ;
}

void ddp_init_met_tag(int state,int rdma0_mode, int rdma1_mode)
{

#if 0
    //maybe auto detect mode? but need to start case first
    /* address of rdma is not 0 it's decouple mode, or directlink mode*/

    /*rdma0 , if directlink ovl0 use mutex 0 or use mutex 1*/
    if (DISP_REG_GET(DISP_REG_RDMA_MEM_START_ADDR)) {
        g_ddp_mode[0] = MODE_DECOUPLE;
        g_ovl0_mutex = 1;
    }else{
        g_ddp_mode[0] = MODE_DIRECT_LINK;
        g_ovl0_mutex = 0;
    }

    /*rdma1  ovl1 use mutex (ovl0_mutex + 1) */
    if (DISP_REG_GET(DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_MEM_START_ADDR)) {
        g_ddp_mode[1] = MODE_DECOUPLE;
        g_ovl1_mutex = g_ovl0_mutex + 1;
    }else{
        g_ddp_mode[1] = MODE_DIRECT_LINK;
        g_ovl1_mutex = g_ovl0_mutex + 1;
    }
    
    /*rdma2 */
    if (DISP_REG_GET(2*DISP_RDMA_INDEX_OFFSET+ DISP_REG_RDMA_MEM_START_ADDR)) {
        g_ddp_mode[2] = MODE_DECOUPLE;
    }else{
        g_ddp_mode[2] = MODE_DIRECT_LINK;
    }
#endif

    if(rdma0_mode <=1)
    {
        g_ddp_mode[0] = rdma0_mode;
        if(g_ddp_mode[0] == MODE_DECOUPLE)
            g_ovl0_mutex = 1;
    }
    if(rdma1_mode <=1)
    {
        g_ddp_mode[1] = rdma1_mode;
        g_ovl1_mutex = g_ovl0_mutex + 1;
    }
    
    if((!met_tag_on) && state)
    {
        met_tag_on = state;
        disp_register_irq_callback(met_irq_handler);
    }
    if(met_tag_on && (!state))
    {
        met_tag_on = state;
        disp_unregister_irq_callback(met_irq_handler);
    }
    return;
}


