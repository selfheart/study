package com.iauto.tls.api.bean;

/**
 * The type Update policy.
 */
public class UpdatePolicy {
    private String campaignApproval;
    private String downloadApproval;
    private String installApproval;
    private String activateApproval;
    private String updatecompletionAck;
    private String[] notifications;
    private String reportId;
    private PkgDecryptionInformation pkgDecryptionInfo;

    /**
     * Gets campaign approval.
     *
     * @return the campaign approval
     */
    public String getCampaignApproval() {
        return campaignApproval;
    }

    /**
     * Sets campaign approval.
     *
     * @param campaignApproval the campaign approval
     */
    public void setCampaignApproval(String campaignApproval) {
        this.campaignApproval = campaignApproval;
    }

    /**
     * Gets download approval.
     *
     * @return the download approval
     */
    public String getDownloadApproval() {
        return downloadApproval;
    }

    /**
     * Sets download approval.
     *
     * @param downloadApproval the download approval
     */
    public void setDownloadApproval(String downloadApproval) {
        this.downloadApproval = downloadApproval;
    }

    /**
     * Gets install approval.
     *
     * @return the install approval
     */
    public String getInstallApproval() {
        return installApproval;
    }

    /**
     * Sets install approval.
     *
     * @param installApproval the install approval
     */
    public void setInstallApproval(String installApproval) {
        this.installApproval = installApproval;
    }

    /**
     * Gets activate approval.
     *
     * @return the activate approval
     */
    public String getActivateApproval() {
        return activateApproval;
    }

    /**
     * Sets activate approval.
     *
     * @param activateApproval the activate approval
     */
    public void setActivateApproval(String activateApproval) {
        this.activateApproval = activateApproval;
    }

    /**
     * Gets updatecompletion ack.
     *
     * @return the updatecompletion ack
     */
    public String getUpdatecompletionAck() {
        return updatecompletionAck;
    }

    /**
     * Sets updatecompletion ack.
     *
     * @param updatecompletionAck the updatecompletion ack
     */
    public void setUpdatecompletionAck(String updatecompletionAck) {
        this.updatecompletionAck = updatecompletionAck;
    }

    /**
     * Get notifications string [ ].
     *
     * @return the string [ ]
     */
    public String[] getNotifications() {
        return notifications;
    }

    /**
     * Sets notifications.
     *
     * @param notifications the notifications
     */
    public void setNotifications(String[] notifications) {
        this.notifications = notifications;
    }

    /**
     * Gets report id.
     *
     * @return the report id
     */
    public String getReportId() {
        return reportId;
    }

    /**
     * Sets report id.
     *
     * @param reportId the report id
     */
    public void setReportId(String reportId) {
        this.reportId = reportId;
    }

    /**
     * Gets pkg decryption info.
     *
     * @return the pkg decryption info
     */
    public PkgDecryptionInformation getPkgDecryptionInfo() {
        return pkgDecryptionInfo;
    }

    /**
     * Sets pkg decryption info.
     *
     * @param pkgDecryptionInfo the pkg decryption info
     */
    public void setPkgDecryptionInfo(PkgDecryptionInformation pkgDecryptionInfo) {
        this.pkgDecryptionInfo = pkgDecryptionInfo;
    }
}
