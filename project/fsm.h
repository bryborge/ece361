#ifndef FSM_H
#define FSM_H

/* CLOSED is the initial state. */
typedef enum {
    CLOSED,
    OPENING,
    OPEN,
    CLOSING,
    OBSTRUCTED
} state_t;

#endif
