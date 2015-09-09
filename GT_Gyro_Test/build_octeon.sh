echo "//------------------------------------------- //"
echo "//-  BUILD FOR OCTEON PLATFORM                //"
echo "//------------------------------------------- //"
echo ''
make clean CROSS=/usr/local/Cavium_Networks/OCTEON-SDK-2.3/tools/bin/mips64-octeon-linux-gnu-
make CROSS=/usr/local/Cavium_Networks/OCTEON-SDK-2.3/tools/bin/mips64-octeon-linux-gnu-
