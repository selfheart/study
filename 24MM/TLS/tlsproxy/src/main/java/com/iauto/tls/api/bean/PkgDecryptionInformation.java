package com.iauto.tls.api.bean;

/**
 * The type Pkg decryption information.
 */
public class PkgDecryptionInformation {
    private EncryptedDecryptionKey encryptedPkgDecryptionKey;
    private String encryptedPkgDecryptionKeyDecryptIv;
    private String encryptedPkgMacKey;
    private String encryptedPkgMacKeyDecryptIv;
    private String encryptedPkgMacKeyAlgo;
    private MetaDataInfo repropolicyMetadataInfo;
    private MetaDataInfo downloadMetadataInfo;

    /**
     * Gets encrypted pkg decryption key.
     *
     * @return the encrypted pkg decryption key
     */
    public EncryptedDecryptionKey getEncryptedPkgDecryptionKey() {
        return encryptedPkgDecryptionKey;
    }

    /**
     * Sets encrypted pkg decryption key.
     *
     * @param encryptedPkgDecryptionKey the encrypted pkg decryption key
     */
    public void setEncryptedPkgDecryptionKey(EncryptedDecryptionKey encryptedPkgDecryptionKey) {
        this.encryptedPkgDecryptionKey = encryptedPkgDecryptionKey;
    }

    /**
     * Gets encrypted pkg decryption key decrypt iv.
     *
     * @return the encrypted pkg decryption key decrypt iv
     */
    public String getEncryptedPkgDecryptionKeyDecryptIv() {
        return encryptedPkgDecryptionKeyDecryptIv;
    }

    /**
     * Sets encrypted pkg decryption key decrypt iv.
     *
     * @param encryptedPkgDecryptionKeyDecryptIv the encrypted pkg decryption key decrypt iv
     */
    public void setEncryptedPkgDecryptionKeyDecryptIv(String encryptedPkgDecryptionKeyDecryptIv) {
        this.encryptedPkgDecryptionKeyDecryptIv = encryptedPkgDecryptionKeyDecryptIv;
    }

    /**
     * Gets encrypted pkg mac key.
     *
     * @return the encrypted pkg mac key
     */
    public String getEncryptedPkgMacKey() {
        return encryptedPkgMacKey;
    }

    /**
     * Sets encrypted pkg mac key.
     *
     * @param encryptedPkgMacKey the encrypted pkg mac key
     */
    public void setEncryptedPkgMacKey(String encryptedPkgMacKey) {
        this.encryptedPkgMacKey = encryptedPkgMacKey;
    }

    /**
     * Gets encrypted pkg mac key decrypt iv.
     *
     * @return the encrypted pkg mac key decrypt iv
     */
    public String getEncryptedPkgMacKeyDecryptIv() {
        return encryptedPkgMacKeyDecryptIv;
    }

    /**
     * Sets encrypted pkg mac key decrypt iv.
     *
     * @param encryptedPkgMacKeyDecryptIv the encrypted pkg mac key decrypt iv
     */
    public void setEncryptedPkgMacKeyDecryptIv(String encryptedPkgMacKeyDecryptIv) {
        this.encryptedPkgMacKeyDecryptIv = encryptedPkgMacKeyDecryptIv;
    }

    /**
     * Gets encrypted pkg mac key algo.
     *
     * @return the encrypted pkg mac key algo
     */
    public String getEncryptedPkgMacKeyAlgo() {
        return encryptedPkgMacKeyAlgo;
    }

    /**
     * Sets encrypted pkg mac key algo.
     *
     * @param encryptedPkgMacKeyAlgo the encrypted pkg mac key algo
     */
    public void setEncryptedPkgMacKeyAlgo(String encryptedPkgMacKeyAlgo) {
        this.encryptedPkgMacKeyAlgo = encryptedPkgMacKeyAlgo;
    }

    /**
     * Gets repropolicy metadata info.
     *
     * @return the repropolicy metadata info
     */
    public MetaDataInfo getRepropolicyMetadataInfo() {
        return repropolicyMetadataInfo;
    }

    /**
     * Sets repropolicy metadata info.
     *
     * @param repropolicyMetadataInfo the repropolicy metadata info
     */
    public void setRepropolicyMetadataInfo(MetaDataInfo repropolicyMetadataInfo) {
        this.repropolicyMetadataInfo = repropolicyMetadataInfo;
    }

    /**
     * Gets download metadata info.
     *
     * @return the download metadata info
     */
    public MetaDataInfo getDownloadMetadataInfo() {
        return downloadMetadataInfo;
    }

    /**
     * Sets download metadata info.
     *
     * @param downloadMetadataInfo the download metadata info
     */
    public void setDownloadMetadataInfo(MetaDataInfo downloadMetadataInfo) {
        this.downloadMetadataInfo = downloadMetadataInfo;
    }
}
