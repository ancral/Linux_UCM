op=$1

ram() {
	lsmod | grep blinkdrv > /dev/null
	if [[ $? -ne 0 ]]; then
    		echo "-----------------------------"
		echo "Instalando el driver blinkdrv"
		echo "-----------------------------"
		if [ -f ../auto.sh ]; then
			if [[ $EUID -ne 0 ]] ; then
			   echo "Debes de ser root para instalar el driver" 1>&2
			   echo "-----------------------------------------"
			   exit 1
			fi
			cd ..
			./auto.sh
			cd ParteB
		else
			echo "No se ha encontrado el auto.sh en el directorio anterior"
			echo "Instale manualmente el driver, o pongalo en ../"
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
