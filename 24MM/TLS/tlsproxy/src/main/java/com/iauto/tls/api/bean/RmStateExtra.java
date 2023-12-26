package com.iauto.tls.api.bean;

/**
 * The type Rm state extra.
 */
public class RmStateExtra {
    /**
     * "IDLE"：OTA 更新無し
     * "SYNCSING" ： 車両情報取得中
     * "waitCampaignUserAcceptance "：キャンペーン承諾待ち
     * "waitPackageDlUserAcceptance"：ダウンロード承諾待ち
     * "packageDownload" ： ダウンロード中
     * "waitInstallUserAcceptance"：インストール承諾待ち
     * "reprog"：インストール中
     * "waitActivateUserAcceptance"：アクティベート承諾待ち
     * "activating"：アクティベート中
     * "systemSyncCheckComplete"：バージョンチェック中
     */
    private String rmVehState;
    /**
     * 車両 OTA 全体状況
     */
    private String stateScope = "vehicle";

    /**
     * Gets rm veh state.
     *
     * @return the rm veh state
     */
    public String getRmVehState() {
        return rmVehState;
    }

    /**
     * Sets rm veh state.
     *
     * @param rmVehState the rm veh state
     */
    public void setRmVehState(String rmVehState) {
        this.rmVehState = rmVehState;
    }

    /**
     * Gets state scope.
     *
     * @return the state scope
     */
    public String getStateScope() {
        return stateScope;
    }

    /**
     * Sets state scope.
     *
     * @param stateScope the state scope
     */
    public void setStateScope(String stateScope) {
        this.stateScope = stateScope;
    }
}
