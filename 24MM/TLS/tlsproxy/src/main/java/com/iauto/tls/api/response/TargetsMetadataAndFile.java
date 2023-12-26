package com.iauto.tls.api.response;

/**
 * The type Targets metadata and file.
 */
public class TargetsMetadataAndFile {
    /**
     * ターゲットメタデータファイ
     * ル名
     */
    private String fileName;
    private TargetsCommonMetadata metadata;

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
     * Gets metadata.
     *
     * @return the metadata
     */
    public TargetsCommonMetadata getMetadata() {
        return metadata;
    }

    /**
     * Sets metadata.
     *
     * @param metadata the metadata
     */
    public void setMetadata(TargetsCommonMetadata metadata) {
        this.metadata = metadata;
    }
}
