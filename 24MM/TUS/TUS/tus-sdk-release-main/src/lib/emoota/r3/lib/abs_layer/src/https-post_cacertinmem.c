/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <spil_file/spil_file.h>

#ifdef __cplusplus
}
#endif

#include "abst.h"
#include "abst_config.h"



static struct CURL *prepare_curl(cstr_t rootca) {
    CURL *curl;
    long verifypeer = 0;
    long verifyhost = 0;

    curl = curl_easy_init();
    if (!curl)
        return curl;

    if (rootca != NULL) {
        verifypeer = 1; // verify peer's certificate
        verifyhost = 2; // verify server name against CN/SAN
    }

    // expected to be a non-fatal error
    if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifypeer)) {
        printf("%s: failed to set CURLOPT_SSL_VERIFYPEER=%ld\n", __func__, verifypeer);
    }
    if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifyhost)) {
        printf("%s: failed to set CURLOPT_SSL_VERIFYHOST=%ld\n", __func__, verifyhost);
    }
    // note: OCSP stapling (CURLOPT_SSL_VERIFYSTATUS) shall be (and to be kept as) 0

    if (verifypeer != 0) {
        struct curl_blob blob;
        blob.data = rootca;
        blob.len = strlen(rootca);
        blob.flags = CURL_BLOB_COPY;
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob)) {
            printf("%s: failed to set CURLOPT_CAINFO_BLOB\n", __func__);
        }
    }

    return curl;
}

struct memory {
    char *response;
    size_t recv_sz;
    size_t rsp_buf_sz;
};


size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    memcpy(ptr, userdata, strlen((char*)userdata) + 1);

    return strlen((char*)userdata);
}

/**
 * used for HTTP header output to stderr. Use for debugging only
 */
static size_t writefunction(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t rval = fwrite(ptr, size, nmemb, (FILE *) stream);
    return (nmemb * size);
}

/**
 * used for HTTP response data output to user provided buffer
 */
static size_t data_cb(void *data, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct memory *mem = (struct memory *) userp;
    size_t rsp_buf_sz = mem->rsp_buf_sz;

    if((realsize + mem->recv_sz) >= rsp_buf_sz) {
        realsize = rsp_buf_sz;    // limit output to rsp_buf_sz
        // limit will cause transfer abort and CURLE_WRITE_ERROR
    }
    if (mem->response != NULL) {
        memcpy(&(mem->response[mem->recv_sz]), data, realsize);
        mem->recv_sz += realsize;
        mem->response[mem->recv_sz] = 0;
    }

    return realsize;
}

static bool curl_is_init = false;

