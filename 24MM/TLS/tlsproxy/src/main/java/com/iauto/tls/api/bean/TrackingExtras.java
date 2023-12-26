package com.iauto.tls.api.bean;

/**
 * The type Tracking extras.
 */
public class TrackingExtras {
    /**
     * ファイル名
     */
    private String filed;

    /**
     * ファイルサイズ
     */
    private Number fileSize;

    /**
     * ダウンロード日時
     */
    private TimeMs dlTime;

    /**
     * 通信方法
     */
    private String transportType;

    /**
     * ダウンロード結果
     */
    private String result;

    /**
     * Gets filed.
     *
     * @return the filed
     */
    public String getFiled() {
        return filed;
    }

    /**
     * Sets filed.
     *
     * @param filed the filed
     */
    public void setFiled(String filed) {
        this.filed = filed;
    }

    /**
     * Gets file size.
     *
     * @return the file size
     */
    public Number getFileSize() {
        return fileSize;
    }

    /**
     * Sets file size.
     *
     * @param fileSize the file size
     */
    public void setFileSize(Number fileSize) {
        this.fileSize = fileSize;
    }

    /**
     * Gets dl time.
     *
     * @return the dl time
     */
    public TimeMs getDlTime() {
        return dlTime;
    }

    /**
     * Sets dl time.
     *
     * @param dlTime the dl time
     */
    public void setDlTime(TimeMs dlTime) {
        this.dlTime = dlTime;
    }

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
     * Gets result.
     *
     * @return the result
     */
    public String getResult() {
        return result;
    }

    /**
     * Sets result.
     *
     * @param result the result
     */
    public void setResult(String result) {
        this.result = result;
    }
}
