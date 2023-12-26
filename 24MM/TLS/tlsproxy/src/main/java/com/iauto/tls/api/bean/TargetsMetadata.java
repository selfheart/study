package com.iauto.tls.api.bean;

/**
 * The type Targets metadata.
 */
public class TargetsMetadata {
    private TargetAndCustom[] targets;
    private Bin[] bins;

    /**
     * Get targets target and custom [ ].
     *
     * @return the target and custom [ ]
     */
    public TargetAndCustom[] getTargets() {
        return targets;
    }

    /**
     * Sets targets.
     *
     * @param targets the targets
     */
    public void setTargets(TargetAndCustom[] targets) {
        this.targets = targets;
    }

    /**
     * Get bins bin [ ].
     *
     * @return the bin [ ]
     */
    public Bin[] getBins() {
        return bins;
    }

    /**
     * Sets bins.
     *
     * @param bins the bins
     */
    public void setBins(Bin[] bins) {
        this.bins = bins;
    }
}
