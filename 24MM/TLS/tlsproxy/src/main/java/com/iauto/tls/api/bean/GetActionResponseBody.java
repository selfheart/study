package com.iauto.tls.api.bean;

/**
 * The type Get action response body.
 */
public class GetActionResponseBody {
    private Action[] actions;

    /**
     * Get actions action [ ].
     *
     * @return the action [ ]
     */
    public Action[] getActions() {
        return actions;
    }

    /**
     * Sets actions.
     *
     * @param actions the actions
     */
    public void setActions(Action[] actions) {
        this.actions = actions;
    }
}
