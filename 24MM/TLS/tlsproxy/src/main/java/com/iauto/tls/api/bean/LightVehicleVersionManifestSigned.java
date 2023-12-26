package com.iauto.tls.api.bean;

/**
 * The type Light vehicle version manifest signed.
 */
public class LightVehicleVersionManifestSigned {
    /**
     * VIN
     */
    private String vin;
    /**
     * OTA マスタ搭載のECU ID
     */
    private String primaryId;
    /**
     * ECU 詳細情報
     */
    private ECUVersionManifest ecuVersionManifest;
    /**
     * AugmentedManifest フォーマット
     */
    private AugmentedManifest augmentedManifest;
    /**
     * 現在の構成情報を表す、
     * 車両で生成されたダイジェ
     * スト情報（ダイジェストは、
     * 個車構成が変化しない限
     * り変化しないこと）
     */
    private String clientDigest;

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
     * Gets ecu version manifest.
     *
     * @return the ecu version manifest
     */
    public ECUVersionManifest getEcuVersionManifest() {
        return ecuVersionManifest;
    }

    /**
     * Sets ecu version manifest.
     *
     * @param ecuVersionManifest the ecu version manifest
     */
    public void setEcuVersionManifest(ECUVersionManifest ecuVersionManifest) {
        this.ecuVersionManifest = ecuVersionManifest;
    }

    /**
     * Gets augmented manifest.
     *
     * @return the augmented manifest
     */
    public AugmentedManifest getAugmentedManifest() {
        return augmentedManifest;
    }

    /**
     * Sets augmented manifest.
     *
     * @param augmentedManifest the augmented manifest
     */
    public void setAugmentedManifest(AugmentedManifest augmentedManifest) {
        this.augmentedManifest = augmentedManifest;
    }

    /**
     * Gets client digest.
     *
     * @return the client digest
     */
    public String getClientDigest() {
        return clientDigest;
    }

    /**
     * Sets client digest.
     *
     * @param clientDigest the client digest
     */
    public void setClientDigest(String clientDigest) {
        this.clientDigest = clientDigest;
    }
}
