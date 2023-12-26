package com.iauto.tls.api.utils.filetools.fileStore;

/**
 * The enum File key value.
 *
 * @program: otaservice
 * @description: Define keywords for some file paths
 */
public enum FileKeyValue {
    /**
     * Add new files this
     * * @param File_Name: file key
     * * @param File_Path: get from the filebean.json file . If it cannot be retrieved, the default value is used
     * <p>
     * All file paths are stored in FileBean.json.
     * During initialization, we do not modify FileBean.json,Unless the file does not contain the following files.
     * So the file path always keeps the updated path value
     */


//update bigdata aes key
    AESKEY_VENDOR_TA01("aeskeyVendorTA01","/aeskeyVendorTA01"),
    /**
     * Aeskey update ta 01 file key value.
     */
    AESKEY_UPDATE_TA01("aeskeyUpdateTA01","/aeskeyUpdateTA01"); //base on userDirFiles ,such as:/data/user_de/0/com.iauto.updatecomposer/files/ +   /aeskeyUpdateTA01


    private String File_Name;
    private String File_Path;


    /**
     *
     * @param File_Name
     * @param File_Path
     */
    FileKeyValue(String File_Name, String File_Path) {
        this.File_Name = File_Name;
        this.File_Path = File_Path;
    }

    /**
     * Gets file name.
     *
     * @return the file name
     */
    public String getFile_Name() {
        return File_Name;
    }

    /**
     * Sets file name.
     *
     * @param file_Name the file name
     */
    public void setFile_Name(String file_Name) {
        File_Name = file_Name;
    }

    /**
     * Gets file path.
     *
     * @return the file path
     */
    public String getFile_Path() {
        return File_Path;
    }

    /**
     * Sets file path.
     *
     * @param file_Path the file path
     */
    public void setFile_Path(String file_Path) {
        File_Path = file_Path;
    }
}


