package com.iauto.tls.api.common;

import static com.iauto.tls.api.common.LogDef.id2String;

import java.util.Locale;

/**
 * The type Log.
 */
public class Log {
    /**
     * The constant TAG.
     */
    public static final String TAG = "OTA";
    /**
     * The constant INFO.
     */
    public static final int INFO = 0;
    /**
     * The constant DEBUG.
     */
    public static final int DEBUG = 1;
    /**
     * The constant WARN.
     */
    public static final int WARN = 2;
    /**
     * The constant ERROR.
     */
    public static final int ERROR = 3;
    private static final int index = 3;
    private static int level = 0;

    /**
     * Init log level.
     *
     * @param value the value
     */
    public static void InitLogLevel(int value) {
        level = value;
    }

    /**
     * Write log.
     *
     * @param tag     the tag
     * @param content the content
     */
    public static void writeLog(String tag, String content) {
        switch (level) {
            case INFO: {
                info(tag, content);
            }
            break;
            case DEBUG: {
                debug(tag, content);
            }
            break;
            case WARN: {
                warn(tag, content);
            }
            break;
            case ERROR: {
                error(tag, content);
            }
            break;
        }
    }

    private static void info(String tag, String content) {
        android.util.Log.i(tag, getLog(content));
    }

    private static void debug(String tag, String content) {
        android.util.Log.d(tag, getLog(content));
    }

    private static void warn(String tag, String content) {
        android.util.Log.w(tag, getLog(content));
    }

    private static void error(String tag, String content) {
        android.util.Log.e(tag, getLog(content));
    }

    private static String getLog(String content) {
        StringBuilder sb = new StringBuilder();
        StackTraceElement[] stackTraceElements = new Throwable().getStackTrace();
        StackTraceElement st = stackTraceElements[index];
        Thread curThread = Thread.currentThread();
        String fileName = st.getFileName();
        String clsName = fileName.substring(0, fileName.indexOf("."));
        sb.append("[").append(curThread.getName()).append("-").append(curThread.getId()).append("]")
                .append(content).append("(").append(clsName).append(":")
                .append(st.getLineNumber()).append(")");

        return sb.toString();
    }

    private static String generatePrefix(StackTraceElement caller) {
        String prefix = "%s.%s(Line:%d)";
        String className = caller.getClassName();
        className = className.substring(className
                .lastIndexOf(".") + 1);
        prefix = String.format(Locale.getDefault(), prefix, className, caller.getMethodName(),
                caller.getLineNumber());
        return prefix;
    }

    private static StackTraceElement getCallerStackTraceElement() {
        return Thread.currentThread().getStackTrace()[4];
    }

    /**
     * V.
     *
     * @param tag the tag
     * @param id  the id
     * @param msg the msg
     */
    public static void v(String tag, short id, String msg) {
        android.util.Log.v(tag, msg);

    }

    /**
     * V.
     *
     * @param id   the id
     * @param args the args
     */
    public static void v(short id, Object... args) {
        StackTraceElement caller = getCallerStackTraceElement();
        String prefix = generatePrefix(caller);

        StringBuffer sb = new StringBuffer();
        sb.append(prefix);
        for (Object arg : args) {
            sb.append(arg);
            sb.append(" ");
        }
        v(TAG, id, sb.toString());
    }

    /**
     * D.
     *
     * @param tag the tag
     * @param id  the id
     * @param msg the msg
     */
    public static void d(String tag, short id, String msg) {
        android.util.Log.d(tag, id2String(id) + msg);
    }

    /**
     * D.
     *
     * @param id   the id
     * @param args the args
     */
    public static void d(short id, Object... args) {
        StackTraceElement caller = getCallerStackTraceElement();
        String prefix = generatePrefix(caller);

        StringBuffer sb = new StringBuffer();
        sb.append(prefix);
        for (Object arg : args) {
            sb.append(arg);
            sb.append(" ");
        }
        d(TAG, id, sb.toString());
    }

    /**
     * .
     *
     * @param tag the tag
     * @param id  the id
     * @param msg the msg
     */
    public static void i(String tag, short id, String msg) {
        android.util.Log.i(tag, id2String(id) + msg);
    }

    /**
     * .
     *
     * @param id   the id
     * @param args the args
     */
    public static void i(short id, Object... args) {
        StackTraceElement caller = getCallerStackTraceElement();
        String prefix = generatePrefix(caller);

        StringBuffer sb = new StringBuffer();
        sb.append(prefix);
        for (Object arg : args) {
            sb.append(arg);
            sb.append(" ");
        }
        i(TAG, id, sb.toString());
    }

    /**
     * W.
     *
     * @param tag the tag
     * @param id  the id
     * @param msg the msg
     */
    public static void w(String tag, short id, String msg) {
        android.util.Log.w(tag, id2String(id) + msg);
    }

    /**
     * W.
     *
     * @param id   the id
     * @param args the args
     */
    public static void w(short id, Object... args) {
        StackTraceElement caller = getCallerStackTraceElement();
        String prefix = generatePrefix(caller);

        StringBuffer sb = new StringBuffer();
        sb.append(prefix);
        for (Object arg : args) {
            sb.append(arg);
            sb.append(" ");
        }
        w(TAG, id, sb.toString());
    }

    /**
     * E.
     *
     * @param tag the tag
     * @param id  the id
     * @param msg the msg
     */
    public static void e(String tag, short id, String msg) {
        android.util.Log.e(tag, id2String(id) + msg);
    }

    /**
     * E.
     *
     * @param id   the id
     * @param args the args
     */
    public static void e(short id, Object... args) {
        StackTraceElement caller = getCallerStackTraceElement();
        String prefix = generatePrefix(caller);

        StringBuffer sb = new StringBuffer();
        sb.append(prefix);
        for (Object arg : args) {
            sb.append(arg);
            sb.append(" ");
        }
        e(TAG, id, sb.toString());
    }
}
