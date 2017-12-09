
#!/bin/bash

n=0

while [ "$n" -ne 20 ]; do
n=$((n+1));
echo add "$n" > /proc/modlist;
done

