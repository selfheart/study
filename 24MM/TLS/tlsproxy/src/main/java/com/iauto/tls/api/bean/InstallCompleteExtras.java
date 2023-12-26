package com.iauto.tls.api.bean;

/**
 * The type Install complete extras.
 */
public class InstallCompleteExtras {
    /**
     * インストールされた全 ECU
     * 数
     */
    private Number numEcus;
    /**
     * インストール成功した ECU
     * 数
     */
    private Number successEcus;
    /**
     * インストール失敗した ECU
     * 数
     */
    private Number failEcus;
    /**
     * ECU 情報(複数)
     */
    private EcuInformation[] ecuInfo;
    /**
     * OEM エラー情報(複数)
     */
    private OemError[] oemErrors;
    /**
     * ソフトインストールが完了し
     * た UNIX 時刻
     */
    private Number updateTimeUTS;

    /**
     * Gets num ecus.
     *
     * @return the num ecus
     */
    public Number getNumEcus() {
        return numEcus;
    }

    /**
     * Sets num ecus.
     *
     * @param numEcus the num ecus
     */
    public void setNumEcus(Number numEcus) {
        this.numEcus = numEcus;
    }

    /**
     * Gets success ecus.
     *
     * @return the success ecus
     */
    public Number getSuccessEcus() {
        return successEcus;
    }

    /**
     * Sets success ecus.
     *
     * @param successEcus the success ecus
     */
    public void setSuccessEcus(Number successEcus) {
        this.successEcus = successEcus;
    }

    /**
     * Gets fail ecus.
     *
     * @return the fail ecus
     */
    public Number getFailEcus() {
        return failEcus;
    }

    /**
     * Sets fail ecus.
     *
     * @param failEcus the fail ecus
     */
    public void setFailEcus(Number failEcus) {
        this.failEcus = failEcus;
    }

    /**
     * Get ecu info ecu information [ ].
     *
     * @return the ecu information [ ]
     */
    public EcuInformation[] getEcuInfo() {
        return ecuInfo;
    }

    /**
     * Sets ecu info.
     *
     * @param ecuInfo the ecu info
     */
    public void setEcuInfo(EcuInformation[] ecuInfo) {
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
}
