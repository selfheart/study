package com.iauto.tls.api.response;

import com.iauto.tls.api.bean.Signature;
import com.iauto.tls.api.bean.SnapshotSignedMetadata;

/**
 * The type Snapshot common metadata.
 */
public class SnapshotCommonMetadata {
    private SnapshotSignedMetadata signed;
    /**
     * 署名部
     */
    private Signature[] signatures;
}
