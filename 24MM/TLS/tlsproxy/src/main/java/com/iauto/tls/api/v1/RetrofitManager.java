package com.iauto.tls.api.v1;

import static com.iauto.tls.api.common.LogDef.LOG_ID_RETROFIT_MANAGER;

import android.util.Pair;

import com.google.gson.Gson;
import com.iauto.tls.api.DownloadHandle.DownloadManager;
import com.iauto.tls.api.common.GlobalDef;
import com.iauto.tls.api.common.Log;
import com.iauto.tls.api.net.RetryInterceptor;
import com.iauto.tls.api.requestbody.CommonMetadata;
import com.iauto.tls.api.requestbody.CompactVehicleManifest;
import com.iauto.tls.api.requestbody.GetActionRequest;
import com.iauto.tls.api.requestbody.GetCampaignStatus;
import com.iauto.tls.api.requestbody.LightVehicleVersionManifest;
import com.iauto.tls.api.requestbody.MultiPartCompleteUploadRequestMetadata;
import com.iauto.tls.api.requestbody.VsmDeviceEvents;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.zip.GZIPInputStream;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

import io.reactivex.rxjava3.observers.DisposableObserver;
import io.reactivex.rxjava3.schedulers.Schedulers;
import okhttp3.OkHttpClient;
import okhttp3.RequestBody;
import okhttp3.ResponseBody;
import okhttp3.logging.HttpLoggingInterceptor;
import retrofit2.Retrofit;
import retrofit2.adapter.rxjava3.RxJava3CallAdapterFactory;
import retrofit2.converter.gson.GsonConverterFactory;
import retrofit2.converter.scalars.ScalarsConverterFactory;

/**
 * The type Retrofit manager.
 */
public class RetrofitManager {
    private OkHttpClient okHttpClient;
    private Retrofit retrofit;
    private RequestInterface requestInterface;
    private Gson gson;
    private String errorMessage;
    private String ContentEncoding = "";


    /**
     * Instantiates a new Retrofit manager.
     */
    public RetrofitManager() {
        HttpLoggingInterceptor loggingInterceptor = new HttpLoggingInterceptor(message -> {
            // 打印retrofit日志
            Log.d(LOG_ID_RETROFIT_MANAGER, "RetrofitBack =" + message);
        });
        loggingInterceptor.setLevel(HttpLoggingInterceptor.Level.BODY);

        OkHttpClient.Builder okHttpClientBuilder= new OkHttpClient.Builder();

        // 配置 SSL Socket Factory，用于支持 HTTPS 请求
        Pair<SSLSocketFactory, TrustManager[]> result = setTwoWayCertificates();
        TrustManager[] trustManagers = result.second;
        okHttpClientBuilder.sslSocketFactory(result.first, (X509TrustManager)trustManagers[0]);

        okHttpClient = okHttpClientBuilder
                //参照：サーバのプッシュ予約から配信完了までの時間 を以下とすること ・Typical：10～19秒 ・Maximum：1分
                .connectTimeout(10, TimeUnit.SECONDS) // 连接超时时间//TODO: 待确定
                .readTimeout(60, TimeUnit.SECONDS) // 读取超时时间
                .writeTimeout(30, TimeUnit.SECONDS) // 写入超时时间//TODO: 待确定
                .addInterceptor(new RetryInterceptor())//拦截器中实现全局的超时错误处理
//                .connectionSpecs(Collections.singletonList(new ConnectionSpec.Builder(ConnectionSpec.MODERN_TLS)
//                        .tlsVersions(TlsVersion.TLS_1_2)
//                        .cipherSuites("TLS_RSA_WITH_AES_128_CBC_SHA")
//                        .build()))
                .build();
        retrofit = new Retrofit.Builder()
                .baseUrl(GlobalDef.getBaseUrl())
                .client(okHttpClient)
                .addConverterFactory(ScalarsConverterFactory.create())
                .addConverterFactory(GsonConverterFactory.create())
                .addCallAdapterFactory(RxJava3CallAdapterFactory.create())
                .build();

        requestInterface = retrofit.create(RequestInterface.class);
        gson = new Gson();
    }

