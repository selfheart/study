package com.iauto.tls.api.bean;

/**
 * The type Augmented manifest.
 */
public class AugmentedManifest {
    private String timeStamp;
    private PackageStorage packageStorage;
    private Number lastUpdateCampaignId;
    private String[] rxswin;
    private String uploadReason;
    private String locale;
    private Number preferredCampaignId;
    private Boolean isNewAttempt;
    private String attemptId;

    /**
     * Gets time stamp.
     *
     * @return the time stamp
     */
    public String getTimeStamp() {
        return timeStamp;
    }

    /**
     * Sets time stamp.
     *
     * @param timeStamp the time stamp
     */
    public void setTimeStamp(String timeStamp) {
        this.timeStamp = timeStamp;
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
     * Gets last update campaign id.
     *
     * @return the last update campaign id
     */
    public Number getLastUpdateCampaignId() {
        return lastUpdateCampaignId;
    }

    /**
     * Sets last update campaign id.
     *
     * @param lastUpdateCampaignId the last update campaign id
     */
    public void setLastUpdateCampaignId(Number lastUpdateCampaignId) {
        this.lastUpdateCampaignId = lastUpdateCampaignId;
    }

    /**
     * Get rxswin string [ ].
     *
     * @return the string [ ]
     */
    public String[] getRxswin() {
        return rxswin;
    }

    /**
     * Sets rxswin.
     *
     * @param rxswin the rxswin
     */
    public void setRxswin(String[] rxswin) {
        this.rxswin = rxswin;
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
     * Gets new attempt.
     *
     * @return the new attempt
     */
    public Boolean getNewAttempt() {
        return isNewAttempt;
    }

    /**
     * Sets new attempt.
     *
     * @param newAttempt the new attempt
     */
    public void setNewAttempt(Boolean newAttempt) {
        isNewAttempt = newAttempt;
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
