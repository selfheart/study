package com.iauto.tls.api.bean;

/**
 * The type Pre condition.
 */
public class PreCondition {
    /**
     * 前提条件
     */
    private String preCondition;
    /**
     * 適用条件（必須／任意）
     * “MANDATORY”,
     * “CONDITIONAL”
     */
    private String displayMode;
    /**
     * 前提条件数値
     */
    private Number value;

    /**
     * Gets pre condition.
     *
     * @return the pre condition
     */
    public String getPreCondition() {
        return preCondition;
    }

    /**
     * Sets pre condition.
     *
     * @param preCondition the pre condition
     */
    public void setPreCondition(String preCondition) {
        this.preCondition = preCondition;
    }

    /**
     * Gets display mode.
     *
     * @return the display mode
     */
    public String getDisplayMode() {
        return displayMode;
    }

    /**
     * Sets display mode.
     *
     * @param displayMode the display mode
     */
    public void setDisplayMode(String displayMode) {
        this.displayMode = displayMode;
    }

    /**
     * Gets value.
     *
     * @return the value
     */
    public Number getValue() {
        return value;
    }

    /**
     * Sets value.
     *
     * @param value the value
     */
    public void setValue(Number value) {
        this.value = value;
    }
}
