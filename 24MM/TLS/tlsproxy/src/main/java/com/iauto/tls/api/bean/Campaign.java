package com.iauto.tls.api.bean;

/**
 * The type Campaign.
 */
public class Campaign {
    /**
     * キャンペーン ID
     */
    private Number campaignId;
    /**
     * キャンペーンタイプ
     * “OTA”：OTA
     * “INFORMATION”：通知の
     * み（入庫案内等）
     */
    private String campaignType;
    /**
     * キャンペーン種別
     * “MARKET”：市場リプロ
     * “YARD”：ヤードリプロ
     * “OTHERS”：その他
     */
    private String campaignKind;
    /**
     * 車載 HMI 表示情報
     */
    private HmiInformation hmiInformation;

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
}
