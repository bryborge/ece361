# SPEC

## Events and transition table

TBD (week 5)

## State machine drawing

TBD (week 5)

## Decisions where a rule was silent

TBD (week 5)

## record_t, quantization, and duplicate keys

```c
typedef struct {
    time_t timestamp;   /* UTC seconds since the Unix epoch */
    float temp_c;       /* rounded to 0.1 °C */
    float rh_pct;       /* rounded to 0.1 %RH */
} reading_t;

typedef struct {
    reading_t reading;
    state_t state;      /* the state at the moment of the reading */
    uint32_t status;    /* the status word at that moment */
} record_t;
```

**Quantization.** The key of a record is `floor(temp_c * 2)`, the index of its 0.5 °C step. `floor`, not `round`: 21.2 and 21.4 °C must share a key, and `round` would put them at 42 and 43. A step includes its lower edge, so 21.5 starts the step 21.5 to 21.9. `floor` also keeps negative temperatures in the right step (-0.2 °C is below 0.0). Multiplying a float by 2 is exact, so the edges x.0 and x.5 land exactly. `record_compare` compares keys only and does not tie-break.

**Duplicate keys: a list per key.** Each key holds every record with that key, in insertion order, which is time order. In the week-4 sorted array, that means a new record goes after the last record with the same key. In the week-8 tree, each node holds the list for its key. A count per key was rejected because it discards the timestamp, state, and status of all but one reading, and the report needs them. A tie-break on the timestamp was rejected because records arrive in time order, so all the records of one key would form a chain down the right side, and a garage stays in one 0.5 °C step for hundreds of ticks. With a list per key, the height depends only on the number of distinct keys.

**Height.** `store_height` returns the number of distinct keys while the store is an array. That equals the height of a tree with one node per key built from inserts in ascending key order, so the height tests hold before and after the tree.

**Capacity.** Until week 8 the store is one static array of 1024 records (`STORE_CAP`, private to `store.c`), and `store_insert` returns -1 when it is full. The tree removes the cap.

**NULL.** A NULL store, record, or output array is a bug in the caller and aborts with an `assert`, except `store_destroy(NULL)`, which does nothing (like `free(NULL)`).

## Stack growth

TBD (week 7)

## Report definition

TBD (week 9)

## Status word bit layout

TBD (week 3)

## RGB color of each state

TBD (week 6)

## 2038

TBD

## The given states: defend or criticize

TBD (week 10)

## Stack compared with an alternative

TBD (week 7)
