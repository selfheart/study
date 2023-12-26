package com.iauto.tls.api.bean;

/**
 * The type Augmented metadata.
 */
public class AugmentedMetadata {
    private Campaign[] campaigns;
    private SubscriptionInfo subscriptionInfo;
    /**
     * “OTA_DISABLED” ： OTA
     * 無効
     * “RE_SYNC”：車両構成情
     * 報アップロード送信要求
     * “STAKE_REQUEST” ： 過
     * 去バージョン要求
     * “ NON_STD_CONFIG ” ：
     * 構成異常
     * “NO_UPDATE”：キャンペ
     * ーン無
     * “UPDATE”：キャンペーン
     * 有
     */
    private String status;
    private SelectedCampaign selectedCampaign;

    /**
     * Get campaigns campaign [ ].
     *
     * @return the campaign [ ]
     */
    public Campaign[] getCampaigns() {
        return campaigns;
    }

    /**
     * Sets campaigns.
     *
     * @param campaigns the campaigns
     */
    public void setCampaigns(Campaign[] campaigns) {
        this.campaigns = campaigns;
    }

    /**
     * Gets subscription info.
     *
     * @return the subscription info
     */
    public SubscriptionInfo getSubscriptionInfo() {
        return subscriptionInfo;
    }

    /**
     * Sets subscription info.
     *
     * @param subscriptionInfo the subscription info
     */
    public void setSubscriptionInfo(SubscriptionInfo subscriptionInfo) {
        this.subscriptionInfo = subscriptionInfo;
    }

    /**
     * Gets status.
     *
     * @return the status
     */
    public String getStatus() {
        return status;
    }

    /**
     * Sets status.
     *
     * @param status the status
     */
    public void setStatus(String status) {
        this.status = status;
    }

    /**
     * Gets selected campaign.
     *
     * @return the selected campaign
     */
    public SelectedCampaign getSelectedCampaign() {
        return selectedCampaign;
    }

    /**
     * Sets selected campaign.
     *
     * @param selectedCampaign the selected campaign
     */
    public void setSelectedCampaign(SelectedCampaign selectedCampaign) {
        this.selectedCampaign = selectedCampaign;
    }
}
