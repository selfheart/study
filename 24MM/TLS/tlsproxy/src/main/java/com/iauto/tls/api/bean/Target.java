package com.iauto.tls.api.bean;

/**
 * The type Target.
 */
public class Target {
    private String fileName;
    private Number length;
    private Hash[] hashes;
    private String fileDownloadUrl;

    /**
     * Gets file name.
     *
     * @return the file name
     */
    public String getFileName() {
        return fileName;
    }

    /**
     * Sets file name.
     *
     * @param fileName the file name
     */
    public void setFileName(String fileName) {
        this.fileName = fileName;
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
     * Get hashes hash [ ].
     *
     * @return the hash [ ]
     */
    public Hash[] getHashes() {
        return hashes;
    }

    /**
     * Sets hashes.
     *
     * @param hashes the hashes
     */
    public void setHashes(Hash[] hashes) {
        this.hashes = hashes;
    }

    /**
     * Gets file download url.
     *
     * @return the file download url
     */
    public String getFileDownloadUrl() {
        return fileDownloadUrl;
    }

    /**
     * Sets file download url.
     *
     * @param fileDownloadUrl the file download url
     */
    public void setFileDownloadUrl(String fileDownloadUrl) {
        this.fileDownloadUrl = fileDownloadUrl;
    }
}
