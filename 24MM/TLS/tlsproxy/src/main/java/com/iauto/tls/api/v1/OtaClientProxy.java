package com.iauto.tls.api.v1;

import static com.iauto.tls.api.common.LogDef.LOG_ID_OTA_PROXY;

import com.iauto.tls.api.common.Log;
import com.iauto.tls.api.requestbody.CommonMetadata;
import com.iauto.tls.api.requestbody.CompactVehicleManifest;
import com.iauto.tls.api.requestbody.GetActionRequest;
import com.iauto.tls.api.requestbody.GetCampaignStatus;
import com.iauto.tls.api.requestbody.LightVehicleVersionManifest;
import com.iauto.tls.api.requestbody.MultiPartCompleteUploadRequestMetadata;
import com.iauto.tls.api.requestbody.VsmDeviceEvents;

import java.io.File;
import java.util.concurrent.CompletableFuture;

import okhttp3.MediaType;
import okhttp3.RequestBody;

/**
 * The type Ota client proxy.
 */
public class OtaClientProxy {
    private static OtaClientProxy otaClientProxy;

    private RetrofitManager retrofitManager;

    private String localPath;//client运行过程中存储文件路径

    private String userAgent;
    private String vin;
    private String date;


    /**
     * 获取Model单例对象
     *
     * @return model单例对象 instance
     */
    public static OtaClientProxy getInstance() {
        Log.d(LOG_ID_OTA_PROXY, "OtaClientProxy getInstance");
        if (null == otaClientProxy) {
            Log.i(LOG_ID_OTA_PROXY, "null!!!");
            otaClientProxy = new OtaClientProxy();
        }
        return otaClientProxy;
    }

    private OtaClientProxy() {
        retrofitManager = new RetrofitManager();
    }

    /**
     * Sets user agent.
     *
     * @param userAgent the user agent
     */
    public void setUserAgent(String userAgent) {
        this.userAgent = userAgent;
    }

    /**
     * Sets vin.
     *
     * @param vin the vin
     */
    public void setVin(String vin) {
        this.vin = vin;
    }


    /**
     * (C_01_Req)Campaign Status Notification
     *
     * @param request      the request
     * @param futureResult the future result
     * @return
     */
    public void campaignStatusNotification(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
        retrofitManager.sendRequest("campaignStatusNotification", userAgent, vin, date, request, futureResult);//TODO: otasu40-centerconnect-rd000-002-a.pdf文档无服务器的response说明，但是詳細シーケンス（AP2_CP2_CP1面_CP1+extFlashROM）-v.1.5.0.en.xlsx有服务器response返回的箭头指向。
    }

    /**
     * (C_03_Req)OTA Action acquisition
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public void otaActionAcquisition(GetActionRequest request, CompletableFuture<String> futureResult) {
        retrofitManager.sendRequest("otaActionAcquisition", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_04_Req)Vehicle configuration information digest
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void vehicleConfigurationInformationDigest(CompactVehicleManifest request, CompletableFuture<String> futureResult) {
        retrofitManager.sendRequest("vehicleConfigurationInformationDigest",userAgent, vin, date, request,futureResult);
        //存储配置信息dlPkgURL等，后续请求会用到

    }

    /**
     * (C_05_Req)Vehicle configuration information upload
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void vehicleConfigurationInformationUpload(LightVehicleVersionManifest request, CompletableFuture<String> futureResult) {
       retrofitManager.sendRequest("vehicleConfigurationInformationUpload", userAgent, vin, date, request,futureResult);
    }

    /**@deprecated
     * (C_06_Req)Repro policy META Data acquisition request【19PFv3 not applicable】
     */
//   public  void reproPolicyMetaDataAcquisitionRequest() {
////         String responseBodyJson=retrofitManager.sendRequest( userAgent, vin, date, request,"reproPolicyMetaDataAcquisitionRequest");
//         return "responseBodyJson";
//
//    }

    /**@deprecated
     * (C_07_Req)Download META Data acquisition request【19PFv3 not applicable】
     */
//   public  void downloadMetaDataAcquisitionRequest() {
////         String responseBodyJson=retrofitManager.sendRequest( userAgent, vin, date, request);
//         return "";
//    }

    /**
     * (C_08_Req)HMI Data acquisition
     *
     * @param futureResult the future result
     */
    public  void hmiDataAcquisition(CompletableFuture<String> futureResult) {
       retrofitManager.sendRequest("hmiDataAcquisition", userAgent, vin, date, "nothing", futureResult);//TODO:服务器返回的responseJson含有HMI文件下载的URL，需要取出来去下载HMI
    }

    /**@deprecated
     * (C_09_Req)Download tracking notification【19PFv3 not applicable】
     */
//   public  void downloadTrackingNotification(VsmDeviceEvents request) {
//         String responseBodyJson=retrofitManager.sendRequest(userAgent, vin, date, request,"downloadTrackingNotification");
//         return responseBodyJson;
//    }

    /**
     * (C_10_Req)User Acceptance Result Notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void userAcceptanceResultNotification(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("userAcceptanceResultNotification",userAgent, vin, date, request, futureResult);
    }

    /**@ deprecated
     * (C_11_Req)VehiclePackage DL
     * (In 19PFver3, VehiclePackage is included in SoftwarePackage, so no action is required.)
     */
//   public  void vehiclePackageDL() {
//        return "responseBodyJson";
//    }

    /**
     * (C_12_Req)Download Acceptance Result Notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void downloadAcceptanceResultNotification(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("downloadAcceptanceResultNotification", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_13_Req)Download start notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void downloadStartNotification(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("downloadStartNotification", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_14_Req)SoftwarePackage DL
     *
     * @param futureResult the future result
     */
    public void softwarePackageDL(CompletableFuture<String> futureResult) {
        retrofitManager.sendRequest("softwarePackageDL", userAgent, vin, date, null, futureResult);
    }

