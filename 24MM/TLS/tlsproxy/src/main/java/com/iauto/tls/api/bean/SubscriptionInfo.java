package com.iauto.tls.api.bean;

/**
 * The type Subscription info.
 */
public class SubscriptionInfo {
    private Boolean otaEnabled;
    private Number statusCheckInterval;

    /**
     * Gets ota enabled.
     *
     * @return the ota enabled
     */
    public Boolean getOtaEnabled() {
        return otaEnabled;
    }

    /**
     * Sets ota enabled.
     *
     * @param otaEnabled the ota enabled
     */
    public void setOtaEnabled(Boolean otaEnabled) {
        this.otaEnabled = otaEnabled;
    }

    /**
     * Gets status check interval.
     *
     * @return the status check interval
     */
    public Number getStatusCheckInterval() {
        return statusCheckInterval;
    }

    /**
     * Sets status check interval.
     *
     * @param statusCheckInterval the status check interval
     */
    public void setStatusCheckInterval(Number statusCheckInterval) {
        this.statusCheckInterval = statusCheckInterval;
    }
}
