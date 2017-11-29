#!/bin/bash

n=0

while [ "$n" -ne 20 ]; do
n=$((n+1));
cat /proc/modlist;
sleep 0.00001;
echo remove "$n" > /proc/modlist;
done



