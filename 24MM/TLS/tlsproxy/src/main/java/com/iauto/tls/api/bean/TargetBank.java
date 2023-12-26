package com.iauto.tls.api.bean;

/**
 * The type Target bank.
 */
public class TargetBank {
    /**
     * 面番号(A=1 面, B=2 面)
     * 1 面、もしくは面情報が
     * 取得できない場合
     * は、”A”固定とする。
     * T-OTA4.0 では、“A”固
     * 定とする。
     */
    private String bank;
    /**
     * ECU ソフトウェア詳細
     */
    private SoftwareDetail[] softwareDetails;
    /**
     * ECU ハードウェア品番
     */
    private String hwId;

    /**
     * Gets bank.
     *
     * @return the bank
     */
    public String getBank() {
        return bank;
    }

    /**
     * Sets bank.
     *
     * @param bank the bank
     */
    public void setBank(String bank) {
        this.bank = bank;
    }

    /**
     * Get software details software detail [ ].
     *
     * @return the software detail [ ]
     */
    public SoftwareDetail[] getSoftwareDetails() {
        return softwareDetails;
    }

    /**
     * Sets software details.
     *
     * @param softwareDetails the software details
     */
    public void setSoftwareDetails(SoftwareDetail[] softwareDetails) {
        this.softwareDetails = softwareDetails;
    }

    /**
     * Gets hw id.
     *
     * @return the hw id
     */
    public String getHwId() {
        return hwId;
    }

    /**
     * Sets hw id.
     *
     * @param hwId the hw id
     */
    public void setHwId(String hwId) {
        this.hwId = hwId;
    }
}
