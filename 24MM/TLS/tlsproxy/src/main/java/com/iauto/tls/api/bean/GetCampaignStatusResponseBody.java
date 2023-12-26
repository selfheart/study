package com.iauto.tls.api.bean;

/**
 * The type Get campaign status response body.
 */
public class GetCampaignStatusResponseBody {
    /**
     * VIN 番号
     */
    private String vin;
    /**
     * キャンペーン ID
     */
    private Number campaignId;
    /**
     * OTA センタでのキャンペー
     * ン有効状態を示す。
     * “Deployed”：有効
     * “Canceled”：中止
     * “Expired”：期限切れ
     * “Paused”：一時配信停止
     * “Other”：その他
     */
    private String status;

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
}
