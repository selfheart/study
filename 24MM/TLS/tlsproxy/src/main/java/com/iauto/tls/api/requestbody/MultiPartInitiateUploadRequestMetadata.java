package com.iauto.tls.api.requestbody;

import com.iauto.tls.api.bean.MultiPartCompleteUploadRequest;
import com.iauto.tls.api.bean.MultiPartInitiateUploadRequest;
import com.iauto.tls.api.bean.Signature;

/**
 * The type Multi part initiate upload request metadata.
 */
public class MultiPartInitiateUploadRequestMetadata {

    /**
     * 表 2-104 参照
     */
    private MultiPartInitiateUploadRequest signed;

    /**
     * 表 2-6 参照
     */
    private Signature[] signatures;

    /**
     * Gets signed.
     *
     * @return the signed
     */
    public MultiPartInitiateUploadRequest getSigned() {
        return signed;
    }

    /**
     * Sets signed.
     *
     * @param signed the signed
     */
    public void setSigned(MultiPartInitiateUploadRequest signed) {
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
