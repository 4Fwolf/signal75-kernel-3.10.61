#include <generated/autoconf.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/aee.h>
#include <linux/xlog.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/earlysuspend.h>
#include <linux/seq_file.h>

#include <asm/uaccess.h>

#include <mach/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mach/mt_pm_ldo.h>
#include <mach/eint.h>
#include <mach/mt_pmic_wrap.h>
#include <mach/mt_gpio.h>
#include <mach/mtk_rtc.h>
#include <mach/mt_spm_mtcmos.h>

#include <mach/battery_common.h>
#include <linux/time.h>
//#include <mach/pmic_mt6328_sw.h>

#include <cust_pmic.h>
#include <cust_battery_meter.h>


//==============================================================================
// Extern
//==============================================================================
extern kal_uint16 pmic_set_register_value(PMU_FLAGS_LIST_ENUM flagname,kal_uint32 val);
extern kal_uint16 pmic_get_register_value(PMU_FLAGS_LIST_ENUM flagname);
extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);
extern void upmu_set_reg_value(kal_uint32 reg, kal_uint32 reg_val);

//==============================================================================
// PMIC-AUXADC related define
//==============================================================================
#define VOLTAGE_FULL_RANGE     	1800
#define VOLTAGE_FULL_RANGE_6311	3200
#define ADC_PRECISE		32768 	// 15 bits
#define ADC_PRECISE_CH7		131072 	// 17 bits
#define ADC_PRECISE_6311	4096 	// 12 bits

//==============================================================================
// PMIC-AUXADC global variable
//==============================================================================

#define PMICTAG                "[Auxadc] "
#define PMICLOG(fmt, arg...)   printk(PMICTAG fmt,##arg)

kal_int32 count_time_out=15;
struct wake_lock pmicAuxadc_irq_lock;
//static DEFINE_SPINLOCK(pmic_adc_lock);
static DEFINE_MUTEX(pmic_adc_mutex);

static DEFINE_SPINLOCK(pmic_adc_lock);

void pmic_auxadc_init(void)
{
	kal_int32 adc_busy;
	wake_lock_init(&pmicAuxadc_irq_lock, WAKE_LOCK_SUSPEND, "pmicAuxadc irq wakelock");
	PMICLOG("****[pmic_auxadc_init] DONE\n");
}


kal_uint32  pmic_is_auxadc_busy(void)
{
	kal_uint32 ret=0;
	kal_uint32 int_status_val_0=0;
	ret=pmic_read_interface(0x73a,(&int_status_val_0),0x7FFF,0x1);
	return int_status_val_0;
}

void PMIC_IMM_PollingAuxadcChannel(void)
{
	 //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[PMIC_IMM_PollingAuxadcChannel] before:%d ",upmu_get_rg_adc_deci_gdly());


	if (pmic_get_register_value(PMIC_RG_ADC_DECI_GDLY)==1)
	{
		while(pmic_get_register_value(PMIC_RG_ADC_DECI_GDLY)==1)
		{
			unsigned long flags;
			spin_lock_irqsave(&pmic_adc_lock, flags);
			if (pmic_is_auxadc_busy()==0)
			{
				pmic_set_register_value(PMIC_RG_ADC_DECI_GDLY,0);
			}
			spin_unlock_irqrestore(&pmic_adc_lock, flags);
		}
	}
	//xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[PMIC_IMM_PollingAuxadcChannel] after:%d ",upmu_get_rg_adc_deci_gdly());
}


