#!/bin/bash

MODULE_PATH="./lkm_hyp.ko"

if sudo insmod "$MODULE_PATH"; then
    echo "Module inserted successfully."
else
    echo "Failed to insert module."
    echo "Last kernel messages:"
    sudo dmesg | tail -50
fi
