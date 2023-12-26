package com.iauto.tls.api.requestbody;

import androidx.annotation.Nullable;

import com.iauto.tls.api.bean.Signature;
import com.iauto.tls.api.bean.VsmDeviceEventsSigned;

/**
 * The type Vsm device events.
 */
public class VsmDeviceEvents {
    /**
     * VsmDeviceEventsSigned
     * フォーマット(共通)参照
     */
    @Nullable
    private VsmDeviceEventsSigned signed;
    /**
     * 署名部（複数設定可能）
     */
    @Nullable
    private Signature[] signatures;

    /**
     * Instantiates a new Vsm device events.
     *
     * @param signed     the signed
     * @param signatures the signatures
     */
    public VsmDeviceEvents(@Nullable VsmDeviceEventsSigned signed, @Nullable Signature[] signatures) {
        this.signed = signed;
        this.signatures = signatures;
    }

    /**
     * Gets signed.
     *
     * @return the signed
     */
    @Nullable
    public VsmDeviceEventsSigned getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(@Nullable VsmDeviceEventsSigned signed) {
        this.signed = signed;
    }

    /**
     * Get signatures signature [ ].
     *
     * @return the signature [ ]
     */
    @Nullable
    public Signature[] getSignatures() {
        return signatures;
    }

    /**
     * Sets signatures.
     *
     * @param signatures the signatures
     */
    public void setSignatures(@Nullable Signature[] signatures) {
        this.signatures = signatures;
    }
}
