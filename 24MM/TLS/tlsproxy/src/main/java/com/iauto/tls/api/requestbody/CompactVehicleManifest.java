package com.iauto.tls.api.requestbody;

import com.iauto.tls.api.bean.CompactVehicleManifestSigned;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Compact vehicle manifest.
 */
public class CompactVehicleManifest {
    private CompactVehicleManifestSigned signed;
    private Signature[] signatures;

    /**
     * Gets signed.
     *
     * @return the signed
     */
    public CompactVehicleManifestSigned getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(CompactVehicleManifestSigned signed) {
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
