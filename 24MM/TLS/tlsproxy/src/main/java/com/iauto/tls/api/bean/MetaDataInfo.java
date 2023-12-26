package com.iauto.tls.api.bean;

/**
 * The type Meta data info.
 */
public class MetaDataInfo {
    private String iv;
    private String mac;
    private String macIv;
    private String decompressedDecryptedLength;

    /**
     * Gets iv.
     *
     * @return the iv
     */
    public String getIv() {
        return iv;
    }

    /**
     * Sets iv.
     *
     * @param iv the iv
     */
    public void setIv(String iv) {
        this.iv = iv;
    }

    /**
     * Gets mac.
     *
     * @return the mac
     */
    public String getMac() {
        return mac;
    }

    /**
     * Sets mac.
     *
     * @param mac the mac
     */
    public void setMac(String mac) {
        this.mac = mac;
    }

    /**
     * Gets mac iv.
     *
     * @return the mac iv
     */
    public String getMacIv() {
        return macIv;
    }

    /**
     * Sets mac iv.
     *
     * @param macIv the mac iv
     */
    public void setMacIv(String macIv) {
        this.macIv = macIv;
    }

    /**
     * Gets decompressed decrypted length.
     *
     * @return the decompressed decrypted length
     */
    public String getDecompressedDecryptedLength() {
        return decompressedDecryptedLength;
    }

    /**
     * Sets decompressed decrypted length.
     *
     * @param decompressedDecryptedLength the decompressed decrypted length
     */
    public void setDecompressedDecryptedLength(String decompressedDecryptedLength) {
        this.decompressedDecryptedLength = decompressedDecryptedLength;
    }
}
