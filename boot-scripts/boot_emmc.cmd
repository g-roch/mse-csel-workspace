setenv bootargs console=ttyS0,115200 earlyprintk root=/dev/mmcblk1p2 rootwait

fatload mmc 1 $kernel_addr_r Image
fatload mmc 1 $fdt_addr_r nanopi-neo-plus2.dtb  

booti $kernel_addr_r - $fdt_addr_r