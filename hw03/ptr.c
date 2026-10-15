/*
 * A pointer library.
 */
#include "ptr.h"

// /**
//  * Returns a pointer to the first value greater than threshold, or NULL if there
//  * is none.
//  *
//  * @param  *a The pointer to return.
//  * @param  n
//  * @param  th The threshold.
//  * @return A pointer to the first floating point value greater than threshold.
//  */
// const float *find_above(const float *a, int n, float th) {
//     // TODO: implement
// }

// /**
//  * Stores the smallest and largest of a[0..n-1] in *min and *max and returns 0;
//  * If n < 1 or any pointer is NULL, it changes nothing and returns -1.
//  *
//  * @param *a
//  * @param n
//  * @param *min
//  * @param *max
//  * @return An integer value signifying success or failure of the operation.
//  */
// int minmax(const float *a, int n, float *min, float *max) {
//     // TODO: implement
// }

// /**
//  * Reverses a[0..n-1] in place. It does nothing if a is NULL or n < 2.
//  *
//  * @param *a
//  * @param n
//  */
// void reverse(float *a, int n) {
//     // TODO: implement
// }

/**
 * Swaps *a and *b.
 *
 * @param *a
 * @param *b
 */
void swap_float(float *a, float *b) {
    float x = *a;
    *a = *b;
    *b = x;
}
