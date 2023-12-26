package com.iauto.tls.api.utils.filetools.fileStore;

import static com.iauto.tls.api.common.LogDef.LOG_ID_FILE_BEAN;

import com.iauto.tls.api.DownloadHandle.BaseBean;
import com.iauto.tls.api.common.Log;

import java.io.File;
import java.util.HashMap;

/**
 * The type File bean.
 *
 * @program:
 * @description: Read the path of all files from here
 */
public class FileBean extends BaseBean {
    /**
     * The File map.
     */
    protected HashMap<String,String> fileMap = new HashMap<>();
    private transient volatile static FileBean instance ;
    /**
     * The constant FileBean_JSON.
     */
    public transient static final String FileBean_JSON = "CheckAssetsFiles.ASSET_FILE_PATH"+"/FileBean.json";


    private FileBean() {
        this.filePath = FileBean_JSON;
    }

    /**
     * Get instance file bean.
     *
     * @return the file bean
     */
    public static FileBean getInstance(){
        if(instance == null){
            synchronized (FileBean.class){
                if(instance == null){
                    Log.d(LOG_ID_FILE_BEAN, "FileBean == null");
                    instance=new FileBean();
                    instance = checkFileBeanFile();
                }
            }
        }
        instance.initFile();
        Log.d(LOG_ID_FILE_BEAN, "getInstance: bRet = " + instance);
        return instance;
    }


    /**
     * Gets file map.
     *
     * @return the file map
     */
    public HashMap<String, String> getFileMap() {
        return fileMap;
    }

    /**
     * Sets file map.
     *
     * @param fileMap the file map
     */
    public void setFileMap(HashMap<String, String> fileMap) {
        this.fileMap = fileMap;
    }

    /**
     * Get file path string.
     *
     * @param fileKey all files key ,such as: DynHMI,MP,DP...
     * @return String filePath
     * @see FileKeyValue
     */
    public String getFilePath(String fileKey){
        String filePath = null;
        if(!fileMap.isEmpty()){
            if(fileMap.containsKey(fileKey)){
                filePath = fileMap.get(fileKey);
            }else {
                Log.d(LOG_ID_FILE_BEAN,"fileMap  does not contain this fileKey : "+fileKey);
            }
        }else{
            Log.d(LOG_ID_FILE_BEAN,"fileMap is null!!");
        }
        return filePath;
    }

    /**
     * Check file bean file file bean.
     *
     * @return the file bean
     */
    public static FileBean checkFileBeanFile() {
        Log.d(LOG_ID_FILE_BEAN, "checkFileBeanFile: begin");
        try {
            File file = new File(FileBean_JSON);
            if (!file.exists()) {

                Log.e(LOG_ID_FILE_BEAN, "updateFromFile: file is not exist ,creating new FileBean file");
                if(file.createNewFile()){
                    Log.d(LOG_ID_FILE_BEAN,"create new file : "+file.getName()+" success!");
                }else {
                    Log.d(LOG_ID_FILE_BEAN, "create new file : " + file.getName() + " failed!");
                }
                return new FileBean();
            }

            String jsonString = instance.loadFile();

            if (null == jsonString) {
                Log.e(LOG_ID_FILE_BEAN, "updateFromFile: FileBean.json file is empty");
                return new FileBean();
            } else {
                Log.d(LOG_ID_FILE_BEAN, "updateFromFile: load FileBean.json success");
                return BaseBean.toClass(jsonString, FileBean.class);
            }
        } catch (Exception e) {
            Log.e(LOG_ID_FILE_BEAN, "updateFromFile: fail, e = ", e);
            return new FileBean();
        }

    }

    private  void initFile(){
        //Initialize filemap,When the file is null or the file path is missing
        for (FileKeyValue fileKeyValue : FileKeyValue.values()) {
            if(!fileMap.containsKey(fileKeyValue.getFile_Name())){
                fileMap.put(fileKeyValue.getFile_Name(),fileKeyValue.getFile_Path());
            }
        }
        writeJson();
    }

}