abst_err_t abst_SLC_transaction(sdp_api_t sdpApi, char *request_buffer, size_t request_sz,
                                char *response_buffer, size_t *response_sz, char *uuid,
                                uint32_t *http_status_code, cstr_t sdp_url, cstr_t sdp_port, cstr_t rootca) {

    abst_err_t rval = ABST_NO_ERROR;
    CURL *curl;
    CURLcode res;
    char api_buffer[512] = {};
    struct memory chunk = {
            .response = response_buffer,
            .recv_sz = 0,
            .rsp_buf_sz = (response_sz == NULL) ? 0 : *response_sz
    };
    printf("Send Request\n");

    if ((sdpApi == sdp_api_notification) && (uuid == NULL)) {
        return ABST_ERR_TRANSACTION_FAILED;
    }

    if (!curl_is_init) {
        //printf("[ RUN      ] abq_req_and_res_001\n");
        curl_global_init(CURL_GLOBAL_ALL);
        curl_is_init = true;
    }else{
        printf("INFO:Network is already initialized.\n");
    }
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    /* get a curl handle */
    //printf("[ RUN      ] abq_req_and_res_002\n");
    curl = prepare_curl(rootca);
    //printf("[ RUN      ] abq_req_and_res_003\n");
    if (curl) {

        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        //printf("[ RUN      ] abq_req_and_res_004\n");
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, request_buffer);
        //printf("[ RUN      ] abq_req_and_res_005\n");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
#ifdef DEBUG_BUILD
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writefunction);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, stderr);
#else
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
#endif

        curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
        /* First set the URL that is about to receive our POST. This URL can
         just as well be a https:// URL if that is what should receive the
         data. */

        // Concatenate sections for HTTP request body
        //printf("[ RUN      ] abq_req_and_res_006\n");
        int base_len = 0;
        switch (sdpApi) {
            case sdp_api_configMatch:
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(request_buffer));
                snprintf(api_buffer, sizeof(api_buffer)-1, "%s:%s%s", sdp_url, sdp_port, SLC_CFG_MATCH);
                curl_easy_setopt(curl, CURLOPT_URL, api_buffer);
                break;
            case sdp_api_syncCheck:
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(request_buffer));
                snprintf(api_buffer, sizeof(api_buffer)-1, "%s:%s%s", sdp_url, sdp_port, SLC_SYNC_CHECK);
                curl_easy_setopt(curl, CURLOPT_URL, api_buffer);
                break;
            case sdp_api_getOtaCmd:
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(request_buffer));
                snprintf(api_buffer, sizeof(api_buffer)-1, "%s:%s%s", sdp_url, sdp_port, SLC_GET_OTA_CMD);
                curl_easy_setopt(curl, CURLOPT_URL, api_buffer);
                break;
            case sdp_api_notification:
                curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
                //                curl_easy_setopt(curl, CURLOPT_PUT, 1L);
                curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) request_sz);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_cb);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
                snprintf(api_buffer, sizeof(api_buffer)-1, "%s:%s%s%s", sdp_url, sdp_port, SLC_PUT_NFY, uuid);
                curl_easy_setopt(curl, CURLOPT_URL, api_buffer);
                break;
            case sdp_api_multiInit:
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(request_buffer));
                snprintf(api_buffer, sizeof(api_buffer)-1, "%s:%s%s", sdp_url, sdp_port, SLC_MPU_INIT);
                curl_easy_setopt(curl, CURLOPT_URL, api_buffer);
                break;
            case adp_api_multiComp:
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(request_buffer));
                snprintf(api_buffer, sizeof(api_buffer)-1, "%s:%s%s", sdp_url, sdp_port, SLC_MPU_COMP);
                curl_easy_setopt(curl, CURLOPT_URL, api_buffer);
                break;
            default:
                return ABST_ERR_TRANSACTION_FAILED;
                break;
        }

        /* Perform the request, res will get the return code */
        //printf("[ RUN      ] abq_req_and_res_007\n");
        res = curl_easy_perform(curl);

        /* Check for errors */
        if (res == CURLE_OK) {
            *response_sz = chunk.recv_sz;
            printf("*** transfer succeeded ***\n");
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if(http_status_code != NULL) {
                *http_status_code = response_code;
            }
            //fprintf(stdout, "RESPONSE:\n%s\n", response_buffer);
            // Process response data if needed.
            //printf("[ RUN      ] abq_req_and_res_008\n");
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            rval = ABST_ERR_TRANSACTION_FAILED;
        }
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return rval;
}


abst_err_t abst_GETfromCDN(const char *url, FILE *file, cstr_t const dir,
                            cstr_t const filename,uint32_t *http_status_code,
                            cstr_t rootca) {

    abst_err_t rval = ABST_NO_ERROR;
    CURL *curl;
    CURLcode res;

    FILE* out_file = file;

    char tmp[256] = {0};

    if(out_file == NULL){

        if(NULL != dir){
            if(0 == sf_exists(dir)) {
                (void)sf_mkdir(dir);
            }
            strcat(tmp, dir);
        }

        if(filename != NULL){
            strcat(&tmp[strlen(tmp)], filename);
        } else{
            sf_filename_of(url, &tmp[strlen(tmp)], sizeof(tmp));
        }

        out_file = fopen (tmp , "w");
        if (out_file == NULL){
            return ABST_ERR_NO_RESOURCE;
        }
    }

    if (!curl_is_init) {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_is_init = true;
    }
    /* get a curl handle */
    curl = prepare_curl(rootca);
    if (curl) {

#ifdef DEBUG_BUILD
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#else
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
#endif
        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunction);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) out_file);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");

        curl_easy_setopt(curl, CURLOPT_URL, url);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res == CURLE_OK) {
            printf("*** transfer succeeded ***\n");
            if (file != out_file) { fclose(out_file); }// don't close passed in file handle. caller to close file handle
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (http_status_code != NULL) {
                *http_status_code = response_code;
            }
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            rval = ABST_ERR_TRANSACTION_FAILED;
        }
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return rval;
}


