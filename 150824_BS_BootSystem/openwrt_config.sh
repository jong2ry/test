
CONFIG_MK="$PWD/config.mk"


echo "--------------------------------------------------"
echo " Configure for OPENWRT Platform"
echo "--------------------------------------------------"
echo $CONFIG_MK

rm $CONFIG_MK
touch $CONFIG_MK
echo PLATFORM=openwrt >> $CONFIG_MK

