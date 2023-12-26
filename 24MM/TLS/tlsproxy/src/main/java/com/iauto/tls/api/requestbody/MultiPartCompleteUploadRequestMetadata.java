package com.iauto.tls.api.requestbody;

import com.iauto.tls.api.bean.MultiPartCompleteUploadRequest;
import com.iauto.tls.api.bean.MultiPartInitiateUploadRequest;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Multi part complete upload request metadata.
 */
public class MultiPartCompleteUploadRequestMetadata {
    /**
     * MultiPartCompleteUploadRequest フォーマット
     */
    private MultiPartCompleteUploadRequest signed;

    /**
     * 表 2-6 参照
     */
    private Signature[] signatures;

    /**
     * Gets signed.
     *
     * @return the signed
     */
    public MultiPartCompleteUploadRequest getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(MultiPartCompleteUploadRequest signed) {
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
