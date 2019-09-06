#寻找最大磁盘,判断是否大于200GB
MAX_SIZE=`df | sed -n '2,$p' | sort -n -r -k 2  | sed -n '1,1p' | awk '{print $2}'`
let "MAX_SIZE = MAX_SIZE / 1024" 
echo "max size:$MAX_SIZE MB"
if [ $MAX_SIZE -lt 204800 ]
then 
	echo 'small than 204800 MB'
	export DISK_MOUNTED=false
	exit 1
fi
echo 'bigger than 204800 MB'
DISK_MOUNTED=true

#获得最大磁盘UUID
DISK_DEV=`df | sed -n '2,$p' | sort -n -r -k 2  | sed -n '1,1p' | awk '{print $1}'`
RS_UUID=`blkid | grep $DISK_DEV | awk '{print $2}' | sed 's/UUID="//g' | sed 's/"//g'` 
echo $RS_UUID

#磁盘剩余空间监控
while : 
do
	TEMP=`df  | sed -n '2,$p' | sort -n  -k 2   | sed -n '1,1p' | awk '{print $4}'`
	let "REST_SIZE = TEMP / 1024"
	echo "$REST_SIZE MB"
	sleep 1
done