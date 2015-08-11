include $(srctree)/drivers/misc/mediatek/Makefile.custom

vcodec_kernel_driver-objs :=      videocodec_kernel_driver.o \
                                            codec/common/val/linux/kernel/val.o 


#obj-$(CONFIG_MTK_MFLEXVIDEO_DRIVER) += vcodec_kernel_driver.o
obj-$(CONFIG_MTK_VIDEOCODEC_DRIVER) += vcodec_kernel_driver.o

#obj-m += vcodec_kernel_driver.o


ccflags-y +=                                    -I$(srctree)/drivers/misc/mediatek/videocodec/inc \
                                                -I$(srctree)/include/linux/vcodec \
                                                -I$(srctree)/drivers/misc/mediatek/videocodec/codec/common/val/linux/kernel \
                                                -I$(srctree)/drivers/misc/mediatek/videocodec/codec/common/hal/inc \
                                                -I$(srctree)/drivers/misc/mediatek/videocodec/codec/common/api/inc \
                                                -I$(srctree)/drivers/misc/mediatek/mach/mt6575/include/mach  \
-I$(srctree)/drivers/misc/mediatek/mach/mt6575/include/mach \
-I$(srctree)/drivers/misc/mediatek/mach/mt6575/include \
