package com.iauto.tls.api.response;

import com.iauto.tls.api.bean.GetCampaignStatusResponseSigned;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Get campaign status response.
 */
public class GetCampaignStatusResponse {
    private GetCampaignStatusResponseSigned signed;

    private Signature[] signatures;

    /**
     * Gets signed.
     *
     * @return the signed
     */
    public GetCampaignStatusResponseSigned getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(GetCampaignStatusResponseSigned signed) {
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
