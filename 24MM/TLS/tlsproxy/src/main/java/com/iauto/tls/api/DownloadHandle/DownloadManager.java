package com.iauto.tls.api.DownloadHandle;


import static com.iauto.tls.api.common.LogDef.LOG_ID_DOWNLOAD_MANAGER;

import android.os.Handler;
import android.text.TextUtils;


import com.iauto.tls.api.common.Log;
import com.iauto.tls.api.utils.filetools.FileUtils;
import com.iauto.tls.api.utils.filetools.JsonFIleUtils;
import com.iauto.tls.api.v1.RequestInterface;

import java.io.File;
import java.io.InputStream;
import java.util.concurrent.CompletableFuture;


import io.reactivex.rxjava3.core.Observer;
import io.reactivex.rxjava3.disposables.Disposable;
import io.reactivex.rxjava3.functions.Consumer;
import io.reactivex.rxjava3.functions.Function;
import io.reactivex.rxjava3.schedulers.Schedulers;
import okhttp3.ResponseBody;


/**
 * The type Download manager.
 */
public class DownloadManager {
    private DownloadInfo currentInfo;




    private volatile static DownloadManager INSTANCE;

    /**
     * Gets instance.
     *
     * @return the instance
     */
    public static DownloadManager getInstance() {
        if (INSTANCE == null) {
            synchronized (DownloadManager.class) {
                if (INSTANCE == null) {
                    INSTANCE = new DownloadManager();
                }
            }
        }

        return INSTANCE;
    }

    private DownloadManager() {


    }


    /**
     * Download.
     *
     * @param userAgent        the user agent
     * @param vin              the vin
     * @param date             the date
     * @param requestInterface the request interface
     * @param localFilePath    the local file path
     * @param futureResult     the future result
     */
    public void download(String userAgent, String vin, String date, RequestInterface requestInterface, String localFilePath, CompletableFuture<String> futureResult) {

        DownloadListener downloadListener = new DownloadListener() {
            @Override
            public void onStartDownload(DownState downState) {

            }

            @Override
            public void onProgress(long downloaded, long total, DownState downState) {

            }

            @Override
            public void onPauseDownload(DownState downState) {

            }

            @Override
            public void onCancelDownload(DownState downState) {

            }

            @Override
            public void onFinishDownload(String savedFile, DownState downState) {

            }

            @Override
            public void onFail(String errorInfo, DownState downState) {

            }
        };

        DownloadPackageHandle downloadPackageHandle = DownloadPackageHandle.getInstance();
        try{
             String url = downloadPackageHandle.getPackageDLURL();
             String filePath = downloadPackageHandle.getLocalFilePAth();
             String fileName = downloadPackageHandle.getPackageFileName();
             startDownload(userAgent, vin, date, url, filePath, fileName, downloadListener, requestInterface);
        }catch (Exception e){
            e.printStackTrace();
        }

    }

    /**
     * 下载文件
     * @param url 文件下载地址
     * @param filePath 文件保存路径（不以“/”结尾）
     * @param fileName 文件保存名称
     * @param listener 下载监听
     */
    private void startDownload(String userAgent, String vin, String date, String url, final String filePath, final String fileName, final DownloadListener listener,RequestInterface requestInterface) {
        currentInfo = new JsonFIleUtils().readJsonFileToObject("ff",DownloadInfo.class);//TODO:filepath统一处理
        long start;
        boolean newTask;
        if (currentInfo != null) {
            if (currentInfo.getState() == DownState.DOWNLOADING) {
                Log.d(LOG_ID_DOWNLOAD_MANAGER,"download is running , wait it");
                //正在下载则不处理
                return;
            } else if (currentInfo.getState() == DownState.PAUSE){
                //从暂停处继续下载
                start = currentInfo.getReadLength();
//                requestInterface = currentInfo.getService();
                newTask = false;
            } else {
                //出错，或者下载已完成
                start = 0;
//                requestInterface = currentInfo.getService();
                newTask = true;
            }
        } else {
            //新的下载任务
            currentInfo = new DownloadInfo();
            currentInfo.setUrl(url);
            currentInfo.setSavePath(filePath);
            currentInfo.setFileName(fileName);
            currentInfo.setListener(listener);

//            currentInfo.setService(requestInterface);
            start = 0;
            newTask = true;
        }
        final DownloadInfo downloadInfo = currentInfo;
        final boolean newFile = newTask;
        requestInterface
                .softwarePackageDL(userAgent, vin, date, url,"bytes=" + start + "-")
                .subscribeOn(Schedulers.io())
                .observeOn(Schedulers.io())
                .map(new Function<ResponseBody, InputStream>() {
                    @Override
                    public InputStream apply(ResponseBody responseBody) throws Exception {
                        return responseBody.byteStream();
                    }
                })
                .doOnNext(new Consumer<InputStream>() {
                    @Override
                    public void accept(InputStream inputStream) throws Exception {
                        FileUtils.writeFileFromIS(filePath, fileName, inputStream, newFile);
                    }
                }).subscribe(new Observer<InputStream>() {
                    @Override
                    public void onSubscribe(Disposable d) {
                        if (listener != null) {
                            listener.onStartDownload(downloadInfo.getState());
                        }
                        downloadInfo.setDisposable(d);
                    }

                    @Override
                    public void onNext(InputStream stream) {
                    }

                    @Override
                    public void onError(Throwable e) {
                        if (listener != null) {
                            listener.onFail(e.getMessage(), downloadInfo.getState());
                            downloadInfo.setState(DownState.ERROR);
                        }
                    }

                    @Override
                    public void onComplete() {
                        Log.d(LOG_ID_DOWNLOAD_MANAGER,"url = "+url+"  download success!");
                    }
                });
        downloadInfo.setState(DownState.DOWNLOADING);

    }

