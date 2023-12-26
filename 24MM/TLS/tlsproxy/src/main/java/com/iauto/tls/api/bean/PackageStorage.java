package com.iauto.tls.api.bean;

/**
 * The type Package storage.
 */
public class PackageStorage {
    /**
     * パッケージ格納領域の使
     * 用済バイト数
     */
    private Number used;
    /**
     * パッケージ格納領域の使
     * 用可能なバイト数。
     */
    private Number available;

    /**
     * Gets used.
     *
     * @return the used
     */
    public Number getUsed() {
        return used;
    }

    /**
     * Sets used.
     *
     * @param used the used
     */
    public void setUsed(Number used) {
        this.used = used;
    }

    /**
     * Gets available.
     *
     * @return the available
     */
    public Number getAvailable() {
        return available;
    }

    /**
     * Sets available.
     *
     * @param available the available
     */
    public void setAvailable(Number available) {
        this.available = available;
    }
}
