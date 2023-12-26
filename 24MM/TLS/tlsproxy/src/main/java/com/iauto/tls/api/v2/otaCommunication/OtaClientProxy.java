/**
 * Copyright @ 2023 - 2025 iAUTO(Shanghai) Co., Ltd.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are NOT permitted except as agreed by
 * iAUTO(Shanghai) Co., Ltd.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

package com.iauto.tls.api.v2.otaCommunication;

import static com.iauto.tls.api.common.LogDef.LOG_ID_OTA_PROXY;

import android.content.Context;

import com.iauto.tls.api.common.Log;
import com.iauto.tls.api.requestbody.CommonMetadata;
import com.iauto.tls.api.requestbody.CompactVehicleManifest;
import com.iauto.tls.api.requestbody.GetActionRequest;
import com.iauto.tls.api.requestbody.GetCampaignStatus;
import com.iauto.tls.api.requestbody.LightVehicleVersionManifest;
import com.iauto.tls.api.requestbody.MultiPartCompleteUploadRequestMetadata;
import com.iauto.tls.api.requestbody.VsmDeviceEvents;
import com.iauto.tls.api.v2.gbookConnect.GbookConnect;

import java.util.concurrent.CompletableFuture;

/**
 * The type Ota client proxy.
 */
public class OtaClientProxy {
    private static OtaClientProxy otaClientProxy;
    

    private static String mTemLocalPath;//client运行过程中存储文件路径

    private String userAgent;
    private String vin;
    private String date;


