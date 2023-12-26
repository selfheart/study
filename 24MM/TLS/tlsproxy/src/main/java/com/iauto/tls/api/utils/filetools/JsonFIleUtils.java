package com.iauto.tls.api.utils.filetools;

import static com.iauto.tls.api.common.LogDef.LOG_ID_JSONFILE_UTILS;


import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.iauto.tls.api.common.Log;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;

/**
 * The type Json f ile utils.
 */
public class JsonFIleUtils {

    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();

    /**
     * Write object to json file.
     *
     * @param obj      the obj
     * @param filePath the file path
     */
    public void writeObjectToJsonFile(Object obj, String filePath) {
        if(!isValidFilePath(filePath)){
            Log.d(LOG_ID_JSONFILE_UTILS, "filePath :"+filePath+"is invalid");
            return;
        }

        // 验证文件是否存在
        File file = new File(filePath);
        if (!file.exists() || !file.isFile()) {
            Log.d(LOG_ID_JSONFILE_UTILS, "filePath :" + filePath + "is not exists and it will be created");
            try {
                // 创建新文件
                if(!file.createNewFile()){
                    Log.d(LOG_ID_JSONFILE_UTILS,"文件创建失败");
                    return;
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        try (FileWriter writer = new FileWriter(filePath)) {
            String json = gson.toJson(obj);
            writer.write(json);
        } catch ( IOException e) {
            e.printStackTrace();
        }

    }

    /**
     * Read json file to object t.
     *
     * @param <T>      the type parameter
     * @param filePath the file path
     * @param clazz    the clazz
     * @return the t
     */
    public <T> T readJsonFileToObject(String filePath, Class<T> clazz) {

        if(!isValidFilePath(filePath)){
            Log.d(LOG_ID_JSONFILE_UTILS, "filePath :"+filePath+"is invalid");
            return null;
        }
        // 验证文件是否存在
        File file = new File(filePath);
        if (!file.exists() || !file.isFile()) {
           return null;
        }
        try (BufferedInputStream bis = new BufferedInputStream(new FileInputStream(filePath))) {
            byte[] data = new byte[bis.available()];
            bis.read(data);
            String json = new String(data);
            return gson.fromJson(json, clazz);
        } catch (IOException e) {
            e.printStackTrace();
        }

        return null;
    }
    private  boolean isValidFilePath(String filePath) {
        // 验证文件路径是否为空
        if (filePath == null || filePath.trim().isEmpty()) {
            Log.e(LOG_ID_JSONFILE_UTILS, "filePath :"+filePath+"is null");
            return false;
        }
        return true;
    }
}
