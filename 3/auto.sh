if [[ $EUID -ne 0 ]] ; then
   echo "Debes de ser root para ejecutarlo" 1>&2
   exit 1
fi

echo "Eliminando el modulo usbhid"
echo "---------------------------" 
rmmod usbhid

echo "Instalandolo de nuevo con restricciones"
echo "---------------------------------------"
modprobe usbhid quirks=0x20A0:0x41E5:0x0004

make clean > /dev/null

echo "Compilando"
echo "----------"
make > /dev/null


if lsmod | grep blinkdrv > /dev/null; then
        echo "Eliminando modulo blinkdrv"
fi

echo "Instalando el modulo blinkdrv"
echo "-----------------------------"
insmod blinkdrv.ko



