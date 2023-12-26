package com.iauto.tls.api.response;

import com.iauto.tls.api.bean.RootSignedMetadata;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Root common metadata.
 */
public class RootCommonMetadata {
    private RootSignedMetadata signed;
    /**
     * 署名部
     */
    private Signature[] signatures;
}