    /**
     * 暂停下载
     *
     * @param url 文件下载地址
     */
    public void pauseDownload(String url) {
        if ("".equals(url)) {
            return;
        }

        final DownloadInfo downloadInfo = currentInfo;
        Disposable disposable = null;
        if (downloadInfo != null) {
            disposable = downloadInfo.getDisposable();
        }
        if (disposable != null && !disposable.isDisposed()) {
            disposable.dispose();
            downloadInfo.setState(DownState.PAUSE);

            //延迟回调，因为disposable.dispose()调用后任务会继续执行若干时间
            final DownloadListener listener = downloadInfo.getListener();
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    listener.onPauseDownload(downloadInfo.getState());
                }
            }, 1000);
        }
    }


    /**
     * 继续下载
     *
     * @param url 文件下载地址
     */
    public void continueDownload(String url) {
        if (TextUtils.isEmpty(url)) {
            return;
        }

        DownloadInfo downloadInfo = currentInfo;
        if (downloadInfo != null && downloadInfo.getState() == DownState.PAUSE) {
//                startDownload(url, downloadInfo.getSavePath(), downloadInfo.getFileName(), downloadInfo.getListener());
        }

    }

    /**
     * 继续下载
     *
     * @param url      文件下载地址
     * @param listener the listener
     */
    public void continueDownload(final String url, final DownloadListener listener) {
        if ("".equals(url)) {
            return;
        }

        DownloadInfo downloadInfo = currentInfo;
        if (downloadInfo != null && downloadInfo.getState() == DownState.PAUSE) {
 //               startDownload(url, downloadInfo.getSavePath(), downloadInfo.getFileName(), listener);
        }

    }

    /**
     * 取消下载
     *
     * @param url        文件下载地址
     * @param deleteFile 是否删除已下载的文件
     */
    public void cancelDownload(String url, boolean deleteFile) {
        if (TextUtils.isEmpty(url)) {
            return;
        }

        final DownloadInfo downloadInfo = currentInfo;
        Disposable disposable = null;
        if (downloadInfo != null) {
            disposable = downloadInfo.getDisposable();
        }
        if (disposable != null && !disposable.isDisposed()) {
            disposable.dispose();

            //延迟回调，因为disposable.dispose()调用后任务会继续执行若干时间
            final DownloadListener listener = downloadInfo.getListener();
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    listener.onCancelDownload(downloadInfo.getState());
                }
            }, 1000);
        }

        if (deleteFile && downloadInfo != null) {
            FileUtils.deleteFile(downloadInfo.getSavePath() + File.separator + downloadInfo.getFileName());
        }
    }

    /**
     * 获取下载任务的状态
     *
     * @param url the url
     * @return task state
     */
    public DownState getTaskState(String url) {
        if ("".equals(url)) {
            return DownState.DEFAULT;
        }
        DownloadInfo downloadInfo = currentInfo;
        if (downloadInfo != null) {
            return downloadInfo.getState();
        }
        return DownState.DEFAULT;
    }

//    /**
//     * 通知当前的下载进度
//     * @return
//     */
//
//    public VsmDeviceEvents reportDownloadProgress(){
//        VsmDeviceEvents vsmDeviceEvents = new VsmDeviceEvents();
//
//
//    }


}