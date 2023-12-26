package com.iauto.tls.api.DownloadHandle;

import android.widget.TextView;


import com.iauto.tls.api.v1.RequestInterface;

import io.reactivex.rxjava3.disposables.Disposable;


/**
 * The type Download info.
 */
public class DownloadInfo extends BaseBean{
    /* 文件存储名 */
    private String fileName;
    /* 存储位置 */
    private String savePath;
    /* 文件总长度 */
    private long contentLength;
    /* 已下载长度 */
    private long readLength;
    /* 下载该文件的url */
    private String url;
    /* 复用Retrofit对象 */
    private RequestInterface service;
    /* 被观察对象，用于取消下载 */
    private Disposable disposable;
    /* 下载状态 */
    private DownState state;
    /* 下载回调 */
    private DownloadListener listener;


    /**
     * Gets file name.
     *
     * @return the file name
     */
    public String getFileName() {
        return fileName;
    }

    /**
     * Sets file name.
     *
     * @param fileName the file name
     */
    public void setFileName(String fileName) {
        this.fileName = fileName;
        writeToJsonFIle();
    }

    /**
     * Gets service.
     *
     * @return the service
     */
    public RequestInterface getService() {
        return service;
    }

    /**
     * Sets service.
     *
     * @param service the service
     */
    public void setService(RequestInterface service) {
        this.service = service;
        writeToJsonFIle();
    }

    /**
     * Gets url.
     *
     * @return the url
     */
    public String getUrl() {
        return url;
    }

    /**
     * Sets url.
     *
     * @param url the url
     */
    public void setUrl(String url) {
        this.url = url;
        writeToJsonFIle();
    }

    /**
     * Gets save path.
     *
     * @return the save path
     */
    public String getSavePath() {
        return savePath;
    }

    /**
     * Sets save path.
     *
     * @param savePath the save path
     */
    public void setSavePath(String savePath) {
        this.savePath = savePath;
        writeToJsonFIle();
    }

    /**
     * Gets content length.
     *
     * @return the content length
     */
    public long getContentLength() {
        return contentLength;
    }

    /**
     * Sets content length.
     *
     * @param contentLength the content length
     */
    public void setContentLength(long contentLength) {
        this.contentLength = contentLength;
        writeToJsonFIle();
    }

    /**
     * Gets read length.
     *
     * @return the read length
     */
    public long getReadLength() {
        return readLength;
    }

    /**
     * Sets read length.
     *
     * @param readLength the read length
     */
    public void setReadLength(long readLength) {
        this.readLength = readLength;
        writeToJsonFIle();//便于断电续传，断电再启动时，从断点处恢复下载
    }

    /**
     * Gets disposable.
     *
     * @return the disposable
     */
    public Disposable getDisposable() {
        return disposable;
    }

    /**
     * Sets disposable.
     *
     * @param disposable the disposable
     */
    public void setDisposable(Disposable disposable) {
        this.disposable = disposable;
        writeToJsonFIle();
    }

    /**
     * Gets state.
     *
     * @return the state
     */
    public DownState getState() {
        return state;
    }

    /**
     * Sets state.
     *
     * @param state the state
     */
    public void setState(DownState state) {
        this.state = state;
        writeToJsonFIle();
    }

    /**
     * Gets listener.
     *
     * @return the listener
     */
    public DownloadListener getListener() {
        return listener;
    }

    /**
     * Sets listener.
     *
     * @param listener the listener
     */
    public void setListener(DownloadListener listener) {
        this.listener = listener;
        writeToJsonFIle();
    }

    @Override
    public String toString() {
        return "DownloadInfo{" +
                "fileName='" + fileName + '\'' +
                ", savePath='" + savePath + '\'' +
                ", contentLength=" + contentLength +
                ", readLength=" + readLength +
                ", url='" + url + '\'' +
                ", service=" + service +
                ", disposable=" + disposable +
                ", state=" + state +
                ", listener=" + listener +
                '}';
    }


    /**
     * Write to json f ile.
     */
    public void writeToJsonFIle(){
        super.writeJson();
    }
}
