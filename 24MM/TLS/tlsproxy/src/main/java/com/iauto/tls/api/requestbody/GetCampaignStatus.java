package com.iauto.tls.api.requestbody;

import com.iauto.tls.api.bean.GetCampaignStatusSigned;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Get campaign status.
 */
public class GetCampaignStatus {
    private GetCampaignStatusSigned signed;
    private Signature[] signatures;

    /**
     * Gets signed.
     *
     * @return the signed
     */
    public GetCampaignStatusSigned getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(GetCampaignStatusSigned signed) {
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
