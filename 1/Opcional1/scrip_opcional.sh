#!/bin/bash



normal() {
	make clean > /dev/null
	make > /dev/null

	echo "///// PARTE NORMAL, SCRIPT CON ENTEROS /////"
	
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
}

opcional() {
	make clean > /dev/null
	make EXTRA_CFLAGS=-DPARTE_OPCIONAL > /dev/null

	echo "///// PARTE OPCIONAL, SCRIPT CON CADENA DE CARACTERES /////"

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

	echo add abc > /proc/modlist
	echo add ddd > /proc/modlist
	echo add abc > /proc/modlist
	echo add cdddd > /proc/modlist
	echo add ap > /proc/modlist

	echo "--------------------------------------------------------------------------------"
	echo "Añadiendo elementos de cadena de caracteres: 'abc', 'ddd', 'abc', 'cdddd' y 'ap'"
	echo "--------------------------------------------------------------------------------"
	cat /proc/modlist;

	echo remove abc > /proc/modlist;

	echo "--------------------------------------------------"
	echo "Eliminando la cadena: abc"
	echo "--------------------------------------------------"
	cat /proc/modlist;

	echo "--------------------------------------------------"
	echo "Limpiando lista"
	echo "--------------------------------------------------"
	echo cleanup > /proc/modlist;


	echo add xXcc > /proc/modlist;
	echo add hjJjaa > /proc/modlist;

	echo "--------------------------------------------------------------"
	echo "Añadiendo elementos de cadena de caracteres: 'xXcc' y 'hjJjaa'"
	echo "--------------------------------------------------------------"
	cat /proc/modlist;

}


case $1 in
  -o | --opcional)
    opcional
    ;;
  -n | --normal)
    normal
    ;;
  *)
    echo "Use los argumentos -o (--opcional) para ejecutar la parte opcional, y -n (--normal) para ejecutar la parte base"
    echo ""
    echo "Ejemplo: 'sh script_opcional.sh (o ./script_opcional si le damos permisos) -o'"
    echo "Para que compile y ejecute las funciones del modulo de la parte opcional"
    ;;
esac
