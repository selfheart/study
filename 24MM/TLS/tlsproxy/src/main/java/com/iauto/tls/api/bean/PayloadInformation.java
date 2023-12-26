package com.iauto.tls.api.bean;

/**
 * The type Payload information.
 */
public class PayloadInformation {
    /**
     * Push notification type.
     * 1. High priority campaign notification
     * 2. OTA Action Information acquisition notification
     * 3. External HMI acceptance notification
     * 4. Parked OTA [Not applicable for 19PFv3]
     * 5. OTA log update notification
     * 6. Campaign abort notification
     */
    private byte pushType;
    /**
     * campaignId Campaign ID information associated with Push.(If not specified, set to 0x00000000)
     */
    private int campaignId;

    /**
     * Gets push type.
     *
     * @return the push type
     */
    public byte getPushType() {
        return pushType;
    }

    /**
     * Sets push type.
     *
     * @param pushType the push type
     */
    public void setPushType(byte pushType) {
        this.pushType = pushType;
    }

    /**
     * Gets campaign id.
     *
     * @return the campaign id
     */
    public int getCampaignId() {
        return campaignId;
    }

    /**
     * Sets campaign id.
     *
     * @param campaignId the campaign id
     */
    public void setCampaignId(int campaignId) {
        this.campaignId = campaignId;
    }
}
