package com.iauto.tls.api.requestbody;

import com.iauto.tls.api.bean.GetCommandSigned;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Common metadata.
 */
public class CommonMetadata {
    private GetCommandSigned signed;
    private Signature[] signatures;

    /**
     * Gets signed.
     *
     * @return the signed
     */
    public GetCommandSigned getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(GetCommandSigned signed) {
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
