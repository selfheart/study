package com.iauto.tls.api.response;

import com.iauto.tls.api.bean.Signature;
import com.iauto.tls.api.bean.TimestampSignedMetadata;

/**
 * The type Timestamp common metadata.
 */
public class TimestampCommonMetadata {
    private TimestampSignedMetadata signed;
    /**
     * 署名部
     */
    private Signature[] signatures;
}
