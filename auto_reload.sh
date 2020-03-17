# ! /bin/sh

log_dir='./'
log_file='reload.log'
while true
do
    procnum=`ps -ef|grep "./server -d"|grep -v grep|wc -l`
    if [ $procnum -eq 0 ]; then
        ./server -d        
        echo `date +%Y-%m-%d` `date +%H:%M:%S`  "restart server" >>${log_dir}${log_file}
    fi
    sleep 5
done
