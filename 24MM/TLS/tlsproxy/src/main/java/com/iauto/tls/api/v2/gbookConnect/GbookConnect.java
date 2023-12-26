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

package com.iauto.tls.api.v2.gbookConnect;

import static com.iauto.tls.api.common.Constants.PACKAGE_NAME;
import static com.iauto.tls.api.common.LogDef.LOG_ID_GBOOK_CONNECT;
import static com.iauto.tls.api.common.LogDef.LOG_ID_OTA_PROXY;
import static com.iauto.tls.api.common.LogDef.LOG_ID_PARSE_REQUEST_MANAGER;

import android.content.Context;

import com.iauto.gbookapi.GBookListener;
import com.iauto.gbookapi.GBookManager;
import com.iauto.gbookapi.GBookServiceManager;
import com.iauto.gbookapi.IGBookDef;
import com.iauto.gbookapi.TSConnectHttpCancelIn;
import com.iauto.gbookapi.TSConnectHttpCancelOut;
import com.iauto.gbookapi.TSConnectHttpProgress;
import com.iauto.gbookapi.TSConnectHttpRequestIn;
import com.iauto.gbookapi.TSConnectHttpRequestOut;
import com.iauto.tls.api.common.Log;
import com.iauto.tls.api.v2.otaCommunication.OtaClientProxy;
import com.iauto.tls.api.v2.otaCommunication.ResponseManager;

/**
 * The type Gbook connect.
 */
public class GbookConnect {
    private volatile static GbookConnect gbookConnect;
    private GBookManager mGbookManager = null;
    private GBookServiceManager mGBookServiceManager;
    private GBookManager mGBookManager;
    private boolean GbookConnectFlag = false;


    /**
     * 获取Model单例对象
     *
     * @return model单例对象 instance
     */
    public static GbookConnect getInstance() {
        Log.d(LOG_ID_GBOOK_CONNECT, "GbookConnect getInstance");
        if (null == gbookConnect) {
            synchronized (GbookConnect.class){
                if(null == gbookConnect){
                    Log.i(LOG_ID_GBOOK_CONNECT, "null!!!");
                    gbookConnect = new GbookConnect();
                }

            }
        }
        return gbookConnect;
    }

    private GbookConnect() {
    }

    /**
     * Init.
     *
     * @param mContext the m context
     */
    public void init(Context mContext) {
        mGBookServiceManager = GBookServiceManager.createGBookServiceManager(mContext, new GBookServiceManager.ServiceConnectionListener() {
            @Override
            public void onServiceConnected() {
                Log.d(LOG_ID_GBOOK_CONNECT, "GBookService connected");
                if (mGBookServiceManager.isConnected()) {
                    try {
                        mGBookManager = mGBookServiceManager.getGBookManager();
                        Log.d(LOG_ID_GBOOK_CONNECT, "onServiceConnected get GBookManager: " + (mGBookManager != null));
                        if (mGBookManager != null) {
                            mGBookManager.registerListener(mGBookListener);
                        }
                        GbookConnectFlag = true;
                    } catch (IllegalStateException e) {
                        Log.e(LOG_ID_GBOOK_CONNECT, "onServiceConnected: getGBookManager Exception: " + e.getMessage());
                    }
                }
            }

            @Override
            public void onServiceDisconnected() {
                Log.d(LOG_ID_GBOOK_CONNECT,"onServiceDisconnected");
                GbookConnectFlag = false;
            }
        });
        try {
            mGBookServiceManager.connect();
        } catch (IllegalStateException e) {
            Log.e(LOG_ID_GBOOK_CONNECT,"GBookService: connect Exception: " + e.getMessage());
        }
    }

    private final GBookListener mGBookListener = new GBookListener() {

        @Override
        public void notifyDataPlanWarningID(int id) {
            Log.d(LOG_ID_GBOOK_CONNECT,"notifyDataPlanWarningID id = "+id);
        }

        @Override
        public void notifyActivateExhibitionModeSuccess() {
            Log.d(LOG_ID_GBOOK_CONNECT,"notifyActivateExhibitionModeSuccess");
        }

        @Override
        public void notifyConnectServiceOpenSuccess() {
            Log.d(LOG_ID_GBOOK_CONNECT,"notifyConnectServiceOpenSuccess");
        }

        @Override
        public void notifyGBookFlowFinished() {
            Log.d(LOG_ID_GBOOK_CONNECT,"notifyGBookFlowFinished");
        }

        @Override
        public void notifyClientAuthFailure() {
            Log.d(LOG_ID_GBOOK_CONNECT,"notifyClientAuthFailure");
        }

        @Override
        public void notifyContractUpdateDisagreeFinished() {
            Log.d(LOG_ID_GBOOK_CONNECT,"notifyContractUpdateDisagreeFinished");
        }

        @Override
        public void notifyExhibitionModeStatus(int value) {
            Log.d(LOG_ID_GBOOK_CONNECT,"notifyExhibitionModeStatus value = " + value);
        }

        @Override
        public void notifyPrivacyVersion(String version) {
            Log.d(LOG_ID_GBOOK_CONNECT,"notifyPrivacyVersion value = " + version);
        }

        @Override
        public void onReplySvrHttp(TSConnectHttpRequestOut rep) {
            Log.d(LOG_ID_GBOOK_CONNECT,"onReplySvrHttp");
            ResponseManager.getInstance().handleResponse(rep);
        }
        @Override
        public void onReplySvrHttpCancel(TSConnectHttpCancelOut rep) {
            Log.d(LOG_ID_GBOOK_CONNECT,"onReplySvrHttpCancel");
        }
        @Override
        public void onHttpProgressCallback(TSConnectHttpProgress rep) {
            Log.d(LOG_ID_GBOOK_CONNECT,"onHttpProgressCallback");
        }
        @Override
        public void notifyServiceFlagChanged() {
            Log.d(LOG_ID_GBOOK_CONNECT,"notifyServiceFlagChanged");
        }
        @Override
        public void notifyContractUpdateFailure() {
            Log.d(LOG_ID_GBOOK_CONNECT,"notifyContractUpdateFailure");
        }

    };

    /**
     * Request svr http.
     *
     * @param req the req
     */
    public void requestSvrHttp(TSConnectHttpRequestIn req) {
        if(!GbookConnectFlag){
            Log.d(LOG_ID_GBOOK_CONNECT, "GbookConnectFlag = false");
            return;
        }
        if(!mGbookManager.requestSvrHttp(req)){
            Log.e(LOG_ID_GBOOK_CONNECT,"requestSvrHttp failed!");
        }
    }

    /**
     * Request svr http cancel.
     *
     * @param req the req
     */
    public void requestSvrHttpCancel(TSConnectHttpCancelIn req) {
        if(!GbookConnectFlag){
            Log.d(LOG_ID_GBOOK_CONNECT, "GbookConnectFlag = false");
            return;
        }
        mGbookManager.requestSvrHttpCancel(req);
    }
}
