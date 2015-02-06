fastboot oem flash_ram 0x1000a000
sleep 1
fastboot flash ram out/boot.bin
sleep 1
fastboot oem boot_ram

