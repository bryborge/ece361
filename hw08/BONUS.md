# Bonus: Removing Keys

`bst_remove(t, key)` takes one key out of the tree. If the key appears more
than once, it removes the copy closest to the root. It returns 0 when it
removes something. If the tree is NULL or the key isn't there, it returns -1
and changes nothing.

How a node is removed depends on how many children it has:

- **No children.** Delete the node. Its parent now has an empty spot where the
  node was.
- **One child.** Delete the node and move its child up into its spot.
- **Two children.** Don't delete this node. Instead, find the smallest key in
  its right subtree: go right once, then keep going left until you can't.
  Copy that key into this node, then delete the node you copied it from. That
  node never has a left child, so deleting it is always one of the first two
  cases.

To run the tests, from `hw08/`:

```sh
make bonus
```

## The original's seven keys

Inserting 27 14 35 10 19 31 42 builds this tree:

```
        27
      /    \
    14      35
   /  \    /  \
  10  19  31  42
```

### Remove 10 (no children)

10 has no children, so it's deleted. 14 no longer has a left child.

```
        27
      /    \
    14      35
      \    /  \
      19  31  42
```

### Remove 14 (one child)

14 has one child, 19. 14 is deleted and 19 moves up into its spot.

```
        27
      /    \
    19      35
           /  \
          31  42
```

### Remove 27 (two children)

27 has two children, so it stays and gets a new key. The smallest key on its
right is 31: go right to 35, then left to 31. Copy 31 into the root, then
delete the old 31 node. It has no children, so that's the first case.

```
        31
      /    \
    19      35
              \
              42
```

The test `check_bonus_sequence` in `tests/test_remove.c` does these same three
removals. After each one, it checks the tree's size and how many steps a
search takes to reach each key. For example, at the end 31 takes 1 step,
since it's the root, and 42 takes 3 steps: 31, 35, 42.
