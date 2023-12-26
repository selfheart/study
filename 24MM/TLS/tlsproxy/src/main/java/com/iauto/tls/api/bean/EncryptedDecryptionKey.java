package com.iauto.tls.api.bean;

/**
 * The type Encrypted decryption key.
 */
public class EncryptedDecryptionKey {
    private String encryptedSymmetricKeyType;
    private String encryptedSymmetricKeyValue;
    private String encryptedSymmetricKeyAlgrithmMode;
    private String paddingScheme;
    private String iv;

    /**
     * Gets encrypted symmetric key type.
     *
     * @return the encrypted symmetric key type
     */
    public String getEncryptedSymmetricKeyType() {
        return encryptedSymmetricKeyType;
    }

    /**
     * Sets encrypted symmetric key type.
     *
     * @param encryptedSymmetricKeyType the encrypted symmetric key type
     */
    public void setEncryptedSymmetricKeyType(String encryptedSymmetricKeyType) {
        this.encryptedSymmetricKeyType = encryptedSymmetricKeyType;
    }

    /**
     * Gets encrypted symmetric key value.
     *
     * @return the encrypted symmetric key value
     */
    public String getEncryptedSymmetricKeyValue() {
        return encryptedSymmetricKeyValue;
    }

    /**
     * Sets encrypted symmetric key value.
     *
     * @param encryptedSymmetricKeyValue the encrypted symmetric key value
     */
    public void setEncryptedSymmetricKeyValue(String encryptedSymmetricKeyValue) {
        this.encryptedSymmetricKeyValue = encryptedSymmetricKeyValue;
    }

    /**
     * Gets encrypted symmetric key algrithm mode.
     *
     * @return the encrypted symmetric key algrithm mode
     */
    public String getEncryptedSymmetricKeyAlgrithmMode() {
        return encryptedSymmetricKeyAlgrithmMode;
    }

    /**
     * Sets encrypted symmetric key algrithm mode.
     *
     * @param encryptedSymmetricKeyAlgrithmMode the encrypted symmetric key algrithm mode
     */
    public void setEncryptedSymmetricKeyAlgrithmMode(String encryptedSymmetricKeyAlgrithmMode) {
        this.encryptedSymmetricKeyAlgrithmMode = encryptedSymmetricKeyAlgrithmMode;
    }

    /**
     * Gets padding scheme.
     *
     * @return the padding scheme
     */
    public String getPaddingScheme() {
        return paddingScheme;
    }

    /**
     * Sets padding scheme.
     *
     * @param paddingScheme the padding scheme
     */
    public void setPaddingScheme(String paddingScheme) {
        this.paddingScheme = paddingScheme;
    }

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
}
