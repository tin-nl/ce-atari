#!/bin/sh
echo "XILINX firmware writing script."

if [ ! -f /tmp/xilinx.xsvf ]; then          # if this file doesn't exist, try to extract it from ZIP package
    if [ -f /tmp/ce_update.zip ]; then      # got the ZIP package, unzip
        unzip -o /tmp/ce_update.zip -d /tmp
    else                                    # no ZIP package? damn!
        echo "/tmp/xilinx.xsvf and /tmp/ce_update.zip don't exist, can't update!"
        exit
    fi
fi

# initialize variables
is36=0
is72=0

# test for XC9536 chip
/ce/update/flash_xilinx /ce/update/test_xc9536xl.xsvf  > /dev/null 2> /dev/null

if [ "$?" -eq "0" ]; then
    is36=1
fi

# test for XC9572 chip
/ce/update/flash_xilinx /ce/update/test_xc9572xl.xsvf  > /dev/null 2> /dev/null

if [ "$?" -eq "0" ]; then
    is72=1
fi

# no chip detected? fail, quit
if [[ "$is36" -eq "0" ]] && [[ "$is72" -eq "0" ]]; then
    echo "No Xilinx type detected - this is invalid!"
    exit
fi

# both chips detected? fail, quit
if [[ "$is36" -eq "1" ]] && [[ "$is72" -eq "1" ]]; then
    echo "Both types of Xilinx detected - this is invalid!"
    exit
fi

# for XC9536 - just burn the firmware
if [[ "$is36" -eq "1" ]]; then
    # write the XC9536 firmware
    echo "Detected XC9536 chip, will write firmware"
    /ce/update/flash_xilinx /tmp/xilinx.xsvf
    cp /tmp/xv1.txt /ce/update/xilinx_current.txt 
    rm -f /tmp/*.xsvf
    exit
fi

# for XC9572 - first check the HDD IF - if it's SCSI or ACSI
if [[ "$is72" -eq "1" ]]; then
    echo "Detected XC9572 chip, now will detect if it's ACSI or SCSI"
    out=$( /ce/app/cosmosex hwinfo )
    
    isAcsi=$( echo "$out" | grep 'ACSI' )
    isScsi=$( echo "$out" | grep 'SCSI' )
    
    # if it's ACSI version
    if [[ -n "$isAcsi" ]]; then
        echo "Detected XC9572 chip and ACSI interface, will write firmware"
        /ce/update/flash_xilinx /tmp/xlnx2a.xsvf
        cp /tmp/xv2a.txt /ce/update/xilinx_current.txt 
        rm -f /tmp/*.xsvf
        exit
    fi

    # if it's SCSI version
    if [[ -n "$isScsi" ]]; then
        echo "Detected XC9572 chip and SCSI interface, will write firmware"
        /ce/update/flash_xilinx /tmp/xlnx2s.xsvf
        cp /tmp/xv2s.txt /ce/update/xilinx_current.txt 
        rm -f /tmp/*.xsvf
        exit
    fi
    
    echo "Detected XC9572 chip but didn't write any firmware :("
fi
