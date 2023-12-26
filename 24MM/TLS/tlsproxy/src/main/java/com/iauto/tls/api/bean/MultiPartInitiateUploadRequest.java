package com.iauto.tls.api.bean;

/**
 * The type Multi part initiate upload request.
 */
public class MultiPartInitiateUploadRequest {
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
     * アップロードトリガー情報
     * 例：OTA センタからの場
     * 合、“ota center”、車両空
     * の場合、”vehicle”
     */
    private String uploadTrigger;
    /**
     * キャンペーンに紐づく場合
     * のみ入力
     */
    private Number campaignId;

    /**
     * errorCode
     */
    private Number errorCode;
    /**
     * 表 2-105 UploadPart フ
     * ォーマット参照
     */
    private UploadPart[] uploadParts;
    /**
     * yyyy-MM-dd'T'HH:mm:ssZ （UTC）
     */
    private String beginTime;
    /**
     * yyyy-MM-dd'T'HH:mm:ssZ （UTC）
     */
    private String endTime;
    /**
     * 設定値通りに入力すること
     */
    private String category;
    /**
     * 下記が有効な値：
     * "cellular", "wifi", "external
     * memory","bluetooth",
     * "other".
     */
    private String transportType;
    /**
     * 240 文字以内で下記文字
     * が有効：
     * 0-9, a-z, A-Z, “-“, “_”, ”.
     */
    private String fileName;

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
     * Gets upload trigger.
     *
     * @return the upload trigger
     */
    public String getUploadTrigger() {
        return uploadTrigger;
    }

    /**
     * Sets upload trigger.
     *
     * @param uploadTrigger the upload trigger
     */
    public void setUploadTrigger(String uploadTrigger) {
        this.uploadTrigger = uploadTrigger;
    }

    /**
     * Gets campaign id.
     *
     * @return the campaign id
     */
    public Number getCampaignId() {
        return campaignId;
    }

    /**
     * Sets campaign id.
     *
     * @param campaignId the campaign id
     */
    public void setCampaignId(Number campaignId) {
        this.campaignId = campaignId;
    }

    /**
     * Gets error code.
     *
     * @return the error code
     */
    public Number getErrorCode() {
        return errorCode;
    }

    /**
     * Sets error code.
     *
     * @param errorCode the error code
     */
    public void setErrorCode(Number errorCode) {
        this.errorCode = errorCode;
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

    /**
     * Gets begin time.
     *
     * @return the begin time
     */
    public String getBeginTime() {
        return beginTime;
    }

    /**
     * Sets begin time.
     *
     * @param beginTime the begin time
     */
    public void setBeginTime(String beginTime) {
        this.beginTime = beginTime;
    }

    /**
     * Gets end time.
     *
     * @return the end time
     */
    public String getEndTime() {
        return endTime;
    }

    /**
     * Sets end time.
     *
     * @param endTime the end time
     */
    public void setEndTime(String endTime) {
        this.endTime = endTime;
    }

    /**
     * Gets category.
     *
     * @return the category
     */
    public String getCategory() {
        return category;
    }

    /**
     * Sets category.
     *
     * @param category the category
     */
    public void setCategory(String category) {
        this.category = category;
    }

    /**
     * Gets transport type.
     *
     * @return the transport type
     */
    public String getTransportType() {
        return transportType;
    }

    /**
     * Sets transport type.
     *
     * @param transportType the transport type
     */
    public void setTransportType(String transportType) {
        this.transportType = transportType;
    }

    /**
     * Gets file name.
     *
     * @return the file name
     */
    public String getFileName() {
        return fileName;
    }

    /**
     * Sets file name.
     *
     * @param fileName the file name
     */
    public void setFileName(String fileName) {
        this.fileName = fileName;
    }
}