    /**
     * Send request.
     *
     * @param requestName   the request name
     * @param userAgent     the user agent
     * @param vin           the vin
     * @param date          the date
     * @param requestObject the request object
     * @param futureResult  the future result
     * @return
     */
    public void sendRequest(String requestName, String userAgent, String vin, String date, Object requestObject, CompletableFuture<String> futureResult) {
        Log.d(LOG_ID_RETROFIT_MANAGER, "sendRequest-requestName = "+requestName);

//        final AtomicReference<String> responseJson = new AtomicReference<>("");

        String responseJson = null;

        switch (requestName) {
            case "campaignStatusNotification": //(C_01_Req)Campaign Status Notification
                requestInterface.campaignStatusNotification(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "otaActionAcquisition": //(C_03_Req)OTA Action acquisition
                requestInterface.otaActionAcquisition(userAgent, vin, date, (GetActionRequest) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "vehicleConfigurationInformationDigest": //(C_04_Req)Vehicle configuration information digest
                requestInterface.vehicleConfigurationInformationDigest(userAgent, vin, date, (CompactVehicleManifest) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "vehicleConfigurationInformationUpload": //(C_05_Req)Vehicle configuration information upload
                requestInterface.vehicleConfigurationInformationUpload(userAgent, vin, date, (LightVehicleVersionManifest) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "hmiDataAcquisition":  //(C_08_Req)HMI Data acquisition
                requestInterface.hmiDataAcquisition(userAgent, vin, date)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "userAcceptanceResultNotification": //(C_10_Req)User Acceptance Result Notification
                requestInterface.userAcceptanceResultNotification(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "downloadAcceptanceResultNotification": //(C_12_Req)Download Acceptance Result Notification
                requestInterface.downloadAcceptanceResultNotification(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "downloadStartNotification": //(C_13_Req)Download start notification
                requestInterface.downloadStartNotification(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "softwarePackageDL": //(C_14_Req)SoftwarePackage DL
                DownloadManager.getInstance().download(userAgent, vin, date,requestInterface,"localfilePath",futureResult);//TODO:文件本地路径统一管理
                break;
            case "downloadProgress": //(C_15_Req)Download progress
                requestInterface.downloadProgress(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "downloadCompletionNotification": //(C_16_Req)Download completion notification
                requestInterface.downloadCompletionNotification(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "downloadCompletionConfirmationResultNotification": //(C_17_Req)Download completion confirmation result notification
                requestInterface.downloadCompletionConfirmationResultNotification(userAgent, vin, date, (GetCampaignStatus) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "campaignValidityCheckRequest": //(C_18_Req)Campaign validity check request
                requestInterface.campaignValidityCheckRequest(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "installAcceptanceResultNotification": //(C_19_Req)Install Acceptance Result Notification
                requestInterface.installAcceptanceResultNotification(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "installationProgress": //(C_20_Req)Installation progress
                requestInterface.installationProgress(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "installationCompletionConfirmationResultNotification": //(C_21_Req)Installation completion confirmation result notification
                requestInterface.installationCompletionConfirmationResultNotification(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "notificationOfActivationAcceptanceResults": //(C_22_Req)Notification of activation acceptance results
                requestInterface.notificationOfActivationAcceptanceResults(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "activationProgress": //(C_23_Req)Activation Progress
                requestInterface.activationProgress(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "notificationOfActivationCompletionConfirmationResult": //(C_24_Req)Notification of activation completion confirmation result
                requestInterface.notificationOfActivationCompletionConfirmationResult(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "softwareUpdateCompletionConfirmationResultNotification": //(C_25_Req)Software update completion confirmation result notification
                requestInterface.softwareUpdateCompletionConfirmationResultNotification(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "vehicleErrorNotification": //(C_28_Req)Vehicle error notification
                requestInterface.vehicleErrorNotification(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "newerVersionConfirmationDirector": //(C_30_Req)Newer version confirmation (Director Root Metadata)
                requestInterface.newerVersionConfirmationDirector(userAgent, vin, date)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "newerVersionConfirmationImage": //(C_31_Req)Newer version confirmation (Image Root Metadata)
                requestInterface.newerVersionConfirmationImage(userAgent, vin, date)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "imageSnapshotMetadataDownloadRequest": //(C_32_Req)Image Snapshot Metadata Download request
                requestInterface.imageSnapshotMetadataDownloadRequest(userAgent, vin, date)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "imageTimestampMetadataDownloadRequest": //(C_33_Req)Image Timestamp Metadata Download request
                requestInterface.imageTimestampMetadataDownloadRequest(userAgent, vin, date)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "imageTargetsMetadataDownloadRequest": //(C_34_Req)Image Targets Metadata Download request
                requestInterface.imageTargetsMetadataDownloadRequest(userAgent, vin, date)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "softwareUpdateCompletionNotification": //(C_35_Req)Software update completion notification
                requestInterface.softwareUpdateCompletionNotification(userAgent, vin, date, (VsmDeviceEvents) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "otaCommandAcquisition": //(C_36_Req)OTA Command acquisition
                requestInterface.otaCommandAcquisition(userAgent, vin, date, (CommonMetadata) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "vehicleLogUploadUrlAcquisition": //(C_37_Req)Vehicle Log Upload URL Acquisition
                requestInterface.vehicleLogUploadUrlAcquisition(userAgent, vin, date)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
            case "vehicleLogUpload": //(C_38_Req)Vehicle Log Upload
                // 计算文件MD5散列值
                String md5Checksum = "calculateMD5Checksum(file);"; //TODO
                requestInterface.vehicleLogUpload(userAgent, vin, date,  md5Checksum, (RequestBody) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));

                break;
            case "vehicleLogUploadCompletionNotice": //(C_39_Req)Vehicle Log Upload Completion Notice
                requestInterface.vehicleLogUploadCompletionNotice(userAgent, vin, date, (MultiPartCompleteUploadRequestMetadata) requestObject)
                        .subscribeOn(Schedulers.io())
                        .subscribe(handleResponseBody(futureResult));
                break;
        }

    }

    /**
     * Handle response body disposable observer.
     *
     * @param futureResult the future result
     * @return the disposable observer
     */
//TODO:错误码对应703，自定义错误码在body
    public DisposableObserver<ResponseBody> handleResponseBody(CompletableFuture<String> futureResult){

        return new DisposableObserver<ResponseBody>() {
            @Override
            public void onNext(ResponseBody responseBody) {
                System.out.println("onNext begin");
                try {
                    if (ContentEncoding.equals("gzip")) {
                        // 创建 GZIP 输入流
                        GZIPInputStream gzipInputStream = new GZIPInputStream(responseBody.byteStream());
                        // 读取解压后的字符串数据
                        StringBuilder stringBuilder = new StringBuilder();
                        byte[] buffer = new byte[1024];
                        int length;
                        while ((length = gzipInputStream.read(buffer)) != -1) {
                            stringBuilder.append(new String(buffer, 0, length, StandardCharsets.UTF_8));
                        }
                        // 关闭 GZIP 输入流
                        gzipInputStream.close();

                        // 输出解压后的字符串数据
                        String unzippedResponseBody = stringBuilder.toString();

                        Log.d(LOG_ID_RETROFIT_MANAGER,"unzippedResponseBody = "+unzippedResponseBody);
                        futureResult.complete(unzippedResponseBody);

                    } else {
                        // 没有进行压缩的情况下直接输出字符串数据
                        Log.d(LOG_ID_RETROFIT_MANAGER,"unzippedResponseBody = "+responseBody.string());
                        futureResult.complete(responseBody.string());
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }

            }

            @Override
            public void onError(Throwable e) {
                try {

                } catch (Exception e1) {
                    e1.printStackTrace();
                }
                futureResult.complete(e.getMessage());//TODO: 需要统一格式
            }


            @Override
            public void onComplete() {
                // 请求完成
                System.out.println("onComplete ");
            }
        };
    }


    /**
     * The type Api exception.
     */
    public class ApiException extends RuntimeException {
        private String errorCode;
        private String errorBody;

        /**
         * Instantiates a new Api exception.
         *
         * @param detailMessage the detail message
         */
        public ApiException(String detailMessage)
        {
            this(detailMessage,"{}");
        }

        /**
         * Instantiates a new Api exception.
         *
         * @param detailMessage the detail message
         * @param errorBody     the error body
         */
        public ApiException(String detailMessage, String errorBody)
        {
            super(detailMessage);
            this.errorBody = errorBody;
            this.errorCode=detailMessage;
        }

        /**
         * Gets error code.
         *
         * @return the error code
         */
        public String getErrorCode()
        {
            return errorCode;
        }

        /**
         * Gets err body.
         *
         * @return the err body
         */
        public String getErrBody()
        {
            return errorBody;
        }
    }

    /**
     * The type Error code table.
     */
    public static class ErrorCodeTable {
        /**
         * Handle specific error boolean.
         *
         * @param errCode the err code
         * @return the boolean
         */
        public static boolean handleSpecificError(String errCode) {
            switch (errCode) {
                case "1":
                    handleErrorCode1();
                    break;
                case "2":
                    handleErrorCode2();
                    break;
                case "3":
                    handleErrorCode3();
                    break;
                default:
                    return false;
            }
            return true;
        }

        private static void handleErrorCode1() {
            // 处理错误码为1的情况
            // ...
        }

        private static void handleErrorCode2() {
            // 处理错误码为2的情况
            // ...
        }

        private static void handleErrorCode3() {
            // 处理错误码为3的情况
            // ...
        }
    }

    /**
     * On handled net error.
     *
     * @param throwable the throwable
     */
    public void  onHandledNetError(Throwable throwable)
    {
        System.out.println("onHandledNetError》"+ (throwable==null?"null":throwable.getMessage()));
    }

    /**
     * 单向认证
     * https请求时初始化证书
     *
     * @param
     * @return certificates
     */
//TODO: 需要导入证书
    public Pair<SSLSocketFactory, TrustManager[]> setCertificates() {
        // 导入证书文件
        InputStream inputStream;
        SSLSocketFactory sslSocketFactory = null;
        TrustManager[] trustManagers = null;
        try {
            inputStream = new FileInputStream("path/to/certificate.crt");
            // 创建一个 X509 证书对象
            CertificateFactory certificateFactory = CertificateFactory.getInstance("X.509");
            X509Certificate certificate = (X509Certificate) certificateFactory.generateCertificate(inputStream);

            // 创建一个空的 KeyStore 对象，并设置类型为 "pkcs12"
            KeyStore keyStore = KeyStore.getInstance(KeyStore.getDefaultType());
            keyStore.load(null, null);

            // 将证书添加到 KeyStore 中
            keyStore.setCertificateEntry("updateOkHttpClientAlias", certificate);

            // 关闭输入流
            inputStream.close();

            // 初始化 TrustManagerFactory，并使用 KeyStore 进行初始化
            TrustManagerFactory trustManagerFactory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
            trustManagerFactory.init(keyStore);

            // 获取 TrustManager 数组
            trustManagers = trustManagerFactory.getTrustManagers();

            // 创建 SSLContext 对象并进行初始化
            SSLContext sslContext = SSLContext.getInstance("TLS");
            sslContext.init(null, trustManagers, new SecureRandom());

            // 获取 SSLSocketFactory 对象
            sslSocketFactory = sslContext.getSocketFactory();

        } catch (CertificateException | KeyStoreException | IOException | NoSuchAlgorithmException | KeyManagementException e) {
            e.printStackTrace();
        }

        return new Pair<>(sslSocketFactory, trustManagers);
    }

    /**
     * 双向认证
     *
     * @return pair
     */
    public Pair<SSLSocketFactory, TrustManager[]> setTwoWayCertificates(){
        SSLSocketFactory sslSocketFactory = null;
        TrustManager[] trustManagers = null;
        try {
            // 创建 SSLContext，并加载服务器和客户端证书
            SSLContext sslContext = SSLContext.getInstance("TLSv1.2");//参照 190_AppendixA_ECU独自のサイバーセキュリティ要求仕様書_Ver1.0   4.1.3. 標準アルゴリズム

            KeyStore keyStore = KeyStore.getInstance("PKCS12");
            // 加载服务器证书
            InputStream serverCertInputStream = new FileInputStream("path/to/certificate.crt");
            keyStore.load(serverCertInputStream, "server_password".toCharArray());
            TrustManagerFactory trustManagerFactory = TrustManagerFactory.getInstance(
                    TrustManagerFactory.getDefaultAlgorithm());
            trustManagerFactory.init(keyStore);

            // 加载客户端证书和私钥
            InputStream clientCertInputStream = new FileInputStream("path/to/certificate.crt");
            keyStore.load(clientCertInputStream, "client_password".toCharArray());
            KeyManagerFactory keyManagerFactory = KeyManagerFactory.getInstance(
                    KeyManagerFactory.getDefaultAlgorithm());
            keyManagerFactory.init(keyStore, "client_password".toCharArray());

            sslContext.init(keyManagerFactory.getKeyManagers(), trustManagerFactory.getTrustManagers(),
                    new SecureRandom());
            sslSocketFactory = sslContext.getSocketFactory();
            sslSocketFactory.getSupportedCipherSuites();
            trustManagers = trustManagerFactory.getTrustManagers();


        } catch (NoSuchAlgorithmException | UnrecoverableKeyException | CertificateException | KeyStoreException | IOException | KeyManagementException e) {
            e.printStackTrace();
        }

        return new Pair<>(sslSocketFactory, trustManagers);


    }

}