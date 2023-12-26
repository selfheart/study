package com.iauto.tls.api.bean;

import com.iauto.tls.api.response.ErrorCode;

/**
 * The type Ecu information.
 */
public class EcuInformation {
    /**
     * ECU 識別 ID
     */
    private String targetId;
    /**
     * ECU ソフトウェア情報識別
     * ID
     */
    private String ecuSoftwareId;
    /**
     * "success" ：完了(成功)、
     * "failure"：完了(失敗)
     */
    private String updateStatus;
    /**
     * エラー情報(複数)
     */
    private ErrorCode[] errorCodes;
    /**
     * 通常は"ota"
     */
    private String updateMethod;

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

    /**
     * Gets ecu software id.
     *
     * @return the ecu software id
     */
    public String getEcuSoftwareId() {
        return ecuSoftwareId;
    }

    /**
     * Sets ecu software id.
     *
     * @param ecuSoftwareId the ecu software id
     */
    public void setEcuSoftwareId(String ecuSoftwareId) {
        this.ecuSoftwareId = ecuSoftwareId;
    }

    /**
     * Gets update status.
     *
     * @return the update status
     */
    public String getUpdateStatus() {
        return updateStatus;
    }

    /**
     * Sets update status.
     *
     * @param updateStatus the update status
     */
    public void setUpdateStatus(String updateStatus) {
        this.updateStatus = updateStatus;
    }

    /**
     * Get error codes error code [ ].
     *
     * @return the error code [ ]
     */
    public ErrorCode[] getErrorCodes() {
        return errorCodes;
    }

    /**
     * Sets error codes.
     *
     * @param errorCodes the error codes
     */
    public void setErrorCodes(ErrorCode[] errorCodes) {
        this.errorCodes = errorCodes;
    }

    /**
     * Gets update method.
     *
     * @return the update method
     */
    public String getUpdateMethod() {
        return updateMethod;
    }

    /**
     * Sets update method.
     *
     * @param updateMethod the update method
     */
    public void setUpdateMethod(String updateMethod) {
        this.updateMethod = updateMethod;
    }
}
