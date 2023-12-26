package com.iauto.tls.api.v1;

import com.iauto.tls.api.requestbody.CommonMetadata;
import com.iauto.tls.api.requestbody.CompactVehicleManifest;
import com.iauto.tls.api.requestbody.GetActionRequest;
import com.iauto.tls.api.requestbody.GetCampaignStatus;
import com.iauto.tls.api.requestbody.LightVehicleVersionManifest;
import com.iauto.tls.api.requestbody.MultiPartCompleteUploadRequestMetadata;
import com.iauto.tls.api.requestbody.VsmDeviceEvents;

import io.reactivex.rxjava3.core.Observable;
import okhttp3.RequestBody;
import okhttp3.ResponseBody;
import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.Header;
import retrofit2.http.Headers;
import retrofit2.http.POST;
import retrofit2.http.PUT;
import retrofit2.http.Path;
import retrofit2.http.Streaming;

/**
 * The interface Request interface.
 */
public interface RequestInterface {
    /**
     * CAMPAIGN STATUS 通知(C_01_Req)
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @Headers({
            "Content-type: application/json;charset=UTF-8",
            "Accept-Encoding: gzip",
            "Content-Encoding: gzip",
    })
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> campaignStatusNotification(@Header("User-Agent") String userAgent,
                                                        @Header("x-vin") String vin,
                                                        @Header("DATE") String date,
                                                        @Body VsmDeviceEvents request);

    /**
     * Ota action acquisition observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> otaActionAcquisition(@Header("User-Agent") String userAgent,
                                                        @Header("x-vin") String vin,
                                                        @Header("DATE") String date,
                                                        @Body GetActionRequest request);

    /**
     * Vehicle configuration information digest observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> vehicleConfigurationInformationDigest(@Header("User-Agent") String userAgent,
                                                  @Header("x-vin") String vin,
                                                  @Header("DATE") String date,
                                                  @Body CompactVehicleManifest request);

    /**
     * Vehicle configuration information upload observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> vehicleConfigurationInformationUpload(@Header("User-Agent") String userAgent,
                                                                   @Header("x-vin") String vin,
                                                                   @Header("DATE") String date,
                                                                   @Body LightVehicleVersionManifest request);

    /**
     * Hmi data acquisition observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @return the observable
     */
    @GET("/datacollection/api/2.0/events")
    Observable<ResponseBody> hmiDataAcquisition(@Header("User-Agent") String userAgent,
                                                                   @Header("x-vin") String vin,
                                                                   @Header("DATE") String date);

    /**
     * Download tracking notification observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> downloadTrackingNotification(@Header("User-Agent") String userAgent,
                                                                   @Header("x-vin") String vin,
                                                                   @Header("DATE") String date,
                                                                   @Body VsmDeviceEvents request);

    /**
     * User acceptance result notification observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> userAcceptanceResultNotification(@Header("User-Agent") String userAgent,
                                                          @Header("x-vin") String vin,
                                                          @Header("DATE") String date,
                                                          @Body VsmDeviceEvents request);

    /**
     * Download acceptance result notification observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> downloadAcceptanceResultNotification(@Header("User-Agent") String userAgent,
                                                                  @Header("x-vin") String vin,
                                                                  @Header("DATE") String date,
                                                                  @Body VsmDeviceEvents request);

    /**
     * Download start notification observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> downloadStartNotification(@Header("User-Agent") String userAgent,
                                                                  @Header("x-vin") String vin,
                                                                  @Header("DATE") String date,
                                                                  @Body VsmDeviceEvents request);

    /**
     * Software package dl observable.
     *
     * @param userAgent   the user agent
     * @param vin         the vin
     * @param date        the date
     * @param dynamicPath the dynamic path
     * @param start       从某个字节开始下载数据
     * @return the observable
     * @Streaming 请求得到的数据会以流的形式返回 ，即文件下载。
     */
    @Streaming
    @GET("{dynamicPath}")
    Observable<ResponseBody> softwarePackageDL(@Header("User-Agent") String userAgent,
                                               @Header("x-vin") String vin,
                                               @Header("DATE") String date,
                                               @Path("dynamicPath") String dynamicPath,
                                               @Header("RANGE") String start);

