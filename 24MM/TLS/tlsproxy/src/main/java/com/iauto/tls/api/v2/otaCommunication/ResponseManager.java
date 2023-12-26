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

import static com.iauto.tls.api.common.LogDef.LOG_ID_GBOOK_CONNECT;
import static com.iauto.tls.api.common.LogDef.LOG_ID_PARSE_REQUEST_MANAGER;

import com.google.gson.Gson;

import com.iauto.gbookapi.TSConnectHttpRequestOut;

import com.iauto.tls.api.common.Log;
import com.iauto.tls.api.response.GetActionResponse;
import com.iauto.tls.api.response.GetCommandResponse;
import com.iauto.tls.api.response.UploadContext;
import com.iauto.tls.api.response.VehicleVersionManifestResponse;

import java.util.Arrays;
import java.util.Map;
import java.util.concurrent.CompletableFuture;

/**
 * The type Response manager.
 */
public class ResponseManager {
    private volatile static ResponseManager responseManager;

    /**
     * Gets instance.
     *
     * @return the instance
     */
    public static ResponseManager getInstance() {
        Log.d(LOG_ID_GBOOK_CONNECT, "ResponseManager getInstance");
        if (null == responseManager) {
            synchronized (ResponseManager.class){
                if(null == responseManager){
                    Log.i(LOG_ID_GBOOK_CONNECT, "null!!!");
                    responseManager = new ResponseManager();
                }
            }
        }
        return responseManager;
    }

    private ResponseManager() {

    }

    /**
     * Handle response.
     *
     * @param rep the rep
     */
    public void handleResponse(TSConnectHttpRequestOut rep){
        //checkRequestId and getRequestName
        String requestName = getRequestName(rep);
        if(requestName.equals("ERROR")){
            return;
        }
        //check response code
        checkResponseCode(rep);
        switch (requestName) {
            //post
            case "campaignStatusNotification": //(C_01_Req)Campaign Status Notification
            case "hmiDataAcquisition":  //(C_08_Req)HMI Data acquisition
            case "userAcceptanceResultNotification": //(C_10_Req)User Acceptance Result Notification
            case "downloadAcceptanceResultNotification": //(C_12_Req)Download Acceptance Result Notification
            case "downloadStartNotification": //(C_13_Req)Download start notification
            case "downloadProgress": //(C_15_Req)Download progress
            case "downloadCompletionNotification": //(C_16_Req)Download completion notification
            case "downloadCompletionConfirmationResultNotification": //(C_17_Req)Download completion confirmation result notification
            case "campaignValidityCheckRequest": //(C_18_Req)Campaign validity check request
            case "installAcceptanceResultNotification": //(C_19_Req)Install Acceptance Result Notification
            case "installationProgress": //(C_20_Req)Installation progress
            case "installationCompletionConfirmationResultNotification": //(C_21_Req)Installation completion confirmation result notification
            case "notificationOfActivationAcceptanceResults": //(C_22_Req)Notification of activation acceptance results
            case "activationProgress": //(C_23_Req)Activation Progress
            case "notificationOfActivationCompletionConfirmationResult": //(C_24_Req)Notification of activation completion confirmation result
            case "softwareUpdateCompletionConfirmationResultNotification": //(C_25_Req)Software update completion confirmation result notification
            case "vehicleErrorNotification": //(C_28_Req)Vehicle error notification
            case "softwareUpdateCompletionNotification": //(C_35_Req)Software update completion notification
            case "vehicleLogUploadCompletionNotice": //(C_39_Req)Vehicle Log Upload Completion Notice
            //get
            case "softwarePackageDL": //(C_14_Req)SoftwarePackage DL
            case "newerVersionConfirmationDirector": //(C_30_Req)Newer version confirmation (Director Root Metadata)
            case "newerVersionConfirmationImage": //(C_31_Req)Newer version confirmation (Image Root Metadata)
            case "imageSnapshotMetadataDownloadRequest": //(C_32_Req)Image Snapshot Metadata Download request
            case "imageTimestampMetadataDownloadRequest": //(C_33_Req)Image Timestamp Metadata Download request
            case "imageTargetsMetadataDownloadRequest": //(C_34_Req)Image Targets Metadata Download request
            //put
            case "vehicleLogUpload": //(C_38_Req)Vehicle Log Upload //TODO:待Gbook更新
                handleOtherResponse(rep,requestName);
                break;
            case "otaCommandAcquisition": //(C_36_Req)OTA Command acquisition
                handleOtaCommandAcquisition(rep,requestName);
                break;
            case "vehicleLogUploadUrlAcquisition": //(C_37_Req)Vehicle Log Upload URL Acquisition
                handleVehicleLogUploadUrlAcquisition(rep,requestName);
                break;
            case "otaActionAcquisition": //(C_03_Req)OTA Action acquisition
                handleOtaActionAcquisition(rep,requestName);
                break;
            case "vehicleConfigurationInformationDigest": //(C_04_Req)Vehicle configuration information digest
            case "vehicleConfigurationInformationUpload": //(C_05_Req)Vehicle configuration information upload
                handleVehicleConfigurationInformation(rep,requestName);
                break;
        }

    }

