package com.iauto.tls.api.bean;

/**
 * The type Snapshot metadata.
 */
public class SnapshotMetadata {
    private SnapshotMetadataFiles[] snapshotMetadataFiles;

    /**
     * Get snapshot metadata files snapshot metadata files [ ].
     *
     * @return the snapshot metadata files [ ]
     */
    public SnapshotMetadataFiles[] getSnapshotMetadataFiles() {
        return snapshotMetadataFiles;
    }

    /**
     * Sets snapshot metadata files.
     *
     * @param snapshotMetadataFiles the snapshot metadata files
     */
    public void setSnapshotMetadataFiles(SnapshotMetadataFiles[] snapshotMetadataFiles) {
        this.snapshotMetadataFiles = snapshotMetadataFiles;
    }
}
