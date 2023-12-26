package com.iauto.tls.api.response;

import com.iauto.tls.api.bean.GetActionResponseSigned;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Get action response.
 */
public class GetActionResponse {
    private GetActionResponseSigned signed;

    private Signature[] signatures;

    /**
     * Gets signed.
     *
     * @return the signed
     */
    public GetActionResponseSigned getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(GetActionResponseSigned signed) {
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
