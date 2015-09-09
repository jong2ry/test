
CONFIG_MK="$PWD/config.mk"


echo "--------------------------------------------------"
echo " Configure for OCTEON Platform"
echo "--------------------------------------------------"
echo $CONFIG_MK

rm $CONFIG_MK
touch $CONFIG_MK
echo PLATFORM=octeon >> $CONFIG_MK

