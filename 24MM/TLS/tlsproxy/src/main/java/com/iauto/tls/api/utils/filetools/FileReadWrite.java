package com.iauto.tls.api.utils.filetools;

import static com.iauto.tls.api.common.LogDef.LOG_ID_FILEREADWRITE;


import com.iauto.tls.api.common.Log;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;

/**
 * The type File read write.
 *
 * @program: UCM
 * @description: Provide multiple file read and write methods
 */
public class FileReadWrite {

    /**
     * Instantiates a new File read write.
     */
    public FileReadWrite() {

    }

    /**
     * Read file on size byte [ ].
     *
     * @param filePath   the file path
     * @param bufferSize the buffer size
     * @return byte [ ]
     * @description: Read a file of a certain size
     */
    public  byte[] readFileOnSize(String filePath,int bufferSize){

        byte[] buffer = new byte[bufferSize];
        try (BufferedInputStream bis = new BufferedInputStream(new FileInputStream(filePath))) {
            bis.read(buffer, 0, bufferSize);
        } catch (IOException e) {
            Log.e(LOG_ID_FILEREADWRITE,"readFileOnSize error : "+e);
        }
        return buffer;
    }

    /**
     * Read file stop on flag byte [ ].
     *
     * @param filePath the file path
     * @param flag     the flag
     * @return byte [ ]
     * @throws UnsupportedEncodingException the unsupported encoding exception
     * @description: Reading file encountered flag or ending and stopped
     */
    public byte[] readFileStopOnFlag(String filePath, int flag) throws UnsupportedEncodingException {
        ArrayList<Byte> bytesList = new ArrayList<>();
         try (BufferedInputStream bis = new BufferedInputStream(new FileInputStream(filePath))) {
            while (true) {
                int b = bis.read();
                if (b == -1 || b == flag) {
                    break;
                }
                bytesList.add((byte) b);
            }
        } catch (IOException e) {
            Log.e(LOG_ID_FILEREADWRITE,"readFileStopOnFlag error : "+e);
        }

        byte[] result = new byte[bytesList.size()];


        for (int i = 0; i < bytesList.size(); i++) {
            result[i] = bytesList.get(i);
        }
        return result;
    }

    /**
     * Read file to bytes byte [ ].
     *
     * @param filePath the file path
     * @return byte [ ]
     * @throws IOException
     * @description: Read file and return byte array
     */
    public  byte[] readFileToBytes(String filePath){
        File file = new File(filePath);
        int len = 0;
        byte[] bytes = new byte[0];
        int offset = 0;
        try (FileInputStream fis = new FileInputStream(file)) {
            len = (int) file.length();
            bytes = new byte[len];
            offset = 0;
            int read;
            while (offset < len && (read = fis.read(bytes, offset, len - offset)) != -1) {
                offset += read;
            }
        } catch (IOException e) {
            Log.e(LOG_ID_FILEREADWRITE,"readFileToBytes error : "+e);
        }
        if (offset != len) {
            Log.e(LOG_ID_FILEREADWRITE,"readFileToBytes error : "+"Could not completely read file");

        }
        return bytes;
    }

    /**
     * Read file to bytes byte [ ].
     *
     * @param filePath the file path
     * @param offset   the offset
     * @param size     the size
     * @return the byte [ ]
     */
    public byte[] readFileToBytes(String filePath, long offset, long size) {
        byte[] data = new byte[(int) size];
        try (RandomAccessFile file = new RandomAccessFile(filePath, "r")) {
            // 设置文件指针位置为偏移量
            file.seek(offset);

            file.read(data, 0, (int)size);


        } catch (IOException e) {
            e.printStackTrace();
            Log.e(LOG_ID_FILEREADWRITE,"readFileToBytes error : "+e);
        }
        return data;
    }

}
