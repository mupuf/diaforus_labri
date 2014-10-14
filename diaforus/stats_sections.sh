#!/bin/bash

flash_max="0x20000"
iram_max="0x9c00"
iramd_max="0x2400"
dram_max="0x1000"

flash="0x`readelf -S $1 | grep \".text \" | cut -d ' ' -f 28`"
iramd="0x`readelf -S $1 | grep \".slowdata \" | cut -d ' ' -f 24`"
iram="0x`readelf -S $1 | grep .text.fastcode | cut -d ' ' -f 19`"
dram="0x`readelf -S $1 | grep \" .data \" | cut -d ' ' -f 28`"

flash_usage=$(( $flash * 100 / $flash_max  ))
iramd_usage=$(( $iramd * 100 / $iramd_max  ))
iram_usage=$(( $iram * 100 / $iram_max  ))
dram_usage=$(( $dram * 100 / $dram_max  ))

echo "flash usage: $flash_usage% ($flash/$flash_max)"
echo "iramd usage: $iramd_usage% ($iramd/$iramd_max"
echo "iram usage: $iram_usage% ($iram/$iram_max)"
echo "dram usage: $dram_usage% ($dram/$dram_max)"
