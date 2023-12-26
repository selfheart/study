package com.iauto.fa.api;

import com.iauto.tls.api.OtaClientProxy;
import com.iauto.tls.api.requestbody.GetActionRequest;
import com.iauto.tls.api.requestbody.GetCampaignStatus;
import com.iauto.tls.api.requestbody.LightVehicleVersionManifest;
import com.iauto.tls.api.requestbody.VsmDeviceEvents;

import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;

/**
 * The type Ota client proxy test.
 */
public class OtaClientProxyTest {

    /**
     * Campaign status notification.
     */
//不需要等回复示例
    public void campaignStatusNotification() {
        VsmDeviceEvents vsmDeviceEvents = null;
        OtaClientProxy otaClientProxy = OtaClientProxy.getInstance();
        otaClientProxy.setLocalPath("files/");
        CompletableFuture<String> futureResult = new CompletableFuture<>();
        otaClientProxy.campaignStatusNotification(vsmDeviceEvents, futureResult);
        System.out.println("finish");
    }

    /**
     * Ota action acquisition.
     */
//同步等回复示例
    public void otaActionAcquisition() {
        GetActionRequest getActionRequest = null;
        OtaClientProxy otaClientProxy = OtaClientProxy.getInstance();
        otaClientProxy.setLocalPath("files/");
        CompletableFuture<String> futureResult = new CompletableFuture<>();
        otaClientProxy.otaActionAcquisition(getActionRequest,futureResult);

        try {
            //等到回复后再根据回复进行下一步
            String ret = futureResult.get();
            //handle(ret);

        } catch (ExecutionException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }


    }

    /**
     * Vehicle configuration information upload.
     */
//异步等回复示例
    public void vehicleConfigurationInformationUpload() {
        LightVehicleVersionManifest lightVehicleVersionManifest = null;
        OtaClientProxy otaClientProxy = OtaClientProxy.getInstance();
        otaClientProxy.setLocalPath("files/");
        CompletableFuture<String> futureResult = new CompletableFuture<>();
        otaClientProxy.vehicleConfigurationInformationUpload(lightVehicleVersionManifest,futureResult);
        futureResult.thenAccept(result->{     //thenAccept 指定回调函数handleResult，futureResult的异步处理完成后会执行handleResult，未完成之前不会阻塞其他操作 otherOperations()的执行
            handleResult();
        });
        /**
         * 其他操作 otherOperations()
         */
        System.out.println("finished");

    }

    /**
     * Vehicle configuration information upload future result.
     *
     * @param futureResult the future result
     */
    public void vehicleConfigurationInformationUploadFutureResult(CompletableFuture<String> futureResult){
        try {
            futureResult.get();
        } catch (ExecutionException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

    }

    /**
     * Handle result.
     */
    public void handleResult(){

    }


    /**
     * The entry point of application.
     *
     * @param args the input arguments
     */
    public static void main(String[] args) {
        new OtaClientProxyTest().campaignStatusNotification();
    }
}
