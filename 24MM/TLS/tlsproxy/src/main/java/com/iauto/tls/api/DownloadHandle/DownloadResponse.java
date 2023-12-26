package com.iauto.tls.api.DownloadHandle;

import java.io.IOException;

import io.reactivex.rxjava3.annotations.NonNull;
import io.reactivex.rxjava3.annotations.Nullable;
import okhttp3.MediaType;
import okhttp3.ResponseBody;
import okio.Buffer;
import okio.BufferedSource;
import okio.ForwardingSource;
import okio.Okio;
import okio.Source;


/**
 * The type Download response.
 */
public class DownloadResponse extends ResponseBody {

    private ResponseBody responseBody;

    private DownloadProgressListener listener;

    // BufferedSource 是okio库中的输入流，这里就当作inputStream来使用。
    private BufferedSource bufferedSource;

    /**
     * Instantiates a new Download response.
     *
     * @param responseBody the response body
     * @param listener     the listener
     */
    public DownloadResponse(ResponseBody responseBody, DownloadProgressListener listener) {
        this.responseBody = responseBody;
        this.listener = listener;
    }

    @Nullable
    @Override
    public MediaType contentType() {
        return responseBody.contentType();
    }

    @Override
    public long contentLength() {
        return responseBody.contentLength();
    }

    @NonNull
    @Override
    public BufferedSource source() {
        if (bufferedSource == null) {
            bufferedSource = Okio.buffer(source(responseBody.source()));
        }
        return bufferedSource;
    }

    private Source source(Source source) {
        return new ForwardingSource(source) {
            long totalBytesRead = 0L;

            @Override
            public long read(@NonNull Buffer sink, long byteCount) throws IOException {
                final long bytesRead = super.read(sink, byteCount);
                // read() returns the number of bytes read, or -1 if this source is exhausted.
                totalBytesRead += bytesRead != -1 ? bytesRead : 0;
                if (null != listener) {
                    listener.progress(totalBytesRead, responseBody.contentLength(), bytesRead == -1);
                }
                return bytesRead;
            }
        };

    }
}
