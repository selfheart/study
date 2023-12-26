package com.iauto.tls.api.bean;

/**
 * The type Get campaign status signed.
 */
public class GetCampaignStatusSigned {
    /**
     * VIN 番号
     */
    private String vin;
    /**
     * プライマリ ECU の一意の
     * 識別子(シリアル番号など)
     */
    private String primaryId;

    /**
     * キャンペーン ID
     */
    private Number campaignId;

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
}
