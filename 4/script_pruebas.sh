#!/bin/bash

echo add 8 > /proc/modlist &
echo add 7 > /proc/modlist & 
cat /proc/modlist &
echo remove 8  > /proc/modlist &
echo add 6 > /proc/modlist &
wait 
echo "-------"
cat /proc/modlist

