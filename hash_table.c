#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

void ht_init(HashTable *ht, int depot_node_id) {
    for (int i = 0; i < TABLE_SIZE; i++)
        ht->buckets[i] = NULL;
    ht->count = 0;
    ht->depot_node_id = depot_node_id;
}

/* djb2 string hash -> bucket index */
unsigned int ht_hash(const char *sku) {
    unsigned long h = 5381;
    int c;
    while ((c = *sku++))
        h = h * 33 + c;
    return (unsigned int)(h % TABLE_SIZE);
}

/* Insert new SKU, or restock if it already exists. Returns 1 on success. */
int ht_insert(HashTable *ht, const char *sku, const char *name,
              int qty, const char *expiry) {
    if (qty < 0) return 0;

    Item *existing = ht_search(ht, sku);
    if (existing) {                         /* restock */
        existing->quantity += qty;
        strncpy(existing->expiry, expiry, DATE_LEN - 1);
        return 1;
    }

    Item *node = (Item *)malloc(sizeof(Item));
    if (!node) return 0;
    strncpy(node->sku, sku, SKU_LEN - 1);     node->sku[SKU_LEN - 1] = '\0';
    strncpy(node->name, name, NAME_LEN - 1);  node->name[NAME_LEN - 1] = '\0';
    strncpy(node->expiry, expiry, DATE_LEN - 1); node->expiry[DATE_LEN - 1] = '\0';
    node->quantity = qty;

    unsigned int idx = ht_hash(sku);
    node->next = ht->buckets[idx];          /* insert at chain head: O(1) */
    ht->buckets[idx] = node;
    ht->count++;
    return 1;
}

Item *ht_search(HashTable *ht, const char *sku) {
    Item *cur = ht->buckets[ht_hash(sku)];
    while (cur) {
        if (strcmp(cur->sku, sku) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

/* change > 0 : restock, change < 0 : dispatch to convoy.
 * Returns 1 ok, 0 not found, -1 insufficient stock. */
int ht_update_qty(HashTable *ht, const char *sku, int change) {
    Item *it = ht_search(ht, sku);
    if (!it) return 0;
    if (it->quantity + change < 0) return -1;
    it->quantity += change;
    return 1;
}

int ht_delete(HashTable *ht, const char *sku) {
    unsigned int idx = ht_hash(sku);
    Item *cur = ht->buckets[idx], *prev = NULL;
    while (cur) {
        if (strcmp(cur->sku, sku) == 0) {
            if (prev) prev->next = cur->next;
            else      ht->buckets[idx] = cur->next;
            free(cur);
            ht->count--;
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

void ht_display(HashTable *ht) {
    printf("\n=== Depot @ Node %d | %d SKUs ===\n", ht->depot_node_id, ht->count);
    printf("%-6s %-10s %-15s %-6s %-11s\n", "Bucket", "SKU", "Name", "Qty", "Expiry");
    for (int i = 0; i < TABLE_SIZE; i++)
        for (Item *c = ht->buckets[i]; c; c = c->next)
            printf("%-6d %-10s %-15s %-6d %-11s\n",
                   i, c->sku, c->name, c->quantity, c->expiry);
}

void ht_low_stock(HashTable *ht, int threshold) {
    printf("\n--- Low stock alert (< %d) ---\n", threshold);
    for (int i = 0; i < TABLE_SIZE; i++)
        for (Item *c = ht->buckets[i]; c; c = c->next)
            if (c->quantity < threshold)
                printf("  %s (%s): %d left\n", c->sku, c->name, c->quantity);
}

void ht_free(HashTable *ht) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Item *c = ht->buckets[i];
        while (c) { Item *n = c->next; free(c); c = n; }
        ht->buckets[i] = NULL;
    }
    ht->count = 0;
}
