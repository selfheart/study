package com.iauto.tls.api.DownloadHandle;

import android.widget.TextView;


/**
 * The interface Download listener.
 */
public interface DownloadListener {

    /**
     * On start download.
     *
     * @param downState the down state
     */
    void onStartDownload(DownState downState);

    /**
     * On progress.
     *
     * @param downloaded the downloaded
     * @param total      the total
     * @param downState  the down state
     */
    void onProgress(long downloaded, long total, DownState downState);

    /**
     * On pause download.
     *
     * @param downState the down state
     */
    void onPauseDownload(DownState downState);

    /**
     * On cancel download.
     *
     * @param downState the down state
     */
    void onCancelDownload(DownState downState);

    /**
     * On finish download.
     *
     * @param savedFile the saved file
     * @param downState the down state
     */
    void onFinishDownload(String savedFile, DownState downState);

    /**
     * On fail.
     *
     * @param errorInfo the error info
     * @param downState the down state
     */
    void onFail(String errorInfo, DownState downState);
}
