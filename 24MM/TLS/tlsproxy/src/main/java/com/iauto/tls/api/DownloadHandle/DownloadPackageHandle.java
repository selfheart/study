package com.iauto.tls.api.DownloadHandle;

import static com.iauto.tls.api.common.LogDef.LOG_ID_PACKAGEDL_HANDLE;

import com.google.gson.Gson;
import com.iauto.tls.api.bean.TargetAndCustom;
import com.iauto.tls.api.common.Log;
import com.iauto.tls.api.response.TargetsMetadataAndFile;
import com.iauto.tls.api.response.VehicleVersionManifestResponse;
import com.iauto.tls.api.utils.filetools.JsonFIleUtils;

/**
 * The type Download package handle.
 */
public class DownloadPackageHandle {
    private String PkgDLFileName;
    private String PkgDLFileURL;
    private String LocalFilePAth = "  ";//TODO



    private static volatile DownloadPackageHandle instance;


    private DownloadPackageHandle(){

    }

    /**
     * Get instance download package handle.
     *
     * @return the download package handle
     */
    public static DownloadPackageHandle getInstance(){
        if(instance == null){
            synchronized (DownloadPackageHandle.class){
                if(instance == null)
                    instance = new DownloadPackageHandle();
            }
        }
        return instance;
    }

    /**
     * Update package dlurl.
     *
     * @param json     the json
     * @param filePath the file path
     */
    public void updatePackageDLURL(String json,String filePath){
        Log.d(LOG_ID_PACKAGEDL_HANDLE,"LOG_ID_PACKAGEDL_HANDLE updatePackageDLURL filepath = "+filePath);
        Gson gson = new Gson();
        VehicleVersionManifestResponse vehicleVersionManifestResponse = gson.fromJson(json,VehicleVersionManifestResponse.class);
        TargetsMetadataAndFile[] targetsMetadataAndFile = vehicleVersionManifestResponse.getTargetsMetadata();
        TargetAndCustom[] targetAndCustom = targetsMetadataAndFile[0].getMetadata().getSigned().getBody().getTargets();
        //update url
        setPkgDLFileName(targetAndCustom[0].getTarget().getFileName());
        setPkgDLFileURL(targetAndCustom[0].getTarget().getFileDownloadUrl());
        //write into file
        new JsonFIleUtils().writeObjectToJsonFile(this,filePath);
    }

    /**
     * Down load.
     */
//TODO:
    public void downLoad(){

    }


    /**
     * Get package dlurl string.
     *
     * @return the string
     */
    public String getPackageDLURL(){
        Log.d(LOG_ID_PACKAGEDL_HANDLE,"LOG_ID_PACKAGEDL_HANDLE getPackageDLURL");
        DownloadPackageHandle packageDLHandle;
        try{
            packageDLHandle = new JsonFIleUtils().readJsonFileToObject(LocalFilePAth, DownloadPackageHandle.class);
        }catch (Exception e){
            e.printStackTrace();
            return null;
        }
        return  packageDLHandle.PkgDLFileURL;
    }

    /**
     * Sets pkg dl file name.
     *
     * @param pkgDLFileName the pkg dl file name
     */
    public void setPkgDLFileName(String pkgDLFileName) {
        PkgDLFileName = pkgDLFileName;
    }

    /**
     * Sets pkg dl file url.
     *
     * @param pkgDLFileURL the pkg dl file url
     */
    public void setPkgDLFileURL(String pkgDLFileURL) {
        PkgDLFileURL = pkgDLFileURL;
    }

    /**
     * Get local file p ath string.
     *
     * @return the string
     */
    public String getLocalFilePAth(){
        Log.d(LOG_ID_PACKAGEDL_HANDLE, "LOG_ID_PACKAGEDL_HANDLE getLocalFilePAth");
        DownloadPackageHandle packageDLHandle;
        try {
            packageDLHandle = new JsonFIleUtils().readJsonFileToObject(LocalFilePAth, DownloadPackageHandle.class);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
        return packageDLHandle.LocalFilePAth;
    }

    /**
     * Get package file name string.
     *
     * @return the string
     */
    public String getPackageFileName(){
        Log.d(LOG_ID_PACKAGEDL_HANDLE, "LOG_ID_PACKAGEDL_HANDLE getLocalFilePAth");
        DownloadPackageHandle packageDLHandle;
        try {
            packageDLHandle = new JsonFIleUtils().readJsonFileToObject(LocalFilePAth, DownloadPackageHandle.class);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
        return packageDLHandle.PkgDLFileName;
    }
}
