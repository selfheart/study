package com.iauto.tls.api.bean;

/**
 * The type Part e tag.
 */
public class PartETag {
    /**
     * ECU 情報（ECU の ID）
     */
    private Number partNumber;
    /**
     * 2.1.44 車両ログアップロー
     * ド（C39_req）のレスポンス
     * ヘッダに入力された値を利
     * 用
     */
    private String etag;

    /**
     * Gets part number.
     *
     * @return the part number
     */
    public Number getPartNumber() {
        return partNumber;
    }

    /**
     * Sets part number.
     *
     * @param partNumber the part number
     */
    public void setPartNumber(Number partNumber) {
        this.partNumber = partNumber;
    }

    /**
     * Gets etag.
     *
     * @return the etag
     */
    public String getEtag() {
        return etag;
    }

    /**
     * Sets etag.
     *
     * @param etag the etag
     */
    public void setEtag(String etag) {
        this.etag = etag;
    }
}
