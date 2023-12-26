package com.iauto.tls.api.response;

/**
 * The type Error code.
 */
public class ErrorCode {
    /**
     * エラーコード
     */
    private Number code;
    /**
     * ユーザ向けのエラー説明
     */
    private String description;

    /**
     * Gets code.
     *
     * @return the code
     */
    public Number getCode() {
        return code;
    }

    /**
     * Sets code.
     *
     * @param code the code
     */
    public void setCode(Number code) {
        this.code = code;
    }

    /**
     * Gets description.
     *
     * @return the description
     */
    public String getDescription() {
        return description;
    }

    /**
     * Sets description.
     *
     * @param description the description
     */
    public void setDescription(String description) {
        this.description = description;
    }
}
