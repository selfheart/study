package com.iauto.tls.api.net;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

/**
 * 参考 Server Push Spec.pdf 表4：レスポンスコードとリトライ要否 / Talbe4 : Response code and retry
 * 封装HttpsStatusCode
 */
public enum HttpsStatusCode {
    /**
     * Success https status code.
     */
    SUCCESS(200, "OK"),
    /**
     * The Bad request.
     */
    BAD_REQUEST(400, "Bad Request"),
    /**
     * Unauthorized https status code.
     */
    UNAUTHORIZED(401, "Unauthorized"),
    /**
     * Forbidden https status code.
     */
    FORBIDDEN(403, "Forbidden"),
    /**
     * The Not found.
     */
    NOT_FOUND(404, "Not Found"),
    /**
     * Conflict https status code.
     */
    CONFLICT(409, "Conflict"),
    /**
     * Too many requests https status code.
     */
    TOO_MANY_REQUESTS(429, "Forbidden"),
    /**
     * The Client closed request.
     */
    CLIENT_CLOSED_REQUEST(499, "Client Closed Request"),
    /**
     * The Internal server error.
     */
    INTERNAL_SERVER_ERROR(500, "Internal Server Error"),
    /**
     * The Not implemented.
     */
    NOT_IMPLEMENTED(501, "Not Implemented"),
    /**
     * The Service unavailable.
     */
    SERVICE_UNAVAILABLE(503, "Service Unavailable"),
    /**
     * The Gateway timeout.
     */
    GATEWAY_TIMEOUT(504, "Gateway Timeout");


    private final int code;
    private final String message;

    HttpsStatusCode(int code, String message) {
        this.code = code;
        this.message = message;
    }

    /**
     * Gets code.
     *
     * @return the code
     */
    public int getCode() {
        return code;
    }

    /**
     * Gets message.
     *
     * @return the message
     */
    public String getMessage() {
        return message;
    }

    /**
     * Retry code boolean.
     *
     * @param retryCode the retry code
     * @return the boolean
     */
    public static boolean retryCode(int retryCode) {
        Integer[] retryCodes = {500, 501, 503, 504};
        HashSet<Integer> set = new HashSet<>(Arrays.asList(retryCodes));
        return set.contains(retryCode);
    }

    /**
     * Find by code https status code.
     *
     * @param code the code
     * @return the https status code
     */
    public static HttpsStatusCode findByCode(int code) {
        for (HttpsStatusCode statusCode : values()) {
            if (statusCode.getCode() == code) {
                return statusCode;
            }
        }
        return null;
    }
}

