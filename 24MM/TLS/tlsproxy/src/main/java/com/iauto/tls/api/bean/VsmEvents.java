package com.iauto.tls.api.bean;

/**
 * The type Vsm events.
 */
public class VsmEvents {
    /**
     * UUID
     */
    private String eventsUuid;
    /**
     * 状態通知イベント
     * C_01_Req:rmState
     * C_09_Req:tracking
     * C_10_Req:notification
     * C_12_Req:download
     */
    private String eventsType = "rmState";
    /**
     * 固定
     */
    private String eventMode = "active";
    /**
     * 状態情報詳細(only C_01_Req)
     */
    private RmStateExtra rmStateExtras;

    /**
     * トラッキング通知詳細(only C_09_Req)
     */
    private TrackingExtras trackingExtras;

    /**
     * ダウンロード情報詳細(only C_12_Req)
     */
    private DownloadExtras downloadExtras;

    /**
     * インストール進捗情報詳細(only C_20_Req)
     */
    private InstallProgressExtras installProgressExtras;

    /**
     * インストール完了情報詳細(only C_21_Req)
     */
    private InstallCompleteExtras installCompleteExtras;

    /**
     * アクティベート進捗情報詳細(only C_23_Req)
     */
    private ActivateProgressExtras activateProgressExtras;

    /**
     * アクティベート情報詳細(only C_23_Req)
     */
    private ActivationExtras activationExtras;

    /**
     * エラー情報
     */
    private OemError errorExtras;

    /**
     * ダウンロード開始時刻(only C_12_Req)
     */
    private TimeMs timeStarted;

    /**
     * イベント発生日時
     */
    private TimeMs currentTime;
    /**
     * 固定(C_01_Req,C_09_Req)
     * "accepted","declined","postponed","skipped"
     */
    private String status = "success";
    /**
     * OTA センタで同一車両に
     * 対する同一キャンペーンを
     * 複数回適用しているか否
     * かを判断するためのユニ
     * ークな ID。
     * 車両側で任意に指定し、
     * 同一キャンペーン適用シー
     * ケンス内は同じ値を使用
     * する。
     */
    private String attemptId;
    /**
     * 条件付き項目
     * 下記以外の場合は必須
     * ・車両構成情報アップロー
     * ド応答受信前の場合
     * 車両構成情報アップロード
     * 応答以降は、車両構成情
     * 報アップロードで指定され
     * ているreportId情報を格納
     * する。
     */
    private String reportId;

    /**
     * Gets events uuid.
     *
     * @return the events uuid
     */
    public String getEventsUuid() {
        return eventsUuid;
    }

    /**
     * Sets events uuid.
     *
     * @param eventsUuid the events uuid
     */
    public void setEventsUuid(String eventsUuid) {
        this.eventsUuid = eventsUuid;
    }

    /**
     * Gets events type.
     *
     * @return the events type
     */
    public String getEventsType() {
        return eventsType;
    }

    /**
     * Sets events type.
     *
     * @param eventsType the events type
     */
    public void setEventsType(String eventsType) {
        this.eventsType = eventsType;
    }

    /**
     * Gets event mode.
     *
     * @return the event mode
     */
    public String getEventMode() {
        return eventMode;
    }

    /**
     * Sets event mode.
     *
     * @param eventMode the event mode
     */
    public void setEventMode(String eventMode) {
        this.eventMode = eventMode;
    }

    /**
     * Gets rm state extras.
     *
     * @return the rm state extras
     */
    public RmStateExtra getRmStateExtras() {
        return rmStateExtras;
    }

    /**
     * Sets rm state extras.
     *
     * @param rmStateExtras the rm state extras
     */
    public void setRmStateExtras(RmStateExtra rmStateExtras) {
        this.rmStateExtras = rmStateExtras;
    }

    /**
     * Gets tracking extras.
     *
     * @return the tracking extras
     */
    public TrackingExtras getTrackingExtras() {
        return trackingExtras;
    }

