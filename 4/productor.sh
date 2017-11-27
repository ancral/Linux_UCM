#!/bin/bash

n=0

while [[ true ]]; do
 n=$((n+1))
 if  [ "$n" -ne 100 ] then;
  echo add "$n" > /proc/modlist;
  sleep 1;
 fi
done
