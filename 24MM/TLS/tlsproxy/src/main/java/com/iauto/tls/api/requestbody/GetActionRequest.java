package com.iauto.tls.api.requestbody;

import com.iauto.tls.api.bean.GetActionRequestSigned;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Get action request.
 */
public class GetActionRequest {
    private GetActionRequestSigned signed;
    private Signature[] signatures;

    /**
     * Gets signed.
     *
     * @return the signed
     */
    public GetActionRequestSigned getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(GetActionRequestSigned signed) {
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
