package com.iauto.tls.api.bean;

/**
 * The type Vsm device events signed.
 */
public class VsmDeviceEventsSigned {
    /**
     * VIN
     */
    private String vin;
    /**
     * プライマリ ECU の一意の
     * 識別子(シリアル番号など)
     */
    private String primaryId;
    /**
     * メッセージ本文(複数も可
     * 能)
     * 各 I/F を参照
     */
    private VsmEvents[] events;

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

    /**
     * Get events vsm events [ ].
     *
     * @return the vsm events [ ]
     */
    public VsmEvents[] getEvents() {
        return events;
    }

    /**
     * Sets events.
     *
     * @param events the events
     */
    public void setEvents(VsmEvents[] events) {
        this.events = events;
    }
}
