package com.iauto.tls.api.bean;

/**
 * The type Sync system assy.
 */
public class SyncSystemASSY {
    private String reportId;
    private PkgDecryptionInformation pkgDecryptionInfo;

    /**
     * Gets report id.
     *
     * @return the report id
     */
    public String getReportId() {
        return reportId;
    }

    /**
     * Sets report id.
     *
     * @param reportId the report id
     */
    public void setReportId(String reportId) {
        this.reportId = reportId;
    }

    /**
     * Gets pkg decryption info.
     *
     * @return the pkg decryption info
     */
    public PkgDecryptionInformation getPkgDecryptionInfo() {
        return pkgDecryptionInfo;
    }

    /**
     * Sets pkg decryption info.
     *
     * @param pkgDecryptionInfo the pkg decryption info
     */
    public void setPkgDecryptionInfo(PkgDecryptionInformation pkgDecryptionInfo) {
        this.pkgDecryptionInfo = pkgDecryptionInfo;
    }
}
