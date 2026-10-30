# Algorithms

## Classify a character as an event (`lock_event`)

```
CONSTANT NEXT_DIGIT, one entry per state:
    LOCKED -> '3'
    GOT1   -> '1'
    GOT2   -> '4'
    OPEN   -> no digit (a value that no digit character can equal)

FUNCTION lock_event(s, c)
    ASSERT s is a valid state
    IF c is 'c' THEN
        RETURN E_CLOSE
    END IF
    IF c is 't' THEN
        RETURN E_TIMEOUT
    END IF
    IF c is not a digit '0' through '9' THEN
        RETURN -1
    END IF
    IF c equals NEXT_DIGIT[s] THEN
        RETURN E_RIGHT
    ELSE
        RETURN E_WRONG
    END IF
END FUNCTION
```

Only lowercase 'c' and 't' are events, as the spec says. 'C' and 'T'
return -1.

## Drive the lock from standard input (`main`)

```
SET s TO LOCKED
WHILE another character c can be read from standard input DO
    IF c is whitespace THEN
        skip to the next character
    END IF
    SET e TO lock_event(s, c)
    IF e is -1 THEN
        WRITE "lock: ignoring 'c'" to standard error
        skip to the next character
    END IF
    SET next TO lock_step(s, e, giving action)
    PRINT state_name(s), c, event_name(e), "->", state_name(next), action_name(action)
    SET s TO next
END WHILE
PRINT "final:", state_name(s)
RETURN 0, even if characters were ignored (stderr already said so)
```

## Run a key sequence in a test

```
FUNCTION run(keys, giving last_action)
    SET s TO LOCKED
    SET last_action TO A_NONE
    FOR each character c in keys DO
        SET e TO lock_event(s, c)
        IF e is -1 THEN
            RECORD a FAIL naming c (the test string is mistyped)
            RETURN s
        END IF
        SET s TO lock_step(s, e, giving last_action)
    END FOR
    RETURN s
END FUNCTION

CHECK run("314")  is OPEN,   last action UNLOCK
CHECK run("3314") is LOCKED, last action BEEP
CHECK run("31t4") is LOCKED, last action BEEP
CHECK run("314c") is LOCKED, last action LOCK
```
