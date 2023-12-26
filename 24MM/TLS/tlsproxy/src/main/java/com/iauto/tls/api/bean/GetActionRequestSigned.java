package com.iauto.tls.api.bean;

/**
 * The type Get action request signed.
 */
public class GetActionRequestSigned {
    /**
     * VIN 番号
     */
    private String vin;
    /**
     * プライマリ ECU の一意の
     * 識別子(シリアル番号など)
     */
    private String primaryId;

    /**
     * Gets vin.
     *
     * @return the vin
     */
    public String getVin() {
        return vin;
    }

    /**
     * Sets vin.
     *
     * @param vin the vin
     */
    public void setVin(String vin) {
        this.vin = vin;
    }

    /**
     * Gets primary id.
     *
     * @return the primary id
     */
    public String getPrimaryId() {
        return primaryId;
    }

    /**
     * Sets primary id.
     *
     * @param primaryId the primary id
     */
    public void setPrimaryId(String primaryId) {
        this.primaryId = primaryId;
    }
}
