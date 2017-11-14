op=$1

ram() {
	if [ ! lsmod | grep blinkdrv > /dev/null ] ; then
    		echo "-----------------------------"
		echo "Instalando el driver blinkdrv"
		echo "-----------------------------"
		if [ -f ../auto.sh | -f auto.sh ]; then
			if [[ $EUID -ne 0 ]] ; then
			   echo "Debes de ser root para instalar el driver" 1>&2
			   echo "-----------------------------------------"
			   exit 1
			fi
			if [ ../auto.sh > /dev/null ] ; then
				./auto.sh
			fi
		else
			echo "auto.sh no encontrado, instale manualmente el driver"
			echo "----------------------------------------------------"
			exit 1
		fi	
	fi
	
	gcc blink_user.c -o blink
	while(true); do
		sleep 1;
		res=$(free | grep Mem | awk '{print $3/$2 * 100.0}');
		./blink $res
		sleep 1;
	done

}

case $op in
	-ram)
	ram
	;;
	*)
	echo "----------------------"
	echo "Escribe ./leds.sh -ram"
	echo "----------------------"
	;;
esac