    /**
     * Download progress observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
        Observable<ResponseBody> downloadProgress(@Header("User-Agent") String userAgent,
                                               @Header("x-vin") String vin,
                                               @Header("DATE") String date,
                                               @Body VsmDeviceEvents request);

    /**
     * Download completion notification observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> downloadCompletionNotification(@Header("User-Agent") String userAgent,
                                                       @Header("x-vin") String vin,
                                                       @Header("DATE") String date,
                                                       @Body VsmDeviceEvents request);

    /**
     * Download completion confirmation result notification observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> downloadCompletionConfirmationResultNotification(@Header("User-Agent") String userAgent,
                                                            @Header("x-vin") String vin,
                                                            @Header("DATE") String date,
                                                            @Body GetCampaignStatus request);

    /**
     * Campaign validity check request observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> campaignValidityCheckRequest(@Header("User-Agent") String userAgent,
                                                                              @Header("x-vin") String vin,
                                                                              @Header("DATE") String date,
                                                                              @Body VsmDeviceEvents request);

    /**
     * Install acceptance result notification observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> installAcceptanceResultNotification(@Header("User-Agent") String userAgent,
                                                          @Header("x-vin") String vin,
                                                          @Header("DATE") String date,
                                                          @Body VsmDeviceEvents request);

    /**
     * Installation completion confirmation result notification observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> installationCompletionConfirmationResultNotification(@Header("User-Agent") String userAgent,
                                                                 @Header("x-vin") String vin,
                                                                 @Header("DATE") String date,
                                                                 @Body VsmDeviceEvents request);

    /**
     * Installation progress observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> installationProgress(@Header("User-Agent") String userAgent,
                                                                                  @Header("x-vin") String vin,
                                                                                  @Header("DATE") String date,
                                                                                  @Body VsmDeviceEvents request);

    /**
     * Notification of activation acceptance results observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> notificationOfActivationAcceptanceResults(@Header("User-Agent") String userAgent,
                                                                                  @Header("x-vin") String vin,
                                                                                  @Header("DATE") String date,
                                                                                  @Body VsmDeviceEvents request);

    /**
     * Activation progress observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> activationProgress(@Header("User-Agent") String userAgent,
                                                  @Header("x-vin") String vin,
                                                  @Header("DATE") String date,
                                                  @Body VsmDeviceEvents request);

    /**
     * Notification of activation completion confirmation result observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> notificationOfActivationCompletionConfirmationResult(@Header("User-Agent") String userAgent,
                                                @Header("x-vin") String vin,
                                                @Header("DATE") String date,
                                                @Body VsmDeviceEvents request);

    /**
     * Software update completion confirmation result notification observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> softwareUpdateCompletionConfirmationResultNotification(@Header("User-Agent") String userAgent,
                                                @Header("x-vin") String vin,
                                                @Header("DATE") String date,
                                                @Body VsmDeviceEvents request);

    /**
     * Vehicle error notification observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> vehicleErrorNotification(@Header("User-Agent") String userAgent,
                                                @Header("x-vin") String vin,
                                                @Header("DATE") String date,
                                                @Body VsmDeviceEvents request);

    /**
     * Newer version confirmation director observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @return the observable
     */
    @GET("/datacollection/api/2.0/events")
    Observable<ResponseBody> newerVersionConfirmationDirector(@Header("User-Agent") String userAgent,
                                                @Header("x-vin") String vin,
                                                @Header("DATE") String date);

    /**
     * Newer version confirmation image observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @return the observable
     */
    @GET("/datacollection/api/2.0/events")
    Observable<ResponseBody> newerVersionConfirmationImage(@Header("User-Agent") String userAgent,
                                                              @Header("x-vin") String vin,
                                                              @Header("DATE") String date);

    /**
     * Image snapshot metadata download request observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @return the observable
     */
    @GET("/datacollection/api/2.0/events")
    Observable<ResponseBody> imageSnapshotMetadataDownloadRequest(@Header("User-Agent") String userAgent,
                                                              @Header("x-vin") String vin,
                                                              @Header("DATE") String date);

    /**
     * Image timestamp metadata download request observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @return the observable
     */
    @GET("/datacollection/api/2.0/events")
    Observable<ResponseBody> imageTimestampMetadataDownloadRequest(@Header("User-Agent") String userAgent,
                                                              @Header("x-vin") String vin,
                                                              @Header("DATE") String date);

    /**
     * Image targets metadata download request observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @return the observable
     */
    @GET("/datacollection/api/2.0/events")
    Observable<ResponseBody> imageTargetsMetadataDownloadRequest(@Header("User-Agent") String userAgent,
                                                              @Header("x-vin") String vin,
                                                              @Header("DATE") String date);

    /**
     * Software update completion notification observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> softwareUpdateCompletionNotification(@Header("User-Agent") String userAgent,
                                                   @Header("x-vin") String vin,
                                                   @Header("DATE") String date,
                                                   @Body VsmDeviceEvents request);

    /**
     * Ota command acquisition observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> otaCommandAcquisition(@Header("User-Agent") String userAgent,
                                                              @Header("x-vin") String vin,
                                                              @Header("DATE") String date,
                                                              @Body CommonMetadata request);

    /**
     * Vehicle log upload url acquisition observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> vehicleLogUploadUrlAcquisition(@Header("User-Agent") String userAgent,
                                                   @Header("x-vin") String vin,
                                                   @Header("DATE") String date);

    /**
     * Vehicle log upload observable.
     *
     * @param userAgent  the user agent
     * @param vin        the vin
     * @param date       the date
     * @param contentMD5 the content md 5
     * @param file       the file
     * @return the observable
     */
    @PUT("/datacollection/api/2.0/events")
    Observable<ResponseBody> vehicleLogUpload(@Header("User-Agent") String userAgent,
                                              @Header("x-vin") String vin,
                                              @Header("DATE") String date,
                                              @Header("Content-MD5") String contentMD5,
                                              @Body RequestBody file);

    /**
     * Vehicle log upload completion notice observable.
     *
     * @param userAgent the user agent
     * @param vin       the vin
     * @param date      the date
     * @param request   the request
     * @return the observable
     */
    @POST("/datacollection/api/2.0/events")
    Observable<ResponseBody> vehicleLogUploadCompletionNotice(@Header("User-Agent") String userAgent,
                                                   @Header("x-vin") String vin,
                                                   @Header("DATE") String date,
                                                   @Body MultiPartCompleteUploadRequestMetadata request);



}