    /**
     * Get request name string.
     *
     * @param rep the rep
     * @return the string
     */
    public String getRequestName(TSConnectHttpRequestOut rep){
        //checkRequestId
        Map<String,RequestBean> requestBeanMap = RequestManager.getInstance().getRequestBeanMap();
        if(!requestBeanMap.containsKey(rep.getConnectId())){
            Log.d(LOG_ID_PARSE_REQUEST_MANAGER,"error ConnectId = "+rep.getConnectId()+"is not natch any requestId");
        }
        RequestManager.getInstance().removeRequest(rep.getConnectId());
        //getRequestName
        String key = "ERROR" ;
        for (Map.Entry<String, RequestBean> entry : requestBeanMap.entrySet()) {
            RequestBean value = entry.getValue();
            if(value.getRequestId() == rep.getReqId()){
                key = entry.getKey();
            }
        }
        return key;
    }

    /**
     * Handle ota action acquisition.
     *
     * @param rep         the rep
     * @param requestName the request name
     */
    public void handleOtaActionAcquisition(TSConnectHttpRequestOut rep, String requestName){
        Log.d(LOG_ID_GBOOK_CONNECT,"handleOtaActionAcquisition");
        Gson gson = new Gson();
        String responseBody = Arrays.toString(rep.getResData());
        GetActionResponse getActionResponse = gson.fromJson(responseBody,GetActionResponse.class);
        CompletableFuture<Object> requestResult = RequestManager.getInstance().getRequestBeanMap().get(requestName).getFutureResult();
        requestResult.complete(getActionResponse);//callback notify
    }

    /**
     * Handle vehicle configuration information.
     *
     * @param rep         the rep
     * @param requestName the request name
     */
    public void  handleVehicleConfigurationInformation(TSConnectHttpRequestOut rep, String requestName){
        Log.d(LOG_ID_GBOOK_CONNECT,"handleVehicleConfigurationInformation");
        Gson gson = new Gson();
        String responseBody = Arrays.toString(rep.getResData());
        VehicleVersionManifestResponse vehicleVersionManifestResponse = gson.fromJson(responseBody, VehicleVersionManifestResponse.class);
        CompletableFuture<Object> requestResult = RequestManager.getInstance().getRequestBeanMap().get(requestName).getFutureResult();
        requestResult.complete(vehicleVersionManifestResponse);//callback notify
    }

    /**
     * Handle vehicle log upload url acquisition.
     *
     * @param rep         the rep
     * @param requestName the request name
     */
    public void handleVehicleLogUploadUrlAcquisition(TSConnectHttpRequestOut rep, String requestName){
        Log.d(LOG_ID_GBOOK_CONNECT,"handleVehicleLogUploadUrlAcquisition");
        Gson gson = new Gson();
        String responseBody = Arrays.toString(rep.getResData());
        UploadContext uploadContext = gson.fromJson(responseBody, UploadContext.class);
        CompletableFuture<Object> requestResult = RequestManager.getInstance().getRequestBeanMap().get(requestName).getFutureResult();
        requestResult.complete(uploadContext);//callback notify
    }

    /**
     * Handle ota command acquisition.
     *
     * @param rep         the rep
     * @param requestName the request name
     */
    public void handleOtaCommandAcquisition(TSConnectHttpRequestOut rep, String requestName){
        Log.d(LOG_ID_GBOOK_CONNECT,"handleOtaCommandAcquisition");
        Gson gson = new Gson();
        String responseBody = Arrays.toString(rep.getResData());
        GetCommandResponse getCommandResponse = gson.fromJson(responseBody, GetCommandResponse.class);
        CompletableFuture<Object> requestResult = RequestManager.getInstance().getRequestBeanMap().get(requestName).getFutureResult();
        requestResult.complete(getCommandResponse);//callback notify
    }

    /**
     * Handle other response.
     *
     * @param rep         the rep
     * @param requestName the request name
     */
    public void handleOtherResponse(TSConnectHttpRequestOut rep, String requestName){
        Log.d(LOG_ID_GBOOK_CONNECT,"handleOtherResponse");
        CompletableFuture<Object> requestResult = RequestManager.getInstance().getRequestBeanMap().get(requestName).getFutureResult();
        requestResult.complete(null);//callback notify,no need handle responseBody,responseBody maybe null TODO:tbd
    }

    /**
     * Check response code boolean.
     *
     * @param rep the rep
     * @return the boolean
     */
    public boolean checkResponseCode(TSConnectHttpRequestOut rep){
        Log.d(LOG_ID_GBOOK_CONNECT,"checkResponseCode");
        if(rep.getResCode()!=200){
            Log.d(LOG_ID_GBOOK_CONNECT,"response Code error resCode = "+rep.getResCode(),"  resDetailCode"+rep.getResCodeDetail());
            return false;
        }
        return true;
    }


}
