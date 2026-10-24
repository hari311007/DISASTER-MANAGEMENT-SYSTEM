#include <stdio.h>
#include <string.h>
#include "heap.h"

static Emergency heap[MAX_EMERGENCIES];
static int size = 0;

/* true if a is more urgent than b: severity, then people affected, then earlier id */
static int higher(const Emergency *a, const Emergency *b)
{
    if (a->severity != b->severity)         return a->severity > b->severity;
    if (a->peopleAffected != b->peopleAffected) return a->peopleAffected > b->peopleAffected;
    return a->id < b->id;
}

static void swap(int i, int j)
{
    Emergency tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
}

static void heapify_up(int i)
{
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (higher(&heap[i], &heap[parent])) {
            swap(i, parent);
            i = parent;
        } else break;
    }
}

static void heapify_down(int i)
{
    for (;;) {
        int left = 2 * i + 1, right = 2 * i + 2, largest = i;
        if (left  < size && higher(&heap[left],  &heap[largest])) largest = left;
        if (right < size && higher(&heap[right], &heap[largest])) largest = right;
        if (largest == i) break;
        swap(i, largest);
        i = largest;
    }
}

static int find_index(int id)
{
    for (int i = 0; i < size; i++)
        if (heap[i].id == id) return i;
    return -1;
}

int heap_is_empty(void) { return size == 0; }
int heap_is_full(void)  { return size == MAX_EMERGENCIES; }
int heap_size(void)     { return size; }

int heap_insert(Emergency e)
{
    if (heap_is_full() || e.severity < 1 || e.severity > 10) return 0;
    if (find_index(e.id) != -1) return 0;           /* duplicate id */
    heap[size] = e;
    size++;
    heapify_up(size - 1);
    return 1;
}

int heap_peek(Emergency *out)
{
    if (heap_is_empty()) return 0;
    *out = heap[0];
    return 1;
}

int heap_extract_max(Emergency *out)
{
    if (heap_is_empty()) return 0;
    *out = heap[0];
    heap[0] = heap[size - 1];
    size--;
    heapify_down(0);
    return 1;
}

int heap_update_severity(int id, int newSeverity)
{
    int i = find_index(id);
    if (i < 0 || newSeverity < 1 || newSeverity > 10) return 0;
    heap[i].severity = newSeverity;
    heapify_up(i);
    heapify_down(i);
    return 1;
}

int heap_delete(int id)
{
    int i = find_index(id);
    if (i < 0) return 0;
    heap[i] = heap[size - 1];
    size--;
    if (i < size) {
        heapify_up(i);
        heapify_down(i);
    }
    return 1;
}

void heap_display(void)
{
    if (heap_is_empty()) { printf("No pending emergencies.\n"); return; }

    Emergency backup[MAX_EMERGENCIES];
    int backupSize = size;
    memcpy(backup, heap, sizeof(Emergency) * size);

    printf("%-4s %-9s %-16s %-10s %s\n", "ID", "Severity", "Location", "Type", "People");
    printf("----------------------------------------------------\n");
    Emergency e;
    while (heap_extract_max(&e))
        printf("%-4d %-9d %-16s %-10s %d\n", e.id, e.severity, e.location, e.type, e.peopleAffected);

    memcpy(heap, backup, sizeof(Emergency) * backupSize);   /* restore heap */
    size = backupSize;
}
