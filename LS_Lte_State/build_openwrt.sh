echo "--------------------------------------------------"
echo " Build for OPENWRT Platform"
echo "--------------------------------------------------"
make clean PLATFORM=openwrt
make PLATFORM=openwrt
