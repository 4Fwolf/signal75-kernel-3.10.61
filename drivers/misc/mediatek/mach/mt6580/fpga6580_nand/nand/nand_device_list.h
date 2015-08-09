
#ifndef __NAND_DEVICE_LIST_H__
#define __NAND_DEVICE_LIST_H__

#define NAND_MAX_ID		4
#define CHIP_CNT		1
#define P_SIZE		16384
#define P_PER_BLK		256
#define C_SIZE		4096
#define RAMDOM_READ		(1<<0)
#define CACHE_READ		(1<<1)

#define RAND_TYPE_SAMSUNG 0
#define RAND_TYPE_TOSHIBA 1
#define RAND_TYPE_NONE 2

#define READ_RETRY_MAX 10
struct gFeature
{
	u32 address;
	u32 feature;
};

enum readRetryType
{
	RTYPE_MICRON,
	RTYPE_SANDISK,
	RTYPE_SANDISK_19NM,
	RTYPE_TOSHIBA,
	RTYPE_HYNIX,
	RTYPE_HYNIX_16NM
};

struct gFeatureSet
{
	u8 sfeatureCmd;
	u8 gfeatureCmd;
	u8 readRetryPreCmd;
	u8 readRetryCnt;
	u32 readRetryAddress;
	u32 readRetryDefault;
	u32 readRetryStart;
	enum readRetryType rtype;
	struct gFeature Interface;
	struct gFeature Async_timing;
};

struct gRandConfig
{
	u8 type;
	u32 seed[6];
};

enum pptbl
{
	MICRON_8K,
	HYNIX_8K,
	SANDISK_16K,
};

struct MLC_feature_set
{
	enum pptbl ptbl_idx;
	struct gFeatureSet 	 FeatureSet;
	struct gRandConfig   randConfig;
};

enum flashdev_vendor
{
	VEND_SAMSUNG,
	VEND_MICRON,
	VEND_TOSHIBA,
	VEND_HYNIX,
	VEND_SANDISK,
	VEND_BIWIN,
	VEND_NONE,
};

enum flashdev_IOWidth
{
	IO_8BIT = 8,
	IO_16BIT = 16,
	IO_TOGGLEDDR = 9,
	IO_TOGGLESDR = 10,
	IO_ONFI = 12,
};

typedef struct
{
   u8 id[NAND_MAX_ID];
   u8 id_length;
   u8 addr_cycle;
   enum flashdev_IOWidth iowidth;
   u16 totalsize;
   u16 blocksize;
   u16 pagesize;
   u16 sparesize;
   u32 timmingsetting;
   u32 s_acccon;
   u32 s_acccon1;
   u32 freq;
   enum flashdev_vendor vendor;
   u16 sectorsize;
   u8 devciename[30];
   u32 advancedmode;
   struct MLC_feature_set feature_set;
}flashdev_info,*pflashdev_info;

static const flashdev_info gen_FlashTable[]={
	{{0x98,0xD7,0x84,0x93}, 4,5,IO_8BIT,4096,4096,16384,1280,0x31C083F9, 0xFFFFFFFF,0xFFFFFFFF,80,VEND_TOSHIBA,1024, "TC58TEG5DCJTA00 ",0 ,
		{SANDISK_16K, {0x55,0xEE,0xFF,7,0xFF,0,0,RTYPE_TOSHIBA,{0xFF, 0xFF},{0xFF, 0xFF}},
		{RAND_TYPE_SAMSUNG,{0x2D2D,1,1,1,1,1}}}},
};

#endif
