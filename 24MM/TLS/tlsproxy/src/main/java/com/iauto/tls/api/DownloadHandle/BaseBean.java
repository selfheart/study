package com.iauto.tls.api.DownloadHandle;



import static com.iauto.tls.api.common.LogDef.LOG_ID_BASE_BEAN;

import android.content.res.AssetManager;

import androidx.annotation.Nullable;

import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;
import com.iauto.tls.api.common.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.util.Objects;

/**
 * 该类为json文件映射基类，功能包括序列化和反序列化，读文件和写文件。
 */
public class BaseBean {
    /**
     * The File path.
     */
    protected transient String filePath;

    /**
     * To json string.
     *
     * @return the string
     */
    public String toJson() {
        try {
            return new Gson().toJson(this);
        } catch (JsonSyntaxException e) {
            e.printStackTrace();
            Log.e(LOG_ID_BASE_BEAN, "to Json error");
        }

        return "";
    }

    /**
     * Convert jsonStr to Bean Class.
     * <p>
     * For example, UpdateConfig updateConfig = BaseBean.toClass(jsonStr, UpdateConfig.class)
     *
     * @param <T>     the type parameter
     * @param jsonStr in general use {@link BaseBean#toJson()} to generate
     * @param clazz   ClassName.class to confirm parsing format
     * @return <T> the type of the object.
     */
    @Nullable
    public static <T extends BaseBean> T toClass(String jsonStr, Class<T> clazz) {
        try {
            T bean = new Gson().fromJson(jsonStr, clazz);
            Log.i(LOG_ID_BASE_BEAN, "parse json file success");
            return bean;
        } catch (JsonSyntaxException e) {
            Log.e(LOG_ID_BASE_BEAN, "parse json file error");
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Write json.
     */
    public void writeJson(){
        String jsonString = toJson();
        RandomAccessFile raf = null;
        try {
            raf = new RandomAccessFile(filePath, "rws");
            raf.setLength(0);
            raf.seek(0);
            raf.write(jsonString.getBytes());
        } catch (Exception e) {
            e.printStackTrace();
            Log.e(LOG_ID_BASE_BEAN);
        } finally {
            try {
                Objects.requireNonNull(raf).close();
            } catch (IOException e) {
                e.printStackTrace();
                Log.e(LOG_ID_BASE_BEAN);
            }
        }
    }

    /**
     * Write json.
     *
     * @param filePath the file path
     */
    public void writeJson(String filePath){
        this.filePath = filePath;
        writeJson();
    }

    /**
     * Load file string.
     *
     * @return the string
     */
    public String loadFile() {
        RandomAccessFile raf = null;
        String retString = null;
        try {
            raf = new RandomAccessFile(filePath, "r");
            raf.seek(0);
            byte [] text_buf = new byte[(int) raf.length()];
            raf.readFully(text_buf);
            retString = new String(text_buf);
        } catch (Exception e) {
            e.printStackTrace();
            Log.e(LOG_ID_BASE_BEAN);
        } finally {
            try {
                Objects.requireNonNull(raf).close();
            } catch (Exception e) {
                e.printStackTrace();
                Log.e(LOG_ID_BASE_BEAN);
            }
        }
        return retString;
    }

    /**
     * Load file string.
     *
     * @param filePath the file path
     * @return the string
     */
    public String loadFile(String filePath) {
        this.filePath = filePath;
        return loadFile();
    }

}
