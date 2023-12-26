package com.iauto.tls.api.bean;

/**
 * The type Selected campaign.
 */
public class SelectedCampaign {
    private Number campaignId;
    private String campaignType;
    private String campaignKind;
    private UpdatePolicy policy;
    private SyncSystemASSY[] syncSystemASSYs;
    private HmiInformation hmiInformation;
    private PreCondition[] preConditions;
    private AcceptanceInfo campaignAcceptan;

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
     * Gets campaign type.
     *
     * @return the campaign type
     */
    public String getCampaignType() {
        return campaignType;
    }

    /**
     * Sets campaign type.
     *
     * @param campaignType the campaign type
     */
    public void setCampaignType(String campaignType) {
        this.campaignType = campaignType;
    }

    /**
     * Gets campaign kind.
     *
     * @return the campaign kind
     */
    public String getCampaignKind() {
        return campaignKind;
    }

    /**
     * Sets campaign kind.
     *
     * @param campaignKind the campaign kind
     */
    public void setCampaignKind(String campaignKind) {
        this.campaignKind = campaignKind;
    }

    /**
     * Gets policy.
     *
     * @return the policy
     */
    public UpdatePolicy getPolicy() {
        return policy;
    }

    /**
     * Sets policy.
     *
     * @param policy the policy
     */
    public void setPolicy(UpdatePolicy policy) {
        this.policy = policy;
    }

    /**
     * Get sync system ass ys sync system assy [ ].
     *
     * @return the sync system assy [ ]
     */
    public SyncSystemASSY[] getSyncSystemASSYs() {
        return syncSystemASSYs;
    }

    /**
     * Sets sync system ass ys.
     *
     * @param syncSystemASSYs the sync system ass ys
     */
    public void setSyncSystemASSYs(SyncSystemASSY[] syncSystemASSYs) {
        this.syncSystemASSYs = syncSystemASSYs;
    }

    /**
     * Gets hmi information.
     *
     * @return the hmi information
     */
    public HmiInformation getHmiInformation() {
        return hmiInformation;
    }

    /**
     * Sets hmi information.
     *
     * @param hmiInformation the hmi information
     */
    public void setHmiInformation(HmiInformation hmiInformation) {
        this.hmiInformation = hmiInformation;
    }

    /**
     * Get pre conditions pre condition [ ].
     *
     * @return the pre condition [ ]
     */
    public PreCondition[] getPreConditions() {
        return preConditions;
    }

    /**
     * Sets pre conditions.
     *
     * @param preConditions the pre conditions
     */
    public void setPreConditions(PreCondition[] preConditions) {
        this.preConditions = preConditions;
    }

    /**
     * Gets campaign acceptan.
     *
     * @return the campaign acceptan
     */
    public AcceptanceInfo getCampaignAcceptan() {
        return campaignAcceptan;
    }

    /**
     * Sets campaign acceptan.
     *
     * @param campaignAcceptan the campaign acceptan
     */
    public void setCampaignAcceptan(AcceptanceInfo campaignAcceptan) {
        this.campaignAcceptan = campaignAcceptan;
    }
}
