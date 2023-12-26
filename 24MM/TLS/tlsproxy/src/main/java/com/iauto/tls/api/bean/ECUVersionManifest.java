package com.iauto.tls.api.bean;

/**
 * The type Ecu version manifest.
 */
public class ECUVersionManifest {
    /**
     * アクティブな ECU メモリバ
     * ンクの情報。
     */
    private TargetBank activeTarget;
    /**
     * ターゲット ID
     */
    private String targetId;

    /**
     * Gets active target.
     *
     * @return the active target
     */
    public TargetBank getActiveTarget() {
        return activeTarget;
    }

    /**
     * Sets active target.
     *
     * @param activeTarget the active target
     */
    public void setActiveTarget(TargetBank activeTarget) {
        this.activeTarget = activeTarget;
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
