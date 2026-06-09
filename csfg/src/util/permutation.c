#include "csfg/config.h"
#include "csfg/util/permutation.h"
#include <assert.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------- */
static void swap(int* p, int a, int b)
{
    int tmp = p[a];
    p[a]    = p[b];
    p[b]    = tmp;
}

/* -------------------------------------------------------------------------- */
static void reverse_range(int* p, int start, int end)
{
    while (start < --end)
        swap(p, start++, end);
}

/* -------------------------------------------------------------------------- */
static void reverse(int* p, int count)
{
    reverse_range(p, 0, count);
}

/* -------------------------------------------------------------------------- */
static int compare(const void* a, const void* b)
{
    return *(const int*)a - *(const int*)b;
}
void permutation_sort(int* p, int count)
{
    if (p == NULL || count == 0)
        return;
    qsort(p, count, sizeof(int), compare);
}

/* -------------------------------------------------------------------------- */
int permutation_next(int* p, int count)
{
    int i, pivot;

    if (count == 0)
        return 0;

    for (pivot = 1; pivot < count; ++pivot)
        if (p[pivot - 1] < p[pivot])
            break;
    if (pivot == count)
    {
        reverse(p, count);
        return 0;
    }

    for (i = 0; i != count; ++i)
        if (p[pivot] > p[i])
        {
            swap(p, i, pivot);
            break;
        }
    CSFG_DEBUG_ASSERT(i != count);

    reverse_range(p, 0, pivot);
    return 1;
}
