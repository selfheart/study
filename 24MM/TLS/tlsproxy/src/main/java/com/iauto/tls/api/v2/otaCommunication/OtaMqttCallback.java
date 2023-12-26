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

import com.iauto.tls.api.bean.PayloadInformation;

/**
 * The type Ota mqtt callback.
 */
public class OtaMqttCallback {

    /**
     * Center Push message (C_02_IND)
     *
     * @param payloadInformation CDefine payload information format of Center Push as follows. △1 Payload data encoded by Base64 is sent to the vehicle. △1
     */
    public void onCenterPushMessage(PayloadInformation payloadInformation) {

    }

    /**
     * センタエラー通知 (C_29_Res) 只有回复，没有发送，如何实现 QA TBD
     */
}
