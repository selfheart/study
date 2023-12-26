package com.iauto.tls.api.DownloadHandle;

import io.reactivex.Observable;
import okhttp3.ResponseBody;
import retrofit2.http.GET;
import retrofit2.http.Header;
import retrofit2.http.Streaming;
import retrofit2.http.Url;


/**
 * The interface Download service.
 */
public interface DownloadService {
    /**
     * Rx download observable.
     *
     * @param start 从某个字节开始下载数据
     * @param url   the url
     * @return the observable
     */
    @Streaming
    @GET
    Observable<ResponseBody> rxDownload(@Header("RANGE") String start, @Url String url);

    /**
     * Rx download observable.
     *
     * @param url the url
     * @return the observable
     */
    @Streaming
    @GET
    Observable<ResponseBody> rxDownload(@Url String url);

}
