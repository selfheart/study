package com.iauto.tls.api.bean;

/**
 * The type Snapshot metadata files.
 */
public class SnapshotMetadataFiles {
    private String filename;
    private Number version;

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
}
