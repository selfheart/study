package com.iauto.tls.api.bean;

/**
 * The type Install progress extras.
 */
public class InstallProgressExtras {
    /**
     * インストール ECU 完了数
     */
    private Number numEcus;
    /**
     * インストール ECU 完了数
     */
    private Number completeEcus;
    /**
     * 表 2-77 参照
     * インストール進捗ECUリス
     * ト
     */
    private EcuProgress[] ecuProgress;

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
     * Gets complete ecus.
     *
     * @return the complete ecus
     */
    public Number getCompleteEcus() {
        return completeEcus;
    }

    /**
     * Sets complete ecus.
     *
     * @param completeEcus the complete ecus
     */
    public void setCompleteEcus(Number completeEcus) {
        this.completeEcus = completeEcus;
    }

    /**
     * Get ecu progress ecu progress [ ].
     *
     * @return the ecu progress [ ]
     */
    public EcuProgress[] getEcuProgress() {
        return ecuProgress;
    }

    /**
     * Sets ecu progress.
     *
     * @param ecuProgress the ecu progress
     */
    public void setEcuProgress(EcuProgress[] ecuProgress) {
        this.ecuProgress = ecuProgress;
    }
}
