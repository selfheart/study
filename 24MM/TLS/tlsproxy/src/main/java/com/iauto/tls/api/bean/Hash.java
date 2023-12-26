package com.iauto.tls.api.bean;

/**
 * The type Hash.
 */
public class Hash {
    /**
     * ハッシュ関数
     */
    private String function = "SHA256";

    /**
     * Base16 エンコードされた
     * ダイジェスト
     */
    private String digest;

    /**
     * Gets function.
     *
     * @return the function
     */
    public String getFunction() {
        return function;
    }

    /**
     * Sets function.
     *
     * @param function the function
     */
    public void setFunction(String function) {
        this.function = function;
    }

    /**
     * Gets digest.
     *
     * @return the digest
     */
    public String getDigest() {
        return digest;
    }

    /**
     * Sets digest.
     *
     * @param digest the digest
     */
    public void setDigest(String digest) {
        this.digest = digest;
    }
}
