MAX_SIZE=`df | sed -n '2,$p' | sort -n -r -k 2  | sed -n '1,1p' | awk '{print $2}'`
DISK_DEV=`df | sed -n '2,$p' | sort -n -r -k 2  | sed -n '1,1p' | awk '{print $1}'`

let "MAX_SIZE = MAX_SIZE / 1024" 
echo "disk:$DISK_DEV,size:$MAX_SIZE MB"


if [ $MAX_SIZE -lt 10240 ]
then 
	echo 'small than 10240 MB'
	export DISK_MOUNTED=false
	exit 1
fi

echo 'bigger than 10240 MB'
DISK_MOUNTED=true

RS_UUID=`blkid | grep $DISK_DEV | awk '{print $2}' | sed 's/UUID="//g' | sed 's/"//g'` 
echo $RS_UUID



while : 
do
	TEMP=`df  | sed -n '2,$p' | sort -n  -k 2   | sed -n '1,1p' | awk '{print $4}'`
	let "REST_SIZE = TEMP / 1024"
	echo "$REST_SIZE MB"
	sleep 1
done

