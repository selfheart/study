package com.iauto.tls.api.bean;

/**
 * The type Software detail.
 */
public class SoftwareDetail {
    /**
     * ECU 内ソフトウェアのサブ
     * ターゲット ID
     */
    private String subTargetId;
    /**
     * ソフトウェア品番
     */
    private String productNumber;

    /**
     * Gets sub target id.
     *
     * @return the sub target id
     */
    public String getSubTargetId() {
        return subTargetId;
    }

    /**
     * Sets sub target id.
     *
     * @param subTargetId the sub target id
     */
    public void setSubTargetId(String subTargetId) {
        this.subTargetId = subTargetId;
    }

    /**
     * Gets product number.
     *
     * @return the product number
     */
    public String getProductNumber() {
        return productNumber;
    }

    /**
     * Sets product number.
     *
     * @param productNumber the product number
     */
    public void setProductNumber(String productNumber) {
        this.productNumber = productNumber;
    }
}
