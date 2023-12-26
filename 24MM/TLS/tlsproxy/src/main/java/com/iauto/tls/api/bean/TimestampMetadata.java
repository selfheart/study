package com.iauto.tls.api.bean;

/**
 * The type Timestamp metadata.
 */
public class TimestampMetadata {
    private String filename;
    private Number version;
    private Number length;
    private Hash[] hashes;

    /**
     * Gets filename.
     *
     * @return the filename
     */
    public String getFilename() {
        return filename;
    }

    /**
     * Sets filename.
     *
     * @param filename the filename
     */
    public void setFilename(String filename) {
        this.filename = filename;
    }

    /**
     * Gets version.
     *
     * @return the version
     */
    public Number getVersion() {
        return version;
    }

    /**
     * Sets version.
     *
     * @param version the version
     */
    public void setVersion(Number version) {
        this.version = version;
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
}
