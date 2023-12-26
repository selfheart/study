package com.iauto.tls.api.response;

import com.iauto.tls.api.bean.Signature;
import com.iauto.tls.api.bean.AugmentedSignedMetadata;

/**
 * The type Augmented common metadata.
 */
public class AugmentedCommonMetadata {
    private AugmentedSignedMetadata signed;
    /**
     * 署名部
     */
    private Signature[] signatures;
}
