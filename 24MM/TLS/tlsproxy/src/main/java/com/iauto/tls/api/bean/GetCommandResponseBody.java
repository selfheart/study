package com.iauto.tls.api.bean;

/**
 * The type Get command response body.
 */
public class GetCommandResponseBody {
    /**
     * VIN 番号
     */
    private String vin;
    /**
     * キャンペーン ID
     */
    private Number commandId;

    /**
     * UpldVehclCnfg：個車構成
     * アップロード要求
     * UpldVehclLog：車両 ログ
     * アップロード要求
     * CancelCampaign ： 指 定
     * キャンペーンの取り消し要
     * 求
     * StartCampaign：指定キャ
     * ンペーンの開始要求。
     * InstallCampaign ：指定 キ
     * ャンペーンのインストール
     * を開始要求
     * ExtCancelCampaign ： 外
     * 部サービスからのキャンペ
     * ーンキャンセル通知
     * GetOtaAction ： action 情
     * 報取得
     */
    private String command;

    /**
     * 任意項目。
     * CommandId が下記の場
     * 合のみ指定
     * ・CancelCampaign
     * ・StartCampaign
     * ・InstallCampaign
     * ・ExtCancelCampaign
     */
    private Number campaignId;

    /**
     * Gets vin.
     *
     * @return the vin
     */
    public String getVin() {
        return vin;
    }

    /**
     * Sets vin.
     *
     * @param vin the vin
     */
    public void setVin(String vin) {
        this.vin = vin;
    }

    /**
     * Gets command id.
     *
     * @return the command id
     */
    public Number getCommandId() {
        return commandId;
    }

    /**
     * Sets command id.
     *
     * @param commandId the command id
     */
    public void setCommandId(Number commandId) {
        this.commandId = commandId;
    }

    /**
     * Gets command.
     *
     * @return the command
     */
    public String getCommand() {
        return command;
    }

    /**
     * Sets command.
     *
     * @param command the command
     */
    public void setCommand(String command) {
        this.command = command;
    }

    /**
     * Gets campaign id.
     *
     * @return the campaign id
     */
    public Number getCampaignId() {
        return campaignId;
    }

    /**
     * Sets campaign id.
     *
     * @param campaignId the campaign id
     */
    public void setCampaignId(Number campaignId) {
        this.campaignId = campaignId;
    }
}
