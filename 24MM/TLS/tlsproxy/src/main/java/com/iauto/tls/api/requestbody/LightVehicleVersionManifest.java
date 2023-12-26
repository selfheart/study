package com.iauto.tls.api.requestbody;

import com.iauto.tls.api.bean.LightVehicleVersionManifestSigned;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Light vehicle version manifest.
 */
public class LightVehicleVersionManifest {
    private LightVehicleVersionManifestSigned signed;
    private Signature[] signatures;

    /**
     * Gets signed.
     *
     * @return the signed
     */
    public LightVehicleVersionManifestSigned getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(LightVehicleVersionManifestSigned signed) {
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
