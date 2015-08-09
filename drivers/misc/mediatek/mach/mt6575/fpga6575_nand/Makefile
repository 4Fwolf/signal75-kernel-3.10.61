
include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifneq ($(CONFIG_ARCH_MTK_PROJECT),)
ARCH_MTK_PROJECT := $(shell echo $(CONFIG_ARCH_MTK_PROJECT))
obj-y   += core/
#obj-y   += dct/dct/
endif

ifneq ($(CONFIG_ARM64), y)
#obj-y   +=  camera/camera/
endif
#obj-y   +=  flashlight/constant_flashlight/
#obj-y   +=  vibrator/
#obj-y   +=  leds/
#obj-y   +=  keypad/
#obj-y   +=  sound/
#obj-y   +=  accdet/

