op=$1

ram() {
	echo "pasa"
	while(true); do
		sleep 2;
		res=$(free | grep Mem | awk '{print $3/$2 * 100.0}');
		gcc blink_user.c -o blink
		./blink $res
		sleep 2;
	done

}

case $op in
	-ram)
	ram
	;;
	*)
	echo "-----------------"
	echo "Usa -ram..."
	echo "-----------------"
	;;
esac