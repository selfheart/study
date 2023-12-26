package com.iauto.tls.api.bean;

import com.google.gson.annotations.SerializedName;

/**
 * The type Time ms.
 */
public class TimeMs {
    /**
     * 1970/1/1 からの経過ミリ
     * 秒数
     */
    @SerializedName("Time")
    private Number time;
    /**
     * 時刻のソース。
     * "default"(車両時計時刻)
     */
    @SerializedName("clocksource")
    private String clockSource = "default";

    /**
     * Gets time.
     *
     * @return the time
     */
    public Number getTime() {
        return time;
    }

    /**
     * Sets time.
     *
     * @param time the time
     */
    public void setTime(Number time) {
        this.time = time;
    }

    /**
     * Gets clock source.
     *
     * @return the clock source
     */
    public String getClockSource() {
        return clockSource;
    }

    /**
     * Sets clock source.
     *
     * @param clockSource the clock source
     */
    public void setClockSource(String clockSource) {
        this.clockSource = clockSource;
    }
}
