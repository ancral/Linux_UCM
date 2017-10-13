#!/bin/bash

if lsmod | grep modlist > /dev/null ; then
	echo "--------------------------------------------------"
	echo "Ya existe el modlist, borrando e instalando de nuevo"
	echo "--------------------------------------------------"
	rmmod modlist
else
	echo "--------------------------------------------------"
	echo "Instalando el modulo"
	echo "--------------------------------------------------"
	
fi 	

insmod modlist.ko

for i in {1..20};
do 
	echo add $i > /proc/modlist;
done

echo "--------------------------------------------------"
echo "Añadiendo elementos: [1..20]"
echo "--------------------------------------------------"
cat /proc/modlist;

echo remove 2 > /proc/modlist;
echo remove 13 > /proc/modlist;
for i in {17..19};
do 
	echo remove $i > /proc/modlist;
done

echo "--------------------------------------------------"
echo "Eliminando los elementos: 2, 13 y [17..19]"
echo "--------------------------------------------------"
cat /proc/modlist;

echo "--------------------------------------------------"
echo "Limpiando lista"
echo "--------------------------------------------------"
echo cleanup > /proc/modlist;


echo add 1 > /proc/modlist;
echo add 29 > /proc/modlist;

echo "--------------------------------------------------"
echo "Añadiendo elementos: 1 y 29"
echo "--------------------------------------------------"
cat /proc/modlist;