#!/bin/bash
@echo off
clear
make
sudo insmod message_slot.ko

echo ""
echo "checking that device has registered"
lsmod | grep message_slot
echo ""
sudo mknod /dev/msg0 c 240 0
sudo mknod /dev/msg5 c 240 5
sudo mknod /dev/msg2021 c 240 5
sudo chmod 666 /dev/msg*
echo ""
echo "check that files have created (msg0/5/2021)"
ls -l /dev/ | grep ' msg'
echo "============================================="

echo "Writing 'hayush' to minor: 0, channel: 605"
./sender /dev/msg0 605 hayush

echo "Writing 'bayush' to minor: 5, channel: 605"
./sender /dev/msg5 605 bayush

echo "Writing 'universe' to minor: 0, channel: 42"
./sender /dev/msg0 42  universe
echo "============================================="

echo "Reading from minor: 5, channel: 605"
RET=`./reader /dev/msg5 605`
echo "Expected: bayush, Got: ${RET}"

echo "============================================="

echo "Writing 'basic' to minor: 2021, channel: 2"
./sender /dev/msg2021 2 basic

echo "============================================="

echo "Reading from minor: 5, channel: 605"
RET=`./reader /dev/msg5 605`
echo "Expected: bayush, Got: ${RET}"

echo ""

echo "Reading from minor: 0, channel: 605"
RET=`./reader /dev/msg0 605`
echo "Expected: hayush, Got: ${RET}"

echo ""

echo "Reading from minor: 5, channel: 605"
RET=`./reader /dev/msg5 605`
echo "Expected: bayush, Got: ${RET}"

echo ""

echo "Reading from minor: 0, channel: 42"
RET=`./reader /dev/msg0 42`
echo "Expected: universe, Got: ${RET}"

echo ""

echo "Reading from minor: 2021, channel: 2"
RET=`./reader /dev/msg2021 2`
echo "Expected: basic, Got: ${RET}"