//==============================================================================
// PMIC-AUXADC 
//==============================================================================
kal_uint32 PMIC_IMM_GetOneChannelValue(mt6350_adc_ch_list_enum dwChannel, int deCount, int trimd)
{
	kal_int32 ret=0;
	kal_int32 ret_data;    
	kal_int32 r_val_temp=0;   
	kal_int32 adc_result=0;   
	int count=0;
	kal_uint32 busy;
       kal_int32 adc_reg_val=0;
	/*
        0 : BATON2 **
        1 : CH6
        2 : THR SENSE2 **
        3 : THR SENSE1
        4 : VCDT
        5 : BATON1  
        6 : ISENSE
        7 : BATSNS
        8 : ACCDET    
        9-16 : audio
	*/

	//do not suppport BATON2 and THR SENSE2 for sw workaround
	if (dwChannel==0 || dwChannel==2)
		return 0;

	if(dwChannel>15)
		return -1;

	PMIC_IMM_PollingAuxadcChannel();


	wake_lock(&pmicAuxadc_irq_lock);

	if (dwChannel<9)
	{
           pmic_set_register_value(PMIC_RG_VBUF_EN,1);

	    //set 0
	    ret=pmic_read_interface(MT6350_AUXADC_CON22,&adc_reg_val,MT6350_PMIC_RG_AP_RQST_LIST_MASK,MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
	    adc_reg_val = adc_reg_val & (~(1<<dwChannel));
	    ret=pmic_config_interface(MT6350_AUXADC_CON22,adc_reg_val,MT6350_PMIC_RG_AP_RQST_LIST_MASK,MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);

	    //set 1
	    ret=pmic_read_interface(MT6350_AUXADC_CON22,&adc_reg_val,MT6350_PMIC_RG_AP_RQST_LIST_MASK,MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
	    adc_reg_val = adc_reg_val | (1<<dwChannel);
	    ret=pmic_config_interface(MT6350_AUXADC_CON22,adc_reg_val,MT6350_PMIC_RG_AP_RQST_LIST_MASK,MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
	}
	else if(dwChannel>=9 && dwChannel<=16)
	{
		ret=pmic_read_interface(MT6350_AUXADC_CON23,&adc_reg_val,MT6350_PMIC_RG_AP_RQST_LIST_RSV_MASK,MT6350_PMIC_RG_AP_RQST_LIST_RSV_SHIFT);
		adc_reg_val = adc_reg_val & (~(1<<(dwChannel-9)));
		ret=pmic_config_interface(MT6350_AUXADC_CON23,adc_reg_val,MT6350_PMIC_RG_AP_RQST_LIST_RSV_MASK,MT6350_PMIC_RG_AP_RQST_LIST_RSV_SHIFT);

		//set 1
		ret=pmic_read_interface(MT6350_AUXADC_CON23,&adc_reg_val,MT6350_PMIC_RG_AP_RQST_LIST_RSV_MASK,MT6350_PMIC_RG_AP_RQST_LIST_RSV_SHIFT);
		adc_reg_val = adc_reg_val | (1<<(dwChannel-9));
		ret=pmic_config_interface(MT6350_AUXADC_CON23,adc_reg_val,MT6350_PMIC_RG_AP_RQST_LIST_RSV_MASK,MT6350_PMIC_RG_AP_RQST_LIST_RSV_SHIFT);		
	}

	switch(dwChannel){         
	case 0:    
		while(pmic_get_register_value(PMIC_RG_ADC_RDY_BATON2) != 1 )
		{
			msleep(1);
			if( (count++) > count_time_out)
			{
			PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
			break;
			}			
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_BATON2); 	
	break;
	case 1:    
		while(pmic_get_register_value(PMIC_RG_ADC_RDY_CH6) != 1 )
		{
			msleep(1);
			if( (count++) > count_time_out)
			{
			PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
			break;
			}			
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_CH6);		
	break;
	case 2:    
		while(pmic_get_register_value(PMIC_RG_ADC_RDY_THR_SENSE2) != 1 )
		{
			msleep(1);
			if( (count++) > count_time_out)
			{
			PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
			break;
			}			
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_THR_SENSE2);				
	break;
	case 3:    
		while(pmic_get_register_value(PMIC_RG_ADC_RDY_THR_SENSE1) != 1 )
		{
			msleep(1);
			if( (count++) > count_time_out)
			{
			PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
			break;
			}			
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_THR_SENSE1);			
	break;
	case 4:    
		while(pmic_get_register_value(PMIC_RG_ADC_RDY_VCDT) != 1 )
		{
			msleep(1);
			if( (count++) > count_time_out)
			{
			PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
			break;
			}			
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_VCDT);				
	break;
	case 5:    
		while(pmic_get_register_value(PMIC_RG_ADC_RDY_BATON1) != 1 )
		{
			msleep(1);
			if( (count++) > count_time_out)
			{
			PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
			break;
			}			
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_BATON1);			
	break;
	case 6:    
		while(pmic_get_register_value(PMIC_RG_ADC_RDY_ISENSE) != 1 )
		{
			msleep(1);
			if( (count++) > count_time_out)
			{
			PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
			break;
			}			
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_ISENSE);				
	break;
	case 7:    
		while(pmic_get_register_value(PMIC_RG_ADC_RDY_BATSNS) != 1 )
		{
			msleep(1);
			if( (count++) > count_time_out)
			{
			PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
			break;
			}			
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_BATSNS);				
	break;
	case 8:    
		while(pmic_get_register_value(PMIC_RG_ADC_RDY_CH5) != 1 )
		{
			msleep(1);
			if( (count++) > count_time_out)
			{
			PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
			break;
			}			
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_CH5);				
	break;
	case 9:    
	case 10: 
	case 11: 		
	case 12:    
	case 13:    
	case 14:    
	case 15:
	case 16: 		
		while(pmic_get_register_value(PMIC_RG_ADC_RDY_INT) != 1 )
		{
			msleep(1);
			if( (count++) > count_time_out)
			{
			PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
			break;
			}			
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_INT);				
	break;
	

	default:
		PMICLOG("[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
		wake_unlock(&pmicAuxadc_irq_lock);
	return -1;
	break;
	}

    switch(dwChannel){         
        case 0:                
            r_val_temp = 1;           
            adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
            break;
        case 1:    
            r_val_temp = 1;
            adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
            break;
        case 2:    
            r_val_temp = 1;
            adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
            break;
        case 3:    
            r_val_temp = 1;
            adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
            break;
        case 4:    
            r_val_temp = 1;
            adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
            break;
        case 5:    
            r_val_temp = 1;
            adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
            break;
        case 6:    
            r_val_temp = 4;
            adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
            break;
        case 7:    
            r_val_temp = 4;
            adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
            break;    
        case 8:    
            r_val_temp = 1;
            adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
            break;    			
	case 9:    
	case 10:  
	case 11:  
	case 12:
	case 13:    
	case 14:  
	case 15:  
	case 16:		
            r_val_temp = 1;
            adc_result = (ret_data*r_val_temp*VOLTAGE_FULL_RANGE)/32768;
            break;  
        default:
            PMICLOG("[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
            wake_unlock(&pmicAuxadc_irq_lock);
            return -1;
            break;
    }

	wake_unlock(&pmicAuxadc_irq_lock);

       PMICLOG("[AUXADC] ch=%d raw=%d data=%d \n", dwChannel, ret_data,adc_result);

	//return ret_data;
	return adc_result;

}

