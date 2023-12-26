package com.iauto.tls.api.bean;

/**
 * The type Hmi message.
 */
public class HmiMessage {
    private String reportId;
    private String url;
    private Number length;
    private String digest;
    private String digestAlgorithm;
    private Number decompressedLength;

    /**
     * Gets report id.
     *
     * @return the report id
     */
    public String getReportId() {
        return reportId;
    }

    /**
     * Sets report id.
     *
     * @param reportId the report id
     */
    public void setReportId(String reportId) {
        this.reportId = reportId;
    }

    /**
     * Gets url.
     *
     * @return the url
     */
    public String getUrl() {
        return url;
    }

    /**
     * Sets url.
     *
     * @param url the url
     */
    public void setUrl(String url) {
        this.url = url;
    }

    /**
     * Gets length.
     *
     * @return the length
     */
    public Number getLength() {
        return length;
    }

    /**
     * Sets length.
     *
     * @param length the length
     */
    public void setLength(Number length) {
        this.length = length;
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

    /**
     * Gets digest algorithm.
     *
     * @return the digest algorithm
     */
    public String getDigestAlgorithm() {
        return digestAlgorithm;
    }

    /**
     * Sets digest algorithm.
     *
     * @param digestAlgorithm the digest algorithm
     */
    public void setDigestAlgorithm(String digestAlgorithm) {
        this.digestAlgorithm = digestAlgorithm;
    }

    /**
     * Gets decompressed length.
     *
     * @return the decompressed length
     */
    public Number getDecompressedLength() {
        return decompressedLength;
    }

    /**
     * Sets decompressed length.
     *
     * @param decompressedLength the decompressed length
     */
    public void setDecompressedLength(Number decompressedLength) {
        this.decompressedLength = decompressedLength;
    }
}
