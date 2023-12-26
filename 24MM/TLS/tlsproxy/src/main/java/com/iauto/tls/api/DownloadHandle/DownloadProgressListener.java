package com.iauto.tls.api.DownloadHandle;

import java.io.File;

import io.reactivex.Observable;
import io.reactivex.disposables.Disposable;
import io.reactivex.functions.Consumer;


/**
 * The type Download progress listener.
 */
public class DownloadProgressListener {

    private DownloadInfo info;
    private DownloadListener listener;

    /**
     * Instantiates a new Download progress listener.
     *
     * @param downloadInfo the download info
     * @param listener     the listener
     */
    public DownloadProgressListener(DownloadInfo downloadInfo, DownloadListener listener) {
        this.info = downloadInfo;
        this.listener = listener;
    }

    /**
     * Progress.
     *
     * @param read          已下载长度
     * @param contentLength 总长度
     * @param done          是否下载完毕
     */
    public void progress(long read, long contentLength, final boolean done){
        //更新已下载的文件大小
        if (info.getContentLength() > contentLength) {
            read = read + (info.getContentLength() - contentLength);
        } else {
            info.setContentLength(contentLength);
        }
        info.setReadLength(read);
        //切换到主线程
        Disposable d = Observable.just(1)
                .subscribe(new Consumer<Integer>() {
                    @Override
                    public void accept(Integer integer) {
                        if (done) {
                            listener.onFinishDownload(info.getSavePath() + File.separator + info.getFileName(), info.getState());
                            info.setState(DownState.FINISH);
                        } else {
                            if(info.getState() == DownState.DOWNLOADING) {
                                if (info.getContentLength() > 0) {
                                    listener.onProgress(info.getReadLength(), info.getContentLength(), info.getState());
                                } else {
                                    // 获取不到文件总大小(contentLength==0)
                                    listener.onProgress(info.getReadLength(), -1, info.getState());
                                }
                            }
                        }
                    }
                });
    }

}