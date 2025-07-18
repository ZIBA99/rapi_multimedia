#!/bin/bash

echo "[1] Building kernel module..."
sudo make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-

echo "[2] Creating device node /dev/gpioled..."
sudo mknod /dev/gpioled c 200 0

echo "[3] Changing permissions..."
sudo chmod 666 /dev/gpioled

echo "[4] Inserting kernel module..."
sudo insmod gpiofunction_module.ko

echo "✅ All steps completed."