    /**
     *
     * @return  instance
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
        
    }

    /**
     * Initialize.
     *
     * @param temLocalPath   the tem local path
     * @param serviceContext the service context
     */
    public void initialize(String temLocalPath, Context serviceContext){
        mTemLocalPath = temLocalPath;
        GbookConnect.getInstance().init(serviceContext);
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
    public void campaignStatusNotification(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
        RequestManager.getInstance().sendRequest("campaignStatusNotification", vin, date, request, futureResult);//TODO: otasu40-centerconnect-rd000-002-a.pdf文档无服务器的response说明，但是詳細シーケンス（AP2_CP2_CP1面_CP1+extFlashROM）-v.1.5.0.en.xlsx有服务器response返回的箭头指向。
    }

    /**
     * (C_03_Req)OTA Action acquisition
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public void otaActionAcquisition(GetActionRequest request, CompletableFuture<Object> futureResult) {
        RequestManager.getInstance().sendRequest("otaActionAcquisition", vin, date, request, futureResult);
    }

    /**
     * (C_04_Req)Vehicle configuration information digest
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void vehicleConfigurationInformationDigest(CompactVehicleManifest request, CompletableFuture<Object> futureResult) {
        RequestManager.getInstance().sendRequest("vehicleConfigurationInformationDigest", vin, date, request,futureResult);
        //存储配置信息dlPkgURL等，后续请求会用到

    }

    /**
     * (C_05_Req)Vehicle configuration information upload
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void vehicleConfigurationInformationUpload(LightVehicleVersionManifest request, CompletableFuture<Object> futureResult) {
       RequestManager.getInstance().sendRequest("vehicleConfigurationInformationUpload", vin, date, request,futureResult);
    }

    /**@deprecated
     * (C_06_Req)Repro policy META Data acquisition request【19PFv3 not applicable】
     */
//   public  void reproPolicyMetaDataAcquisitionRequest() {
////         String responseBodyJson=RequestManager.getInstance().sendRequest( vin, date, request,"reproPolicyMetaDataAcquisitionRequest");
//         return "responseBodyJson";
//
//    }

    /**@deprecated
     * (C_07_Req)Download META Data acquisition request【19PFv3 not applicable】
     */
//   public  void downloadMetaDataAcquisitionRequest() {
////         String responseBodyJson=RequestManager.getInstance().sendRequest( vin, date, request);
//         return "";
//    }

    /**
     * (C_08_Req)HMI Data acquisition
     *
     * @param futureResult the future result
     */
    public  void hmiDataAcquisition(CompletableFuture<Object> futureResult) {
       RequestManager.getInstance().sendRequest("hmiDataAcquisition", vin, date, "nothing", futureResult);//TODO:服务器返回的responseJson含有HMI文件下载的URL，需要取出来去下载HMI
    }

    /**@deprecated
     * (C_09_Req)Download tracking notification【19PFv3 not applicable】
     */
//   public  void downloadTrackingNotification(VsmDeviceEvents request) {
//         String responseBodyJson=RequestManager.getInstance().sendRequest(userAgent, vin, date, request,"downloadTrackingNotification");
//         return responseBodyJson;
//    }

    /**
     * (C_10_Req)User Acceptance Result Notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void userAcceptanceResultNotification(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("userAcceptanceResultNotification", vin, date, request, futureResult);
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
    public  void downloadAcceptanceResultNotification(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("downloadAcceptanceResultNotification", vin, date, request, futureResult);
    }

    /**
     * (C_13_Req)Download start notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void downloadStartNotification(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("downloadStartNotification", vin, date, request, futureResult);
    }

    /**
     * (C_14_Req)SoftwarePackage DL
     *
     * @param SoftwarePackageURL the software package url
     * @param futureResult       the future result
     */
    public void softwarePackageDL(String SoftwarePackageURL, CompletableFuture<Object> futureResult) {
        RequestManager.getInstance().sendRequest("softwarePackageDL", vin, date,  SoftwarePackageURL, futureResult);
    }

    /**
     * (C_15_Req)Download progress
     * TODO: 把我们计算出的进度通知给服务器
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void downloadProgress(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
       RequestManager.getInstance().sendRequest("downloadProgress", vin, date, request,  futureResult);
    }

    /**
     * (C_16_Req)Download completion notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void downloadCompletionNotification(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("downloadCompletionNotification", vin, date, request, futureResult);
    }

    /**
     * (C_17_Req)Download completion confirmation result notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void downloadCompletionConfirmationResultNotification(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("downloadCompletionConfirmationResultNotification", vin, date, request, futureResult);
    }

    /**
     * (C_18_Req)Campaign validity check request
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void campaignValidityCheckRequest(GetCampaignStatus request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("campaignValidityCheckRequest", vin, date, request, futureResult);
    }

    /**
     * (C_19_Req)Install Acceptance Result Notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void installAcceptanceResultNotification(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("installAcceptanceResultNotification", vin, date, request, futureResult);
    }

    /**
     * (C_20_Req)Installation progress
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void installationProgress(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
       RequestManager.getInstance().sendRequest("", vin, date, request, futureResult);
    }

    /**
     * (C_21_Req)Installation completion confirmation result notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void installationCompletionConfirmationResultNotification(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
        RequestManager.getInstance().sendRequest("installationCompletionConfirmationResultNotification", vin, date, request, futureResult);
    }

    /**
     * (C_22_Req)Notification of activation acceptance results
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void notificationOfActivationAcceptanceResults(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("notificationOfActivationAcceptanceResults", vin, date, request, futureResult);
    }

    /**
     * (C_23_Req)Activation Progress
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void activationProgress(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("activationProgress", vin, date, request, futureResult);
    }

    /**
     * (C_24_Req)Notification of activation completion confirmation result
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void notificationOfActivationCompletionConfirmationResult(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("notificationOfActivationCompletionConfirmationResult", vin, date, request, futureResult);
    }

    /**
     * (C_25_Req)Software update completion confirmation result notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void softwareUpdateCompletionConfirmationResultNotification(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("softwareUpdateCompletionConfirmationResultNotification", vin, date, request, futureResult);
    }

    /**
     * (C_28_Req)Vehicle error notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void vehicleErrorNotification(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("vehicleErrorNotification", vin, date, request, futureResult);
    }

    /**
     * (C_30_Req)Newer version confirmation (Director Root Metadata)
     *
     * @param url          the url
     * @param futureResult the future result
     */
    public  void newerVersionConfirmationDirector(String url, CompletableFuture<Object> futureResult) {
        RequestManager.getInstance().sendRequest("newerVersionConfirmationDirector", vin, date, "nothing", futureResult);
    }

    /**
     * (C_31_Req)Newer version confirmation (Image Root Metadata)
     *
     * @param futureResult the future result
     */
    public  void newerVersionConfirmationImage(CompletableFuture<Object> futureResult) {
        RequestManager.getInstance().sendRequest("newerVersionConfirmationImage", vin, date, "nothing", futureResult);
    }

    /**
     * (C_32_Req)Image Snapshot Metadata Download request
     *
     * @param futureResult the future result
     */
    public  void imageSnapshotMetadataDownloadRequest(CompletableFuture<Object> futureResult) {
        RequestManager.getInstance().sendRequest("imageSnapshotMetadataDownloadRequest", vin, date, "nothing", futureResult);
    }

    /**
     * (C_33_Req)Image Timestamp Metadata Download request
     *
     * @param futureResult the future result
     */
    public  void imageTimestampMetadataDownloadRequest(CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("imageTimestampMetadataDownloadRequest", vin, date, "nothing", futureResult);
    }

    /**
     * (C_34_Req)Image Targets Metadata Download request
     *
     * @param futureResult the future result
     */
    public  void imageTargetsMetadataDownloadRequest(CompletableFuture<Object> futureResult) {
        RequestManager.getInstance().sendRequest("imageTargetsMetadataDownloadRequest", vin, date, "nothing", futureResult);
    }

    /**
     * (C_35_Req)Software update completion notification
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void softwareUpdateCompletionNotification(VsmDeviceEvents request, CompletableFuture<Object> futureResult) {
       RequestManager.getInstance().sendRequest("softwareUpdateCompletionNotification", vin, date, request, futureResult);
    }

    /**
     * (C_36_Req)OTA Command acquisition
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void otaCommandAcquisition(CommonMetadata request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("otaCommandAcquisition", vin, date, request, futureResult);
    }

    /**
     * (C_37_Req)Vehicle Log Upload URL Acquisition
     *
     * @param futureResult the future result
     */
    public  void vehicleLogUploadUrlAcquisition(CompletableFuture<Object> futureResult) {
        RequestManager.getInstance().sendRequest("vehicleLogUploadUrlAcquisition", vin, date, "nothing", futureResult);
    }

    /**
     * (C_38_Req)Vehicle Log Upload
     *
     * @param path         the path
     * @param futureResult the future result
     */
    public  void vehicleLogUpload(String path, CompletableFuture<Object> futureResult) {
        RequestManager.getInstance().sendRequest("vehicleLogUpload", vin, date, path, futureResult);

    }

    /**
     * (C_39_Req)Vehicle Log Upload Completion Notice
     *
     * @param request      the request
     * @param futureResult the future result
     */
    public  void vehicleLogUploadCompletionNotice(MultiPartCompleteUploadRequestMetadata request, CompletableFuture<Object> futureResult) {
         RequestManager.getInstance().sendRequest("vehicleLogUploadCompletionNotice", vin, date, request, futureResult);

    }




}
