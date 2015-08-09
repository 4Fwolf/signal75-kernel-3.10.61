#!/bin/bash
# ================================Vars==================================
curdir=`pwd`
toolchaindir='./toolchain/arm-linux-androideabi-4.7/bin/arm-linux-androideabi-'
mediatek='./drivers/misc/mediatek'
kitchen='./kitchen'
outdir='./build_out'
project='signal75'
initram='BOOT-EXTRACTED_lp'
upddir='Update_lp'
kernvers='[3.10.61]'
drv='platform/mt6577/kernel/drivers/gpu'
module='pvrsrvkm.ko'
#========================================================================

# ================================Exports==================================
export ARCH=arm
export CROSS_COMPILE=~/build/toolchain/arm-linux-androideabi-4.7/bin/arm-linux-androideabi-
#export CROSS_COMPILE=~/build/toolchain/aarch64-linux-android-4.9/bin/aarch64-linux-android-
export PROJECT=signal75
export TARGET_PRODUCT=signal75
#export CONFIG_INITRAMFS_SOURCE=~/build/boot.img-ramdisk
#========================================================================

# ================================Usage==================================
usage(){
echo " "
echo "======================================"
echo "Usage: ./mk <x> <y>"
echo " "
echo "	<x>:=========="
echo "		n    -new build"
echo "		r    -rebuild"
echo "	<y>:=========="
echo "		k   -build kernel only"
echo "		bs  -build project"
echo "======================================"
echo "	[./mk c]    -clean source tree"
echo "======================================"
echo " "
}
#========================================================================

# ================================Clean==================================
clean(){
echo " "
echo "=============================================================="
echo "Cleaning build tree:"
echo "=============================================================="
echo " "
./build.sh mrproper
echo " "
echo " "
echo " "
echo "=============================================================="
echo "Cleaning done succesfully!"
echo "=============================================================="
echo " "
}
#========================================================================

# ===========================Make simple build===========================
mkbuild_simple(){
echo " "
echo "=============================================================="
echo "Starting build project"
echo "=============================================================="
echo " "

echo " "
echo "============Building:"
./build.sh release | tee "$outdir"/log.txt

#echo " "
#echo "============Copiyng kernel:"
#cp -R "$kitchen"/"$initram" "$kitchen"/BOOT-EXTRACTED
#cp -R "$curdir"/arch/arm/boot/zImage "$kitchen"/BOOT-EXTRACTED/zImage

#echo " "
#echo "============Making boot.img:"
#cp -R "$kitchen"/boot.img "$kitchen"/WORKING_kernel/boot.img
#cd "$kitchen"
#./menu

path="kernel-$kernvers-`date +%F_%H-%M`"

echo " "
echo "============Copiyng boot.img:"
mkdir -p "$outdir"/"$path"/
touch "$outdir"/"$path"/info.txt
mv -f "$outdir"/log.txt "$outdir"/"$path"/log.txt
cp -R "$curdir"/arch/arm/boot/zImage "$outdir"/"$path"/zImage

#echo " "
#echo "============Making recovery.img:"
#cp -R "$kitchen"/BOOT-EXTRACTED_rec "$kitchen"/BOOT-EXTRACTED
#cp -R "$curdir"/arch/arm/boot/zImage "$kitchen"/BOOT-EXTRACTED/zImage
#cp -R "$kitchen"/cwm.img "$kitchen"/WORKING_kernel/boot.img
#cd "$kitchen"
#./menu

#echo " "
#echo "============Copiyng recovery.img:"
#cp -R "$kitchen"/WORKING_kernel/boot.img "$outdir"/"$path"/recovery.img

echo " "
echo " "
echo " "
echo "=============================================================="
echo "Build done succesfully!"
echo "=============================================================="
echo " "
}
#========================================================================

# ===========================Make only kernel============================
mkkernel(){
echo " "
echo "=============================================================="
echo "Starting build kernel"
echo "=============================================================="
echo " "

echo " "
echo "============Building:"
make -j3 zImage | tee "$outdir"/log.txt

echo " "
echo "============Copiyng kernel:"
cp -R "$kitchen"/"$initram" "$kitchen"/BOOT-EXTRACTED
cp -R "$curdir"/arch/arm/boot/zImage "$kitchen"/BOOT-EXTRACTED/zImage

echo " "
echo "============Making boot.img:"
cp -R "$kitchen"/boot.img "$kitchen"/WORKING_kernel/boot.img
cd "$kitchen"
./menu

path="kernel_and_recovery-$kernvers-`date +%F_%H-%M`"

echo " "
echo "============Copiyng boot.img:"
mkdir -p "$outdir"/"$path"
touch "$outdir"/"$path"/info.txt
mv -f "$outdir"/log.txt "$outdir"/"$path"/log.txt
cp -R "$kitchen"/WORKING_kernel/boot.img "$outdir"/"$path"/boot.img
cp -R "$curdir"/arch/arm/boot/zImage "$outdir"/"$path"/zImage

echo " "
echo "============Making recovery.img:"
cp -R "$kitchen"/BOOT-EXTRACTED_rec "$kitchen"/BOOT-EXTRACTED
cp -R "$curdir"/arch/arm/boot/zImage "$kitchen"/BOOT-EXTRACTED/zImage
cp -R "$kitchen"/cwm.img "$kitchen"/WORKING_kernel/boot.img
cd "$kitchen"
./menu

echo " "
echo "============Copiyng recovery.img:"
cp -R "$kitchen"/WORKING_kernel/boot.img "$outdir"/"$path"/recovery.img

echo " "
echo " "
echo " "
echo "=============================================================="
echo "Build done succesfully!"
echo "=============================================================="
echo " "
}
#========================================================================

# ================================Flashing==================================
flash(){
adb push "$outdir"/"$path"/Update/Update.zip /mnt/sdcard/Update.zip
adb reboot recovery
echo "flash recovery 2?"
read -n 1
adb shell /sbin/recovery —update_package=/sdcard/Update.zip
}
#========================================================================

# ================================Main==================================
case "$1" in
	n)
		clean
		case "$2" in
			k)
				start=`date +%H-%M`
				mkkernel
				end=`date +%H-%M`
				echo "Start: $start"
				echo "End: $end"
			;;
			bs)
				start=`date +%H-%M`
				mkbuild_simple
				end=`date +%H-%M`
				echo "Start: $start"
				echo "End: $end"
				#flash
			;;
			*)
				usage
			;;
		esac
	;;
	r)
		case "$2" in
			k)
				start=`date +%H-%M`
				mkkernel
				end=`date +%H-%M`
				echo "Start: $start"
				echo "End: $end"
			;;
			bs)
				start=`date +%H-%M`
				mkbuild_simple
				end=`date +%H-%M`
				echo "Start: $start"
				echo "End: $end"
				#flash
			;; 
			*)
				usage
			;;
		esac
	;;
	c)
		clean
	;;
	*)
		usage
	;;	
esac
#========================================================================
