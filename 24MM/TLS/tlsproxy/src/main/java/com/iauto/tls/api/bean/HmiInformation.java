package com.iauto.tls.api.bean;

/**
 * The type Hmi information.
 */
public class HmiInformation {
    /**
     * 車載 HMI ファイル情報
     */
    private HmiMessage hmiMessage;
    /**
     * 車両指定ロケール
     */
    private String requestedLocale;
    /**
     * キャンペーンに設定された
     * デフォルトロケール
     */
    private String defaultLocale;

    /**
     * Gets hmi message.
     *
     * @return the hmi message
     */
    public HmiMessage getHmiMessage() {
        return hmiMessage;
    }

    /**
     * Sets hmi message.
     *
     * @param hmiMessage the hmi message
     */
    public void setHmiMessage(HmiMessage hmiMessage) {
        this.hmiMessage = hmiMessage;
    }

    /**
     * Gets requested locale.
     *
     * @return the requested locale
     */
    public String getRequestedLocale() {
        return requestedLocale;
    }

    /**
     * Sets requested locale.
     *
     * @param requestedLocale the requested locale
     */
    public void setRequestedLocale(String requestedLocale) {
        this.requestedLocale = requestedLocale;
    }

    /**
     * Gets default locale.
     *
     * @return the default locale
     */
    public String getDefaultLocale() {
        return defaultLocale;
    }

    /**
     * Sets default locale.
     *
     * @param defaultLocale the default locale
     */
    public void setDefaultLocale(String defaultLocale) {
        this.defaultLocale = defaultLocale;
    }
}
