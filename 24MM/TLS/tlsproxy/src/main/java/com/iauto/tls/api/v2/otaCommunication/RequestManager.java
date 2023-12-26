/**
 * Copyright @ 2023 - 2025 iAUTO(Shanghai) Co., Ltd.
 * All Rights Reserved.
 * <p>
 * Redistribution and use in source and binary forms, with or without
 * modification, are NOT permitted except as agreed by
 * iAUTO(Shanghai) Co., Ltd.
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

package com.iauto.tls.api.v2.otaCommunication;

import static com.iauto.tls.api.common.Constants.PACKAGE_NAME;
import static com.iauto.tls.api.common.LogDef.LOG_ID_GBOOK_CONNECT;
import static com.iauto.tls.api.common.LogDef.LOG_ID_PARSE_REQUEST_MANAGER;
import static com.iauto.tls.api.common.LogDef.LOG_ID_RETROFIT_MANAGER;

import com.google.gson.JsonObject;
import com.iauto.gbookapi.IGBookDef;
import com.iauto.gbookapi.TSConnectHttpRequestIn;
import com.iauto.tls.api.common.GlobalDef;
import com.iauto.tls.api.common.Log;
import com.iauto.tls.api.utils.ParseHelpUtils;
import com.iauto.tls.api.v2.gbookConnect.GbookConnect;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.CompletableFuture;

/**
 * The type Request manager.
 */
public class RequestManager {

    private volatile static RequestManager requestManager;
    private static int requestId = 1;
    private Map<String,RequestBean> requestBeanMap;

    /**
     * Gets instance.
     *
     * @return the instance
     */
    public static RequestManager getInstance() {
        Log.d(LOG_ID_GBOOK_CONNECT, "RequestManager getInstance");
        if(null == requestManager){
            synchronized (RequestManager.class){
                if (null == requestManager) {
                    Log.i(LOG_ID_GBOOK_CONNECT, "null!!!");
                    requestManager = new RequestManager();
                }
            }
        }
        return requestManager;
    }

    private RequestManager() {

    }


    /**
     * Send request.
     *
     * @param requestName   the request name
     * @param vin           the vin
     * @param date          the date
     * @param requestObject the request object
     * @param futureResult  the future result
     */
    public void sendRequest(String requestName, String vin, String date, Object requestObject, CompletableFuture<Object> futureResult) {
        Log.d(LOG_ID_RETROFIT_MANAGER, "sendRequest-requestName = "+requestName);
        synchronized (requestBeanMap){
            //check requestID
            if(requestBeanMap.containsKey(requestName)){
                Log.d(LOG_ID_PARSE_REQUEST_MANAGER,"requestName = "+requestName,"has been send,please wait!");
                return;
            }
            if(requestId>Integer.MAX_VALUE)
                requestId = 1;
            requestBeanMap.put(requestName,new RequestBean(++requestId,futureResult));
        }

        TSConnectHttpRequestIn reqInData = new TSConnectHttpRequestIn();
        reqInData.setConnectId(PACKAGE_NAME);
        reqInData.setReqId(requestId);
        // joint headers
        reqInData.addHttpHeaders("x-vin", vin);
        reqInData.addHttpHeaders("DATE", date);
        reqInData.addHttpHeaders("Content-Type", "application/json;charset=UTF-8");
        //reqInData.setReqType(IGBookDef.ReqType.TSCONNECT_REQ_TYPE_OTA);//TODO:等待GBook更新


        switch (requestName) {
            //post
            case "campaignStatusNotification": //(C_01_Req)Campaign Status Notification
            case "otaActionAcquisition": //(C_03_Req)OTA Action acquisition
            case "vehicleConfigurationInformationDigest": //(C_04_Req)Vehicle configuration information digest
            case "vehicleConfigurationInformationUpload": //(C_05_Req)Vehicle configuration information upload
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
            case "otaCommandAcquisition": //(C_36_Req)OTA Command acquisition
            case "vehicleLogUploadUrlAcquisition": //(C_37_Req)Vehicle Log Upload URL Acquisition
            case "vehicleLogUploadCompletionNotice": //(C_39_Req)Vehicle Log Upload Completion Notice
                String requestBody = new ParseHelpUtils().gGzipAndBase64Encode((String) requestObject);//gzip and base64
                reqInData.setReqMethod(IGBookDef.ReqMethod.TSCONNECT_REQ_METHOD_POST); // post
                reqInData.setUrl(GlobalDef.getBaseUrl());//url
                reqInData.setPosDataType(IGBookDef.PosDataType.TSCONNECT_POSDATA_BUF); // buffer
                reqInData.setResDataType(IGBookDef.ResDataType.TSCONNECT_RESDATA_BUF); // buffer
                reqInData.setPosData(requestBody.getBytes(StandardCharsets.UTF_8));
                GbookConnect.getInstance().requestSvrHttp(reqInData);//request
                break;
            //get
            case "softwarePackageDL": //(C_14_Req)SoftwarePackage DL
            case "newerVersionConfirmationDirector": //(C_30_Req)Newer version confirmation (Director Root Metadata)
            case "newerVersionConfirmationImage": //(C_31_Req)Newer version confirmation (Image Root Metadata)
            case "imageSnapshotMetadataDownloadRequest": //(C_32_Req)Image Snapshot Metadata Download request
            case "imageTimestampMetadataDownloadRequest": //(C_33_Req)Image Timestamp Metadata Download request
            case "imageTargetsMetadataDownloadRequest": //(C_34_Req)Image Targets Metadata Download request
                reqInData.setReqMethod(IGBookDef.ReqMethod.TSCONNECT_REQ_METHOD_GET); // get
                reqInData.setUrl((String) requestObject);//url
                reqInData.setResDataType(IGBookDef.ResDataType.TSCONNECT_RESDATA_FILE); // File
                GbookConnect.getInstance().requestSvrHttp(reqInData);//request
                break;
            //put
            case "vehicleLogUpload": //(C_38_Req)Vehicle Log Upload //TODO:待Gbook更新

                break;

        }

    }

    /**
     * Gets request bean map.
     *
     * @return the request bean map
     */
    public Map<String, RequestBean> getRequestBeanMap() {
        return requestBeanMap;
    }

    /**
     * Remove request.
     *
     * @param key the key
     */
    public void removeRequest(String key) {
        if(requestBeanMap.containsKey(key)){
            requestBeanMap.remove(key);
        }else {
            Log.d(LOG_ID_GBOOK_CONNECT,"key = "+key+"is not include ");
        }

    }
}
