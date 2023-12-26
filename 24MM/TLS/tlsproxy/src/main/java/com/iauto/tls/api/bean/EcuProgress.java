package com.iauto.tls.api.bean;

/**
 * The type Ecu progress.
 */
public class EcuProgress {
    /**
     * 現在の ECU の全パッケー
     * ジサイズ
     */
    private Number totalBytes;
    /**
     * 現在の ECU の更新済み
     * サイズ
     */
    private Number bytesProcessed;
    /**
     * 現在更新中の ECU のター
     * ゲット ID
     */
    private String targetId;

    /**
     * Gets total bytes.
     *
     * @return the total bytes
     */
    public Number getTotalBytes() {
        return totalBytes;
    }

    /**
     * Sets total bytes.
     *
     * @param totalBytes the total bytes
     */
    public void setTotalBytes(Number totalBytes) {
        this.totalBytes = totalBytes;
    }

    /**
     * Gets bytes processed.
     *
     * @return the bytes processed
     */
    public Number getBytesProcessed() {
        return bytesProcessed;
    }

    /**
     * Sets bytes processed.
     *
     * @param bytesProcessed the bytes processed
     */
    public void setBytesProcessed(Number bytesProcessed) {
        this.bytesProcessed = bytesProcessed;
    }

    /**
     * Gets target id.
     *
     * @return the target id
     */
    public String getTargetId() {
        return targetId;
    }

    /**
     * Sets target id.
     *
     * @param targetId the target id
     */
    public void setTargetId(String targetId) {
        this.targetId = targetId;
    }
}
