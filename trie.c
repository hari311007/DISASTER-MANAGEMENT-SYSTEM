#include <stdio.h>   //used for quick name search
#include <stdlib.h>
#include <string.h>
#include "trie.h"

static TrieNode *root  = NULL;
static int       total = 0;

/* ---------- internal helpers ---------- */

/* map a character to 0..26, or -1 if not allowed */
static int char_index(char c)
{
    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c == ' ')             return 26;
    return -1;
}

static TrieNode *new_node(void)  //allocation of memory and creation of newnode
{
    return (TrieNode *)calloc(1, sizeof(TrieNode));
}

static int valid_name(const char *s)
{
    if (!s || !*s) return 0;
    for (; *s; s++)
        if (char_index(*s) < 0) return 0;
    return 1;
}

/* walk down the trie following key; NULL if path does not exist */
static TrieNode *find_node(const char *key)
{
    TrieNode *cur = root;
    if (!cur || !key) return NULL;
    for (; *key; key++) {
        int i = char_index(*key);
        if (i < 0 || !cur->child[i]) return NULL;
        cur = cur->child[i];
    }
    return cur;
}

static int node_is_empty(const TrieNode *n)
{
    for (int i = 0; i < ALPHABET; i++)
        if (n->child[i]) return 0;
    return 1;
}

static void print_header(void)
{
    printf("%-5s %-22s %-5s %-18s %s\n", "ID", "Name", "Age", "Location", "Status");
    printf("--------------------------------------------------------------------\n");
}

static void print_row(const Survivor *s)
{
    printf("%-5d %-22s %-5d %-18s %s\n",
           s->id, s->name, s->age, s->location, trie_status_text(s->status));
}

/* DFS: print every survivor under n whose status matches filter (-1 = all) */
static void dfs_print(const TrieNode *n, int filter, int *count)
{
    if (!n) return;
    if (n->isEnd && (filter < 0 || n->info.status == filter)) {
        print_row(&n->info);
        (*count)++;
    }
    for (int i = 0; i < ALPHABET; i++)
        dfs_print(n->child[i], filter, count);
}

static int dfs_count(const TrieNode *n, int filter)
{
    int c = 0;
    if (!n) return 0;
    if (n->isEnd && (filter < 0 || n->info.status == filter)) c = 1;
    for (int i = 0; i < ALPHABET; i++)
        c += dfs_count(n->child[i], filter);
    return c;
}

/* returns 1 if node n can now be freed by its parent */
static int delete_rec(TrieNode *n, const char *key, int *deleted)
{
    if (*key == '\0') {
        if (!n->isEnd) return 0;
        n->isEnd = 0;
        memset(&n->info, 0, sizeof(Survivor));
        *deleted = 1;
        return node_is_empty(n);
    }
    int i = char_index(*key);
    if (i < 0 || !n->child[i]) return 0;

    if (delete_rec(n->child[i], key + 1, deleted)) {
        free(n->child[i]);
        n->child[i] = NULL;
        return !n->isEnd && node_is_empty(n);
    }
    return 0;
}

static void free_rec(TrieNode *n)
{
    if (!n) return;
    for (int i = 0; i < ALPHABET; i++)
        free_rec(n->child[i]);
    free(n);
}

/* ---------- public functions ---------- */

int trie_insert(Survivor s)
{
    if (!valid_name(s.name)) return 0;
    if (!root) {
        root = new_node();
        if (!root) return 0;
    }

    TrieNode *cur = root;
    for (const char *p = s.name; *p; p++) {
        int i = char_index(*p);
        if (!cur->child[i]) {
            cur->child[i] = new_node();
            if (!cur->child[i]) return 0;
        }
        cur = cur->child[i];
    }

    if (cur->isEnd) return 0;          /* already registered */
    cur->isEnd = 1;
    cur->info  = s;
    total++;
    return 1;
}

Survivor *trie_search(const char *name)
{
    TrieNode *n = find_node(name);
    return (n && n->isEnd) ? &n->info : NULL;
}

int trie_delete(const char *name)
{
    int deleted = 0;
    if (!root || !valid_name(name)) return 0;
    delete_rec(root, name, &deleted);
    if (deleted) total--;
    return deleted;
}

int trie_update_status(const char *name, int status)
{
    Survivor *s = trie_search(name);
    if (!s || status < STATUS_MISSING || status > STATUS_EVACUATED) return 0;
    s->status = status;
    return 1;
}

int trie_update_location(const char *name, const char *loc)
{
    Survivor *s = trie_search(name);
    if (!s || !loc) return 0;
    snprintf(s->location, LOC_LEN, "%s", loc);
    return 1;
}

int trie_prefix_search(const char *prefix)
{
    int count = 0;
    TrieNode *start = find_node(prefix);
    if (!start) {
        printf("No survivors found starting with \"%s\".\n", prefix ? prefix : "");
        return 0;
    }
    print_header();
    dfs_print(start, -1, &count);
    if (count == 0)
        printf("No survivors found starting with \"%s\".\n", prefix);
    return count;
}

int trie_starts_with(const char *prefix)
{
    TrieNode *n = find_node(prefix);
    return (n && dfs_count(n, -1) > 0) ? 1 : 0;
}

void trie_display_all(void)
{
    int count = 0;
    if (!root || total == 0) {
        printf("No survivors registered yet.\n");
        return;
    }
    print_header();
    dfs_print(root, -1, &count);
    printf("Total: %d\n", count);
}

int trie_list_by_status(int status)
{
    int count = 0;
    if (!root) {
        printf("No survivors registered yet.\n");
        return 0;
    }
    print_header();
    dfs_print(root, status, &count);
    if (count == 0)
        printf("No survivors with status %s.\n", trie_status_text(status));
    return count;
}

int trie_count(void)
{
    return total;
}

int trie_count_by_status(int status)
{
    return dfs_count(root, status);
}

const char *trie_status_text(int status)
{
    switch (status) {
        case STATUS_MISSING:   return "Missing";
        case STATUS_SAFE:      return "Safe";
        case STATUS_INJURED:   return "Injured";
        case STATUS_EVACUATED: return "Evacuated";
        default:               return "Unknown";
    }
}

void trie_free(void)
{
    free_rec(root);
    root  = NULL;
    total = 0;
}