    /**
     * (C_15_Req)Download progress
     * TODO: 把我们计算出的进度通知给服务器
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void downloadProgress(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
       retrofitManager.sendRequest("downloadProgress", userAgent, vin, date, request,  futureResult);
    }

    /**
     * (C_16_Req)Download completion notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void downloadCompletionNotification(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("downloadCompletionNotification", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_17_Req)Download completion confirmation result notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void downloadCompletionConfirmationResultNotification(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("downloadCompletionConfirmationResultNotification", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_18_Req)Campaign validity check request
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void campaignValidityCheckRequest(GetCampaignStatus request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("campaignValidityCheckRequest", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_19_Req)Install Acceptance Result Notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void installAcceptanceResultNotification(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("installAcceptanceResultNotification", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_20_Req)Installation progress
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void installationProgress(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
       retrofitManager.sendRequest("", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_21_Req)Installation completion confirmation result notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void installationCompletionConfirmationResultNotification(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
        retrofitManager.sendRequest("installationCompletionConfirmationResultNotification", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_22_Req)Notification of activation acceptance results
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void notificationOfActivationAcceptanceResults(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("notificationOfActivationAcceptanceResults", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_23_Req)Activation Progress
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void activationProgress(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("activationProgress", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_24_Req)Notification of activation completion confirmation result
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void notificationOfActivationCompletionConfirmationResult(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("notificationOfActivationCompletionConfirmationResult", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_25_Req)Software update completion confirmation result notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void softwareUpdateCompletionConfirmationResultNotification(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("softwareUpdateCompletionConfirmationResultNotification", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_28_Req)Vehicle error notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void vehicleErrorNotification(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("vehicleErrorNotification", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_30_Req)Newer version confirmation (Director Root Metadata)
     *
     * @param futureResult the future result
     */
    public  void newerVersionConfirmationDirector(CompletableFuture<String> futureResult) {
        retrofitManager.sendRequest("newerVersionConfirmationDirector", userAgent, vin, date, "nothing", futureResult);
    }

    /**
     * (C_31_Req)Newer version confirmation (Image Root Metadata)
     *
     * @param futureResult the future result
     */
    public  void newerVersionConfirmationImage(CompletableFuture<String> futureResult) {
        retrofitManager.sendRequest("newerVersionConfirmationImage", userAgent, vin, date, "nothing", futureResult);
    }

    /**
     * (C_32_Req)Image Snapshot Metadata Download request
     *
     * @param futureResult the future result
     */
    public  void imageSnapshotMetadataDownloadRequest(CompletableFuture<String> futureResult) {
        retrofitManager.sendRequest("imageSnapshotMetadataDownloadRequest", userAgent, vin, date, "nothing", futureResult);
    }

    /**
     * (C_33_Req)Image Timestamp Metadata Download request
     *
     * @param futureResult the future result
     */
    public  void imageTimestampMetadataDownloadRequest(CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("imageTimestampMetadataDownloadRequest", userAgent, vin, date, "nothing", futureResult);
    }

    /**
     * (C_34_Req)Image Targets Metadata Download request
     *
     * @param futureResult the future result
     */
    public  void imageTargetsMetadataDownloadRequest(CompletableFuture<String> futureResult) {
        retrofitManager.sendRequest("imageTargetsMetadataDownloadRequest", userAgent, vin, date, "nothing", futureResult);
    }

    /**
     * (C_35_Req)Software update completion notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void softwareUpdateCompletionNotification(VsmDeviceEvents request, CompletableFuture<String> futureResult) {
       retrofitManager.sendRequest("softwareUpdateCompletionNotification", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_36_Req)OTA Command acquisition
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void otaCommandAcquisition(CommonMetadata request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("otaCommandAcquisition", userAgent, vin, date, request, futureResult);
    }

    /**
     * (C_37_Req)Vehicle Log Upload URL Acquisition
     *
     * @param futureResult the future result
     */
    public  void vehicleLogUploadUrlAcquisition(CompletableFuture<String> futureResult) {
        retrofitManager.sendRequest("vehicleLogUploadUrlAcquisition", userAgent, vin, date, "nothing", futureResult);
    }

    /**
     * (C_38_Req)Vehicle Log Upload
     *
     * @param path         the path
     * @param futureResult the future result
     */
    public  void vehicleLogUpload(String path, CompletableFuture<String> futureResult) {
        //TODO:封装 从车辆日志上传URL获取（C_37_Req）中指定的URL进行获取。按表2-105中定义的partNumber上载文件。另外，在HTTP头中添加Content-MD5，使用表2-105中定义的md5CheckSum中输入的值进行请求
        // 创建文件RequestBody
        File file = new File(path);
        RequestBody fileRequestBody = RequestBody.create(MediaType.parse("application/octet-stream"), file);
        // 计算文件MD5散列值
        String md5Checksum = "calculateMD5Checksum(file);"; //TODO
        retrofitManager.sendRequest("vehicleLogUpload", userAgent, vin, date, fileRequestBody, futureResult);

    }

    /**
     * (C_39_Req)Vehicle Log Upload Completion Notice
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void vehicleLogUploadCompletionNotice(MultiPartCompleteUploadRequestMetadata request, CompletableFuture<String> futureResult) {
         retrofitManager.sendRequest("vehicleLogUploadCompletionNotice", userAgent, vin, date, request, futureResult);

    }

    /**
     * Gets local path.
     *
     * @return the local path
     */
    public String getLocalPath() {
        return localPath;
    }

    /**
     * Sets local path.
     *
     * @param localPath the local path
     */
    public void setLocalPath(String localPath) {
        this.localPath = localPath;
    }
}
