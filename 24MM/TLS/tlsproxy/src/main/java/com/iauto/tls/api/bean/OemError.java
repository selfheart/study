package com.iauto.tls.api.bean;

/**
 * The type Oem error.
 */
public class OemError {
    /**
     * エラー発生時のアップデー
     * トライフサイクルのフェーズ
     */
    private Number phaseCode;
    /**
     * エラー発生コンポーネント
     */
    private Number siteCode;
    /**
     * OEM エラーコード
     */
    private Number oemCode;

    /**
     * ECU 個別識別子
     */
    private String targetId;

    /**
     * Gets phase code.
     *
     * @return the phase code
     */
    public Number getPhaseCode() {
        return phaseCode;
    }

    /**
     * Sets phase code.
     *
     * @param phaseCode the phase code
     */
    public void setPhaseCode(Number phaseCode) {
        this.phaseCode = phaseCode;
    }

    /**
     * Gets site code.
     *
     * @return the site code
     */
    public Number getSiteCode() {
        return siteCode;
    }

    /**
     * Sets site code.
     *
     * @param siteCode the site code
     */
    public void setSiteCode(Number siteCode) {
        this.siteCode = siteCode;
    }

    /**
     * Gets oem code.
     *
     * @return the oem code
     */
    public Number getOemCode() {
        return oemCode;
    }

    /**
     * Sets oem code.
     *
     * @param oemCode the oem code
     */
    public void setOemCode(Number oemCode) {
        this.oemCode = oemCode;
    }

    /**
     * Gets target id.
     *
     * @return the target id
     */
    public String getTargetId() {
        return targetId;
    }

    /**
     * Sets target id.
     *
     * @param targetId the target id
     */
    public void setTargetId(String targetId) {
        this.targetId = targetId;
    }
}
