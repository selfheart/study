package com.iauto.tls.api.net;

import static com.iauto.tls.api.common.LogDef.LOG_ID_TIMEOUT_INTERCEPTOR;

import com.iauto.tls.api.common.Log;

import okhttp3.Interceptor;
import okhttp3.Request;
import okhttp3.Response;

import java.io.IOException;
import java.net.SocketTimeoutException;

/**
 * The type Retry interceptor.
 */
public class RetryInterceptor implements Interceptor {
    private int maxRetryCount = 3; //参考Server Push Spec.pdf  図3：サーバ異常
    private final long waitTime = 1000 * 60 ;//参考Server Push Spec.pdf  図3：サーバ異常

    @Override
    public Response intercept(Chain chain) throws IOException {
        Request request = chain.request();
        Response response = null;
        int retryCount = 0;

        // 进行重试操作，直到达到最大重试次数
        while (retryCount < maxRetryCount) {
            Log.d(LOG_ID_TIMEOUT_INTERCEPTOR,"Failed to connect to server, retry "+ retryCount +" times");
            try {
                response = chain.proceed(request);
                if(HttpsStatusCode.retryCode(response.code())){
                    if (retryCount < maxRetryCount - 1) {
                        retryCount++;
                        // 等待一段时间再进行重试
                        waitForRetry();
                    } else {
                        // 达到最大重试次数，进行其他错误处理
                        handleMaxRetryReached();
                    }
                }else{
                    break; // 请求成功，跳出重试循环
                }
            } catch (SocketTimeoutException e) {
                // 发生超时错误
                if (retryCount < maxRetryCount - 1) {
                    retryCount++;
                    // 等待一段时间再进行重试
                    waitForRetry();
                } else {
                    // 达到最大重试次数，进行其他错误处理
                    handleMaxRetryReached();
                }
            }
        }

        return response;
    }



    private void waitForRetry() {
        try {
            Thread.sleep(waitTime);
        } catch (InterruptedException e) {
            // 处理中断异常，根据需求进行操作
            e.printStackTrace();
        }
    }

    private void handleMaxRetryReached() {
        // 达到最大重试次数，进行其他错误处理
        Log.e(LOG_ID_TIMEOUT_INTERCEPTOR,"Failed to connect to server, maximum retry count has been reached!");
    }
}

