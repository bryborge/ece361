## The Test that Found the Crash

```c
static void check_contains_empty(void) {
    bst_t *t = bst_create();
    size_t visits = 99;

    CHECK(!bst_contains(t, 1, &visits));
    CHECK(visits == 0);
    bst_destroy(t);
}
```

```sh
bst.c:139:19: runtime error: member access within null pointer of type 'const struct node'
```
