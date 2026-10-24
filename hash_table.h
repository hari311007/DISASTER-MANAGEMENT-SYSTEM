#ifndef HASH_TABLE_H
#define HASH_TABLE_H

/* ---------- INVENTORY HASH TABLE (Separate Chaining) ----------
 * Maps SKU ID -> item details for one relief depot.
 * Each depot is linked to a node in the road graph (depot_node_id).
 */

#define TABLE_SIZE 31          /* prime number -> fewer collisions */
#define SKU_LEN    16
#define NAME_LEN   32
#define DATE_LEN   11          /* "YYYY-MM-DD" */

typedef struct Item {
    char sku[SKU_LEN];         /* unique key, e.g. "MED-001"   */
    char name[NAME_LEN];       /* e.g. "Paracetamol"           */
    int  quantity;
    char expiry[DATE_LEN];     /* e.g. "2027-03-15"            */
    struct Item *next;         /* chain for collisions         */
} Item;

typedef struct {
    Item *buckets[TABLE_SIZE];
    int   count;               /* number of distinct SKUs      */
    int   depot_node_id;       /* graph node where depot sits  */
} HashTable;

void  ht_init(HashTable *ht, int depot_node_id);
unsigned int ht_hash(const char *sku);
int   ht_insert(HashTable *ht, const char *sku, const char *name,
                int qty, const char *expiry);      /* add / restock */
Item *ht_search(HashTable *ht, const char *sku);
int   ht_update_qty(HashTable *ht, const char *sku, int change); /* +restock / -dispatch */
int   ht_delete(HashTable *ht, const char *sku);
void  ht_display(HashTable *ht);
void  ht_low_stock(HashTable *ht, int threshold);
void  ht_free(HashTable *ht);

#endif
