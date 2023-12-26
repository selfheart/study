package com.iauto.tls.api.bean;

/**
 * The type Activation extras.
 */
public class ActivationExtras {

    /**
     * ソフトアクティベートが完了
     * した UNIX 時刻
     */
    private Number updateTimeUTS;
    /**
     * "success":成功、"failure":
     * 失敗
     */
    private String updateStatus;
    /**
     * 表 2-88 参照
     * ECU 情報(複数)
     */
    private EcuInformation ecuInfo;
    /**
     * OEM エラー情報(複数)
     */
    private OemError[] oemErrors;

    /**
     * Gets update time uts.
     *
     * @return the update time uts
     */
    public Number getUpdateTimeUTS() {
        return updateTimeUTS;
    }

    /**
     * Sets update time uts.
     *
     * @param updateTimeUTS the update time uts
     */
    public void setUpdateTimeUTS(Number updateTimeUTS) {
        this.updateTimeUTS = updateTimeUTS;
    }

    /**
     * Gets update status.
     *
     * @return the update status
     */
    public String getUpdateStatus() {
        return updateStatus;
    }

    /**
     * Sets update status.
     *
     * @param updateStatus the update status
     */
    public void setUpdateStatus(String updateStatus) {
        this.updateStatus = updateStatus;
    }

    /**
     * Gets ecu info.
     *
     * @return the ecu info
     */
    public EcuInformation getEcuInfo() {
        return ecuInfo;
    }

    /**
     * Sets ecu info.
     *
     * @param ecuInfo the ecu info
     */
    public void setEcuInfo(EcuInformation ecuInfo) {
        this.ecuInfo = ecuInfo;
    }

    /**
     * Get oem errors oem error [ ].
     *
     * @return the oem error [ ]
     */
    public OemError[] getOemErrors() {
        return oemErrors;
    }

    /**
     * Sets oem errors.
     *
     * @param oemErrors the oem errors
     */
    public void setOemErrors(OemError[] oemErrors) {
        this.oemErrors = oemErrors;
    }
}
