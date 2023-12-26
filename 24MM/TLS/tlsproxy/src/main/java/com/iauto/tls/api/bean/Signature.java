package com.iauto.tls.api.bean;

/**
 * The type Signature.
 */
public class Signature {
    /**
     * 署名検証のために必要な
     * 公開鍵 ID (*1)
     */
    private String keyid;
    /**
     * 署名アルゴリズム
     */
    private String method = "NONEwithECDSA";
    /**
     * 署名部分のハッシュ値情
     * 報
     */
    private Hash hash;
    /**
     * 署名値を Base16 でエンコ
     * ードした文字列
     */
    private String value;

    /**
     * Gets keyid.
     *
     * @return the keyid
     */
    public String getKeyid() {
        return keyid;
    }

    /**
     * Sets keyid.
     *
     * @param keyid the keyid
     */
    public void setKeyid(String keyid) {
        this.keyid = keyid;
    }

    /**
     * Gets method.
     *
     * @return the method
     */
    public String getMethod() {
        return method;
    }

    /**
     * Sets method.
     *
     * @param method the method
     */
    public void setMethod(String method) {
        this.method = method;
    }

    /**
     * Gets hash.
     *
     * @return the hash
     */
    public Hash getHash() {
        return hash;
    }

    /**
     * Sets hash.
     *
     * @param hash the hash
     */
    public void setHash(Hash hash) {
        this.hash = hash;
    }

    /**
     * Gets value.
     *
     * @return the value
     */
    public String getValue() {
        return value;
    }

    /**
     * Sets value.
     *
     * @param value the value
     */
    public void setValue(String value) {
        this.value = value;
    }
}