abst_err_t abst_PUTonCDN(char *url, cstr_t md5, uint8_t *request_buffer, size_t buffer_sz,
                        char *response_buffer, size_t response_sz,
                        uint32_t *http_status_code, cstr_t rootca) {

    abst_err_t rval = ABST_NO_ERROR;
    CURL *curl;
    CURLcode res;

    struct memory chunk = {
            .response = response_buffer,
            .recv_sz = 0,
            .rsp_buf_sz = response_sz
    };
    byte_t header1[128] = {0};
    byte_t header2[128] = {0};
    cstr_t md5header = "Content-MD5: ";

    if (!curl_is_init) {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_is_init = true;
    }
    struct curl_slist *headers = NULL;
    snprintf(header1, sizeof(header1), "Content-Length: %i", (int)buffer_sz);
    headers = curl_slist_append(headers, header1);
    snprintf(header2, strlen(md5header)+1+MAX_BASE64_MD5_SZ, "%s%s",md5header, md5);
    headers = curl_slist_append(headers, header2);

    /* get a curl handle */
    curl = prepare_curl(rootca);
    if (curl) {

        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        //        printf("\n\nUPLOAD DEBUG: %s\n\n", request_buffer);
        curl_easy_setopt(curl, CURLOPT_READDATA, request_buffer);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE, buffer_sz);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, data_cb);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&chunk);

#ifdef DEBUG_BUILD
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#else
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
#endif

        curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);

        /* Turn off the default CA locations, otherwise libcurl will load CA
         * certificates from the locations that were detected/specified at
         * build-time
         */
        curl_easy_setopt(curl, CURLOPT_CAINFO, CONFIG_BASE "/rootca.pem");
        curl_easy_setopt(curl, CURLOPT_CAPATH, CONFIG_BASE);

        /* Perform the request, res will get the return code */
        //printf("[ RUN      ] abq_upl_file_req_009\n");
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res == CURLE_OK) {
            printf("*** transfer succeeded ***\n");
            if (http_status_code != NULL) {
                long response_code;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                *http_status_code = response_code;
            }
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            rval = ABST_ERR_TRANSACTION_FAILED;
        }
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return rval;
}


abst_err_t
abst_GETUptMetafromCDN(char *url, char *out, size_t *response_sz, uint32_t *http_status_code, cstr_t rootca) {

    abst_err_t rval = ABST_NO_ERROR;
    CURL *curl;
    CURLcode res;
    struct memory chunk = {
            .response = out,
            .recv_sz = 0,
            .rsp_buf_sz = *response_sz
    };

    if (!curl_is_init) {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_is_init = true;
    }
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    /* get a curl handle */
    curl = prepare_curl(rootca);
    if (curl) {

        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        /* default type with postfields is application/x-www-form-urlencoded,
           change it if you want */

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
#ifdef DEBUG_BUILD
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writefunction);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, stderr);
#else
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
#endif

        curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        /* First set the URL that is about to receive our POST. This URL can
         just as well be a https:// URL if that is what should receive the
         data. */

        //      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=daniel&project=curl");

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res == CURLE_OK) {
            *response_sz = chunk.recv_sz;
            printf("*** transfer succeeded ***\n");
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (http_status_code != NULL) {
                *http_status_code = response_code;
            }
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            if (*response_sz < chunk.recv_sz) {
                printf("*response_sz(%zu) < chunk.recv_sz(%zu)\n",
                        *response_sz, chunk.recv_sz);
            }
            rval = ABST_ERR_TRANSACTION_FAILED;
        }
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return rval;


}
