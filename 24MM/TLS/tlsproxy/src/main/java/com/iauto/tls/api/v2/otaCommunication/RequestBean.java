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

import static com.iauto.tls.api.common.LogDef.LOG_ID_PARSE_REQUEST_BEAN;

import com.iauto.tls.api.common.Log;


import java.util.concurrent.CompletableFuture;

/**
 * The type Request bean.
 */
public class RequestBean {

    private int requestId;
    private CompletableFuture<Object> futureResult;

    /**
     * Instantiates a new Request bean.
     *
     * @param requestId    the request id
     * @param futureResult the future result
     */
    public RequestBean(int requestId, CompletableFuture<Object> futureResult) {
        this.requestId = requestId;
        this.futureResult = futureResult;
    }

    /**
     * Instantiates a new Request bean.
     */
    public RequestBean() {
    }

    /**
     * Gets request id.
     *
     * @return the request id
     */
    public int getRequestId() {
        return requestId;
    }

    /**
     * Sets request id.
     *
     * @param requestId the request id
     */
    public void setRequestId(int requestId) {
        this.requestId = requestId;
    }

    /**
     * Gets future result.
     *
     * @return the future result
     */
    public CompletableFuture<Object> getFutureResult() {
        return futureResult;
    }

    /**
     * Sets future result.
     *
     * @param futureResult the future result
     */
    public void setFutureResult(CompletableFuture<Object> futureResult) {
        this.futureResult = futureResult;
    }
}
