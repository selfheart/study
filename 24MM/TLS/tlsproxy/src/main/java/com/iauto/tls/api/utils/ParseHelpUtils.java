/**
 * Copyright @ 2023 - 2025 iAUTO(Shanghai) Co., Ltd.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are NOT permitted except as agreed by
 * iAUTO(Shanghai) Co., Ltd.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

package com.iauto.tls.api.utils;

import static com.iauto.tls.api.common.LogDef.LOG_ID_PARSE_HELP_UTILS;

import com.iauto.tls.api.common.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Base64;
import java.util.zip.GZIPOutputStream;

/**
 * The type Parse help utils.
 */
public class ParseHelpUtils {


    /**
     * G gzip and base 64 encode string.
     *
     * @param str the str
     * @return the string
     */
    public String gGzipAndBase64Encode(String str) {
        Log.d(LOG_ID_PARSE_HELP_UTILS,"gGzipAndBase64Encode ");
        return Base64.getEncoder().encodeToString(Gzip(str));
    }

    /**
     * Gzip byte [ ].
     *
     * @param logdata the logdata
     * @return the byte [ ]
     */
    public byte[] Gzip(String logdata) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        GZIPOutputStream gzip;
        try {
            gzip = new GZIPOutputStream(out);
            gzip.write(logdata.getBytes());
            gzip.close();
        } catch (IOException var4) {
            Log.e(LOG_ID_PARSE_HELP_UTILS,var4.getMessage());
        }
        return out.toByteArray();
    }
}
