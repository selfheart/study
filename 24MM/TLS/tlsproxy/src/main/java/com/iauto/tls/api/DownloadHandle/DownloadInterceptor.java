package com.iauto.tls.api.DownloadHandle;

import java.io.IOException;

import io.reactivex.rxjava3.annotations.NonNull;
import okhttp3.Interceptor;
import okhttp3.Response;


/**
 * The type Download interceptor.
 */
public class DownloadInterceptor  implements Interceptor {

    private DownloadProgressListener listener;

    /**
     * Instantiates a new Download interceptor.
     *
     * @param listener the listener
     */
    public DownloadInterceptor(DownloadProgressListener listener) {
        this.listener = listener;
    }

    @NonNull
    @Override
    public Response intercept(@NonNull Chain chain) throws IOException {
        Response response = chain.proceed(chain.request());
        return response.newBuilder()
                .body(new DownloadResponse(response.body(), listener)).build();
    }
}
