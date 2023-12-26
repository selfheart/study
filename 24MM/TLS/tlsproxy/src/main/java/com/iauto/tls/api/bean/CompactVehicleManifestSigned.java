package com.iauto.tls.api.bean;

/**
 * The type Compact vehicle manifest signed.
 */
public class CompactVehicleManifestSigned {
    /**
     * VIN 番号
     */
    private String vin;
    /**
     * プライマリ ECU の一意の
     * 識別子(シリアル番号など)
     */
    private String primaryId;
    private String clientDigest;
    private Number timestamp;
    private PackageStorage packageStorage;
    private String uploadReason;
    private String locale;
    private Number preferredCampaignId;
    private String attemptId;

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
     * Gets client digest.
     *
     * @return the client digest
     */
    public String getClientDigest() {
        return clientDigest;
    }

    /**
     * Sets client digest.
     *
     * @param clientDigest the client digest
     */
    public void setClientDigest(String clientDigest) {
        this.clientDigest = clientDigest;
    }

    /**
     * Gets timestamp.
     *
     * @return the timestamp
     */
    public Number getTimestamp() {
        return timestamp;
    }

    /**
     * Sets timestamp.
     *
     * @param timestamp the timestamp
     */
    public void setTimestamp(Number timestamp) {
        this.timestamp = timestamp;
    }

    /**
     * Gets package storage.
     *
     * @return the package storage
     */
    public PackageStorage getPackageStorage() {
        return packageStorage;
    }

    /**
     * Sets package storage.
     *
     * @param packageStorage the package storage
     */
    public void setPackageStorage(PackageStorage packageStorage) {
        this.packageStorage = packageStorage;
    }

    /**
     * Gets upload reason.
     *
     * @return the upload reason
     */
    public String getUploadReason() {
        return uploadReason;
    }

    /**
     * Sets upload reason.
     *
     * @param uploadReason the upload reason
     */
    public void setUploadReason(String uploadReason) {
        this.uploadReason = uploadReason;
    }

    /**
     * Gets locale.
     *
     * @return the locale
     */
    public String getLocale() {
        return locale;
    }

    /**
     * Sets locale.
     *
     * @param locale the locale
     */
    public void setLocale(String locale) {
        this.locale = locale;
    }

    /**
     * Gets preferred campaign id.
     *
     * @return the preferred campaign id
     */
    public Number getPreferredCampaignId() {
        return preferredCampaignId;
    }

    /**
     * Sets preferred campaign id.
     *
     * @param preferredCampaignId the preferred campaign id
     */
    public void setPreferredCampaignId(Number preferredCampaignId) {
        this.preferredCampaignId = preferredCampaignId;
    }

    /**
     * Gets attempt id.
     *
     * @return the attempt id
     */
    public String getAttemptId() {
        return attemptId;
    }

    /**
     * Sets attempt id.
     *
     * @param attemptId the attempt id
     */
    public void setAttemptId(String attemptId) {
        this.attemptId = attemptId;
    }
}
