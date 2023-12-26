package com.iauto.tls.api.bean;

/**
 * The type Upload part.
 */
public class UploadPart {
    /**
     * ECU 情報（ECU の ID）
     */
    private Number partNumber;

    /**
     * part data を 128 bit の
     * MD5 でハッシュ化した上
     * で、base64 でエンコードし
     * た値。車両ログアップロー
     * ドの際にメッセージの整合
     * 性チェックで用いる。
     */
    private String md5CheckSum;

    /**
     * 署名付 URL（only for C_37_Res）
     */
    private String uploadURL;

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
     * Gets md 5 check sum.
     *
     * @return the md 5 check sum
     */
    public String getMd5CheckSum() {
        return md5CheckSum;
    }

    /**
     * Sets md 5 check sum.
     *
     * @param md5CheckSum the md 5 check sum
     */
    public void setMd5CheckSum(String md5CheckSum) {
        this.md5CheckSum = md5CheckSum;
    }

    /**
     * Gets upload url.
     *
     * @return the upload url
     */
    public String getUploadURL() {
        return uploadURL;
    }

    /**
     * Sets upload url.
     *
     * @param uploadURL the upload url
     */
    public void setUploadURL(String uploadURL) {
        this.uploadURL = uploadURL;
    }
}
