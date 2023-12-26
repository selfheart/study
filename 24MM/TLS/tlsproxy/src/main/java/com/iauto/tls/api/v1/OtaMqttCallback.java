package com.iauto.tls.api.v1;

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
