package com.iauto.tls.api.response;

import com.iauto.tls.api.bean.TargetsSignedMetadata;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Targets common metadata.
 */
public class TargetsCommonMetadata {
    private TargetsSignedMetadata signed;
    /**
     * 署名部
     */
    private Signature[] signatures;

    /**
     * Gets signed.
     *
     * @return the signed
     */
    public TargetsSignedMetadata getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(TargetsSignedMetadata signed) {
        this.signed = signed;
    }

    /**
     * Get signatures signature [ ].
     *
     * @return the signature [ ]
     */
    public Signature[] getSignatures() {
        return signatures;
    }

    /**
     * Sets signatures.
     *
     * @param signatures the signatures
     */
    public void setSignatures(Signature[] signatures) {
        this.signatures = signatures;
    }
}
