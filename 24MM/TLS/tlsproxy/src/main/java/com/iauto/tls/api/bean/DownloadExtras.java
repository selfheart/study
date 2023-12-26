package com.iauto.tls.api.bean;

/**
 * The type Download extras.
 */
public class DownloadExtras {
    /**
     * 通信方法
     */
    private String transportType;
    /**
     * ダウンロードサイズ
     */
    private Number downloadSize;
    /**
     * ダウンロード済みサイズ
     */
    private Number bytesDownloaded;
    /**
     * OEM エラー情報(複数)
     */
    private OemError[] oemErrors;

    /**
     * Gets transport type.
     *
     * @return the transport type
     */
    public String getTransportType() {
        return transportType;
    }

    /**
     * Sets transport type.
     *
     * @param transportType the transport type
     */
    public void setTransportType(String transportType) {
        this.transportType = transportType;
    }

    /**
     * Gets download size.
     *
     * @return the download size
     */
    public Number getDownloadSize() {
        return downloadSize;
    }

    /**
     * Sets download size.
     *
     * @param downloadSize the download size
     */
    public void setDownloadSize(Number downloadSize) {
        this.downloadSize = downloadSize;
    }

    /**
     * Gets bytes downloaded.
     *
     * @return the bytes downloaded
     */
    public Number getBytesDownloaded() {
        return bytesDownloaded;
    }

    /**
     * Sets bytes downloaded.
     *
     * @param bytesDownloaded the bytes downloaded
     */
    public void setBytesDownloaded(Number bytesDownloaded) {
        this.bytesDownloaded = bytesDownloaded;
    }

    /**
     * Get oem errors oem error [ ].
     *
     * @return the oem error [ ]
     */
    public OemError[] getOemErrors() {
        return oemErrors;
    }

    /**
     * Sets oem errors.
     *
     * @param oemErrors the oem errors
     */
    public void setOemErrors(OemError[] oemErrors) {
        this.oemErrors = oemErrors;
    }
}
