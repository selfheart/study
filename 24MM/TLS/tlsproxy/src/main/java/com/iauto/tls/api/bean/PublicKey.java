package com.iauto.tls.api.bean;

/**
 * The type Public key.
 */
public class PublicKey {
    /**
     * 署名検証用公開鍵 ID(*1)
     */
    private String publicKeyId;
    /**
     * 鍵種別(RSA/ECDSA)
     */
    private String publicKeyType;
    /**
     * 署 名 検 証 用 公 開 鍵 を
     * Base16 でエンコードした
     * 文字列
     */
    private String publicKeyValue;

    /**
     * Gets public key id.
     *
     * @return the public key id
     */
    public String getPublicKeyId() {
        return publicKeyId;
    }

    /**
     * Sets public key id.
     *
     * @param publicKeyId the public key id
     */
    public void setPublicKeyId(String publicKeyId) {
        this.publicKeyId = publicKeyId;
    }

    /**
     * Gets public key type.
     *
     * @return the public key type
     */
    public String getPublicKeyType() {
        return publicKeyType;
    }

    /**
     * Sets public key type.
     *
     * @param publicKeyType the public key type
     */
    public void setPublicKeyType(String publicKeyType) {
        this.publicKeyType = publicKeyType;
    }

    /**
     * Gets public key value.
     *
     * @return the public key value
     */
    public String getPublicKeyValue() {
        return publicKeyValue;
    }

    /**
     * Sets public key value.
     *
     * @param publicKeyValue the public key value
     */
    public void setPublicKeyValue(String publicKeyValue) {
        this.publicKeyValue = publicKeyValue;
    }
}
