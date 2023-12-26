package com.iauto.tls.api.response;

import com.iauto.tls.api.bean.GetCommandResponseSigned;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Get command response.
 */
public class GetCommandResponse {
    private GetCommandResponseSigned signed;

    private Signature[] signatures;

    /**
     * Gets signed.
     *
     * @return the signed
     */
    public GetCommandResponseSigned getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(GetCommandResponseSigned signed) {
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
