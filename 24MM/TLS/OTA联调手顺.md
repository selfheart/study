<div align='center'>24MM OTA 联调 调试手顺</div>

### 1、release烧写地址

\\scorpio.storm\share\node37\24MM\24MM\Lexus\teslin\demo\ota_cloud_1017 该目录下烧写最新的release

[实机烧写手顺 | iAutoWiki.js](http://wikijs.ci.iauto.com/zh/home/iAutoDroid_platform/teslin_project/device_repro_manual)

### 2、**机器上设置cardata内容**

正常需要收到notifyHttpCommonHeaderReadyStatus这个通知后，或者主动取得已经ready的场合，再发起HTTP请求，不然一定会error

CHIPNO_INFO 和  IMEI改成和服务器自己设置的一致的

su
sqlite3 /extdata/data/datamanager/CarDataManagerKeep.db
update TBL_STRING_ITEMS set VALUE="01234567890123456789" where KEY="DM_BK_STRING_UUID";
update TBL_STRING_ITEMS set VALUE="24MMTEST1008" where KEY="DM_BK_STRING_CHIPNO_INFO";
update TBL_STRING_ITEMS set VALUE='{"DcmConnection":1,"IMEI":"202310081234567","Iccid":"12345678901234567890","MSISDN":"111112222233333","DcmContract":1,"DcmAdf":1,"ToyotaFlag":1,"HelpnetFlag":1}' where KEY="DM_BK_STRING_DCM_INFO";
update TBL_INTEGER_ITEMS set VALUE="2" where KEY="DM_BK_DIAG_PRE_PRO";
.q

这一套设置好，网络连接上。ACC OFF/ON就应该ready了

### 3、输出日志

su
echo 0 > /proc/sys/kernel/printk
setprop persist.log.tag.OTA-MASTER V
setprop persist.log.tag  S
setprop persist.iauto.log.switch 31
logcat -G 256m && logcat  -g
logcat -s OTA-MASTER & 



su
echo 0 > /proc/sys/kernel/printk
setprop persist.log.tag.TLSProxy  V
setprop persist.log.tag  S
setprop persist.iauto.log.switch 31
logcat -G 256m && logcat  -g
logcat -s TLSProxy & 



su
echo 0 > /proc/sys/kernel/printk
setprop persist.log.tag.TSCONNECT V
setprop persist.log.tag  S
setprop persist.iauto.log.switch 31
logcat -G 256m && logcat  -g
logcat -s TSCONNECT &

## 4、IG on 触发

IG on 触发  下载 