    /**
     * Sets tracking extras.
     *
     * @param trackingExtras the tracking extras
     */
    public void setTrackingExtras(TrackingExtras trackingExtras) {
        this.trackingExtras = trackingExtras;
    }

    /**
     * Gets download extras.
     *
     * @return the download extras
     */
    public DownloadExtras getDownloadExtras() {
        return downloadExtras;
    }

    /**
     * Sets download extras.
     *
     * @param downloadExtras the download extras
     */
    public void setDownloadExtras(DownloadExtras downloadExtras) {
        this.downloadExtras = downloadExtras;
    }

    /**
     * Gets install progress extras.
     *
     * @return the install progress extras
     */
    public InstallProgressExtras getInstallProgressExtras() {
        return installProgressExtras;
    }

    /**
     * Sets install progress extras.
     *
     * @param installProgressExtras the install progress extras
     */
    public void setInstallProgressExtras(InstallProgressExtras installProgressExtras) {
        this.installProgressExtras = installProgressExtras;
    }

    /**
     * Gets install complete extras.
     *
     * @return the install complete extras
     */
    public InstallCompleteExtras getInstallCompleteExtras() {
        return installCompleteExtras;
    }

    /**
     * Sets install complete extras.
     *
     * @param installCompleteExtras the install complete extras
     */
    public void setInstallCompleteExtras(InstallCompleteExtras installCompleteExtras) {
        this.installCompleteExtras = installCompleteExtras;
    }

    /**
     * Gets activate progress extras.
     *
     * @return the activate progress extras
     */
    public ActivateProgressExtras getActivateProgressExtras() {
        return activateProgressExtras;
    }

    /**
     * Sets activate progress extras.
     *
     * @param activateProgressExtras the activate progress extras
     */
    public void setActivateProgressExtras(ActivateProgressExtras activateProgressExtras) {
        this.activateProgressExtras = activateProgressExtras;
    }

    /**
     * Gets activation extras.
     *
     * @return the activation extras
     */
    public ActivationExtras getActivationExtras() {
        return activationExtras;
    }

    /**
     * Sets activation extras.
     *
     * @param activationExtras the activation extras
     */
    public void setActivationExtras(ActivationExtras activationExtras) {
        this.activationExtras = activationExtras;
    }

    /**
     * Gets error extras.
     *
     * @return the error extras
     */
    public OemError getErrorExtras() {
        return errorExtras;
    }

    /**
     * Sets error extras.
     *
     * @param errorExtras the error extras
     */
    public void setErrorExtras(OemError errorExtras) {
        this.errorExtras = errorExtras;
    }

    /**
     * Gets time started.
     *
     * @return the time started
     */
    public TimeMs getTimeStarted() {
        return timeStarted;
    }

    /**
     * Sets time started.
     *
     * @param timeStarted the time started
     */
    public void setTimeStarted(TimeMs timeStarted) {
        this.timeStarted = timeStarted;
    }

    /**
     * Gets current time.
     *
     * @return the current time
     */
    public TimeMs getCurrentTime() {
        return currentTime;
    }

    /**
     * Sets current time.
     *
     * @param currentTime the current time
     */
    public void setCurrentTime(TimeMs currentTime) {
        this.currentTime = currentTime;
    }

    /**
     * Gets status.
     *
     * @return the status
     */
    public String getStatus() {
        return status;
    }

    /**
     * Sets status.
     *
     * @param status the status
     */
    public void setStatus(String status) {
        this.status = status;
    }

    /**
     * Gets attempt id.
     *
     * @return the attempt id
     */
    public String getAttemptId() {
        return attemptId;
    }

    /**
     * Sets attempt id.
     *
     * @param attemptId the attempt id
     */
    public void setAttemptId(String attemptId) {
        this.attemptId = attemptId;
    }

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
}
