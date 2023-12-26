package com.iauto.tls.api.response;

import com.iauto.tls.api.bean.UploadPart;

/**
 * The type Upload context.
 */
public class UploadContext {
    /**
     * 2.1.46 車両ログアップロー
     * ド完了通知で利用
     */
    private String uploadContext;
    /**
     * UploadPart フォーマット
     */
    private UploadPart[] uploadParts;

    /**
     * Gets upload context.
     *
     * @return the upload context
     */
    public String getUploadContext() {
        return uploadContext;
    }

    /**
     * Sets upload context.
     *
     * @param uploadContext the upload context
     */
    public void setUploadContext(String uploadContext) {
        this.uploadContext = uploadContext;
    }

    /**
     * Get upload parts upload part [ ].
     *
     * @return the upload part [ ]
     */
    public UploadPart[] getUploadParts() {
        return uploadParts;
    }

    /**
     * Sets upload parts.
     *
     * @param uploadParts the upload parts
     */
    public void setUploadParts(UploadPart[] uploadParts) {
        this.uploadParts = uploadParts;
    }
}
