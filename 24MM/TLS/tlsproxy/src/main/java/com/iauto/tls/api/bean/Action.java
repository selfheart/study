package com.iauto.tls.api.bean;

/**
 * The type Action.
 */
public class Action {
    /**
     * OTA Action ID
     * 詳細は下記参照
     */
    private String actionId;
    /**
     * OTA Action に応じたパラ
     * メタ
     */
    private String value;

    /**
     * Gets action id.
     *
     * @return the action id
     */
    public String getActionId() {
        return actionId;
    }

    /**
     * Sets action id.
     *
     * @param actionId the action id
     */
    public void setActionId(String actionId) {
        this.actionId = actionId;
    }

    /**
     * Gets value.
     *
     * @return the value
     */
    public String getValue() {
        return value;
    }

    /**
     * Sets value.
     *
     * @param value the value
     */
    public void setValue(String value) {
        this.value = value;
    }
}
