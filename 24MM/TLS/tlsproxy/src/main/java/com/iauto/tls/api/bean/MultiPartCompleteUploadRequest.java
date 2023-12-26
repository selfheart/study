package com.iauto.tls.api.bean;

/**
 * The type Multi part complete upload request.
 */
public class MultiPartCompleteUploadRequest {
    /**
     * VIN
     */
    private String vin;
    /**
     * OTA マスタ搭載のECU ID
     */
    private String primaryId;
    /**
     * UNIX の タ イ ム ス タ ン プ
     * （UTC）
     */
    private Number currentTime;
    /**
     * 2.1.43 で取得した値を入
     * 力
     */
    private String uploadContext;
    /**
     * 表 2-111 PartETag フォ
     * ーマット参照
     */
    private PartETag[] partETags;

    /**
     * Gets vin.
     *
     * @return the vin
     */
    public String getVin() {
        return vin;
    }

    /**
     * Sets vin.
     *
     * @param vin the vin
     */
    public void setVin(String vin) {
        this.vin = vin;
    }

    /**
     * Gets primary id.
     *
     * @return the primary id
     */
    public String getPrimaryId() {
        return primaryId;
    }

    /**
     * Sets primary id.
     *
     * @param primaryId the primary id
     */
    public void setPrimaryId(String primaryId) {
        this.primaryId = primaryId;
    }

    /**
     * Gets current time.
     *
     * @return the current time
     */
    public Number getCurrentTime() {
        return currentTime;
    }

    /**
     * Sets current time.
     *
     * @param currentTime the current time
     */
    public void setCurrentTime(Number currentTime) {
        this.currentTime = currentTime;
    }

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
     * Get part e tags part e tag [ ].
     *
     * @return the part e tag [ ]
     */
    public PartETag[] getPartETags() {
        return partETags;
    }

    /**
     * Sets part e tags.
     *
     * @param partETags the part e tags
     */
    public void setPartETags(PartETag[] partETags) {
        this.partETags = partETags;
    }
}
