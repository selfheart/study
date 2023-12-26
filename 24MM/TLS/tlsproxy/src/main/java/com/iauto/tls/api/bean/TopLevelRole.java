package com.iauto.tls.api.bean;

/**
 * The type Top level role.
 */
public class TopLevelRole {
    /**
     * 役割
     * {root, targets, snapshot,
     * timestamp}
     */
    private String role;
    /**
     * role が指し示すメタデータ
     * の 署 名 検 証 用 公 開 鍵
     * ID(*1)
     */
    private String[] keyIds;
    /**
     * role が指し示すメタデータ
     * の署名が正当と判断する
     * 署名検証成功数の閾値
     */
    private Number threshold;

    /**
     * Gets role.
     *
     * @return the role
     */
    public String getRole() {
        return role;
    }

    /**
     * Sets role.
     *
     * @param role the role
     */
    public void setRole(String role) {
        this.role = role;
    }

    /**
     * Get key ids string [ ].
     *
     * @return the string [ ]
     */
    public String[] getKeyIds() {
        return keyIds;
    }

    /**
     * Sets key ids.
     *
     * @param keyIds the key ids
     */
    public void setKeyIds(String[] keyIds) {
        this.keyIds = keyIds;
    }

    /**
     * Gets threshold.
     *
     * @return the threshold
     */
    public Number getThreshold() {
        return threshold;
    }

    /**
     * Sets threshold.
     *
     * @param threshold the threshold
     */
    public void setThreshold(Number threshold) {
        this.threshold = threshold;
    }
}
