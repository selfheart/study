package com.iauto.tls.api.response;

/**
 * The type Vehicle version manifest response.
 */
public class VehicleVersionManifestResponse {
    private TargetsMetadataAndFile[] targetsMetadata;
    private SnapshotCommonMetadata snapshotMetadata;
    private TimestampCommonMetadata timestampMetadata;
    private AugmentedCommonMetadata augmentedMetadata;

    /**
     * Get targets metadata targets metadata and file [ ].
     *
     * @return the targets metadata and file [ ]
     */
    public TargetsMetadataAndFile[] getTargetsMetadata() {
        return targetsMetadata;
    }

    /**
     * Sets targets metadata.
     *
     * @param targetsMetadata the targets metadata
     */
    public void setTargetsMetadata(TargetsMetadataAndFile[] targetsMetadata) {
        this.targetsMetadata = targetsMetadata;
    }

    /**
     * Gets snapshot metadata.
     *
     * @return the snapshot metadata
     */
    public SnapshotCommonMetadata getSnapshotMetadata() {
        return snapshotMetadata;
    }

    /**
     * Sets snapshot metadata.
     *
     * @param snapshotMetadata the snapshot metadata
     */
    public void setSnapshotMetadata(SnapshotCommonMetadata snapshotMetadata) {
        this.snapshotMetadata = snapshotMetadata;
    }

    /**
     * Gets timestamp metadata.
     *
     * @return the timestamp metadata
     */
    public TimestampCommonMetadata getTimestampMetadata() {
        return timestampMetadata;
    }

    /**
     * Sets timestamp metadata.
     *
     * @param timestampMetadata the timestamp metadata
     */
    public void setTimestampMetadata(TimestampCommonMetadata timestampMetadata) {
        this.timestampMetadata = timestampMetadata;
    }

    /**
     * Gets augmented metadata.
     *
     * @return the augmented metadata
     */
    public AugmentedCommonMetadata getAugmentedMetadata() {
        return augmentedMetadata;
    }

    /**
     * Sets augmented metadata.
     *
     * @param augmentedMetadata the augmented metadata
     */
    public void setAugmentedMetadata(AugmentedCommonMetadata augmentedMetadata) {
        this.augmentedMetadata = augmentedMetadata;
    }
}
