package com.iauto.tls.api.bean;

/**
 * The type Activate progress extras.
 */
public class ActivateProgressExtras {
    /**
     * アクティベート ECU予定数
     */
    private Number totalActivation;

    /**
     * アクティベート ECU完了数
     */
    private Number completeActivation;

    /**
     * Gets total activation.
     *
     * @return the total activation
     */
    public Number getTotalActivation() {
        return totalActivation;
    }

    /**
     * Sets total activation.
     *
     * @param totalActivation the total activation
     */
    public void setTotalActivation(Number totalActivation) {
        this.totalActivation = totalActivation;
    }

    /**
     * Gets complete activation.
     *
     * @return the complete activation
     */
    public Number getCompleteActivation() {
        return completeActivation;
    }

    /**
     * Sets complete activation.
     *
     * @param completeActivation the complete activation
     */
    public void setCompleteActivation(Number completeActivation) {
        this.completeActivation = completeActivation;
    }
}
