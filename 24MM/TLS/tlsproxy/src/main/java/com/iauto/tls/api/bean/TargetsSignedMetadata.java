package com.iauto.tls.api.bean;

/**
 * The type Targets signed metadata.
 */
public class TargetsSignedMetadata {
    /**
     * ロールタイプ
     * targets,snapshot,timesta
     * mp,root の何れかを設定
     * TargetMetadata：targets
     * SnampshotMetadata ：
     * snapshot
     * TimestampMetadata ：
     * timestamp
     * AugmentMetadata ：
     * targets
     */
    private String type;

    /**
     * メタデータ有効期限(UTC)
     */
    private Number expire;

    /**
     * メタデータバージョン
     */
    private Number version;

    /**
     * 各メタデータのデータ部が
     * 設定される。
     */
    private TargetsMetadata body;

    /**
     * Gets type.
     *
     * @return the type
     */
    public String getType() {
        return type;
    }

    /**
     * Sets type.
     *
     * @param type the type
     */
    public void setType(String type) {
        this.type = type;
    }

    /**
     * Gets expire.
     *
     * @return the expire
     */
    public Number getExpire() {
        return expire;
    }

    /**
     * Sets expire.
     *
     * @param expire the expire
     */
    public void setExpire(Number expire) {
        this.expire = expire;
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
     * Gets body.
     *
     * @return the body
     */
    public TargetsMetadata getBody() {
        return body;
    }

    /**
     * Sets body.
     *
     * @param body the body
     */
    public void setBody(TargetsMetadata body) {
        this.body = body;
    }
}
