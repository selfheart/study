package com.iauto.tls.api.DownloadHandle;


/**
 * The enum Down state.
 */
public enum  DownState {
    /**
     * Default down state.
     */
    DEFAULT(0),
    /**
     * Downloading down state.
     */
    DOWNLOADING(1),
    /**
     * Pause down state.
     */
    PAUSE(2),
    /**
     * Error down state.
     */
    ERROR(3),
    /**
     * Finish down state.
     */
    FINISH(4);

    private int state;

    /**
     * Gets state.
     *
     * @return the state
     */
    public int getState() {
        return state;
    }

    /**
     * Sets state.
     *
     * @param state the state
     */
    public void setState(int state) {
        this.state = state;
    }

    DownState(int state) {
        this.state = state;
    }
}
