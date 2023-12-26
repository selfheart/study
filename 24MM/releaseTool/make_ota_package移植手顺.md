<div align='center'>make_ota_package移植手顺</div>

make_ota_package移植流程：

1.在项目目录下：make otapackage 编译成功
cp /out/target/product/mt2712/obj/PACKAGING/target_files_intermediates/apoo_1s-target_files-eng.zhang/META/* fdd3_tools/make_ota_package/config/META/
    若make otapacakge失败，且仅做包失败，中间文件依然生成，则不影响META

2.
cp /out/target/product/mt2712/obj/PACKAGING/target_files_intermediates/apoo_1s-target_files-eng.zhang/SYSTEM/build.prop make_ota_package/config/


3.
cp /out/target/product/mt2712/obj/PACKAGING/target_files_intermediates/apoo_1s-target_files-eng.zhang/BOOT/RAMDISK/etc/recovery.fstab make_ota_package/config/

4. 秘钥替换（如果在编译时，指定了秘钥路径，请注意替换成对应位置的秘钥）
cp /build/target/product/security/testkey.x509.pem make_ota_package/config/
cp /build/target/product/security/testkey.pk8 make_ota_package/config/

5. 移植releasetools，对应项目目录：/build/make/tools/releasetools
    Android8 可使用Leepi
    Android9 可使用Morley android9
    新的Android版本，需要手动对比各个文件不同

6. 移植scripts，对应项目目录：/system/update_engine/scripts
    Android8 可使用Leepi
    Android9 可使用Morley android9
    新的Android版本，需要手动对比各个文件不同

7. 移植android_out
    7.1 make_ota_package/android_out/soong/host/linux-x86/bin/soong_zip 对应目录 /out/soong/host/linux-x86/bin/soong_zip
    7.2 make_ota_package/android_out/host/linux-x86  对应目录 /out/host/linux-x86/
        7.2.1 framework 对应 /out/host/linux-x86/framework
            将旧版本删除后，全部拷贝过来，因为新的Android可能会对该目录做删减
        7.2.2 lib64 对应 /out/host/linux-x86/lib64/
            将/out/host/lib64/下对应的库，拷贝过来即可，因为make_ota_package的库经过筛选
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libbase.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libbrillo.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libbrillo-stream.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libchrome.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libconscrypt_openjdk_jni.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libcrypto-host.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libc++.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libevent-host.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libext2fs-host.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/liblog.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libprotobuf-cpp-lite.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libsparse-host.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libssl-host.so .
            cp ~/500/apoo/code/out/host/linux-x86/lib64/libz-host.so .

        7.2.3 bin 对应 /out/host/linux-x86/bin
            直接替换对应文件即可
            注意：delta_generator，是update_engine中对应的做包部分，即目录：/system/update_engine/payload_generator
            如果修改了payload_generator的内容，需要make delta_generator之后，在make_ota_pacakge中替换对应文件即可

8. 配置 ab_partitions.txt 和 share_config.txt
    注意：Morley之后的项目，mode id = 机种+式样地，Leepi项目中是分开的

9. 拷贝ab面的image到IMAGES目录下

10. 修改build_prg.sh

11. 如果使用公司提供的prg文件，修改run.sh和rundiff.sh，中各个iamge对应的名称

12. 如果需要添加统计做包时间，参考morley-android9项目中的run.sh

13. 制作更新包