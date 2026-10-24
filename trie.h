#ifndef TRIE_H
#define TRIE_H



#define SURV_NAME_LEN   30
#define LOC_LEN    30
#define ALPHABET   27          /* 'a'-'z' plus the space character */

#define STATUS_MISSING    0
#define STATUS_SAFE       1
#define STATUS_INJURED    2
#define STATUS_EVACUATED  3

typedef struct {
    int  id;
    char name[SURV_NAME_LEN];
    int  age;
    char location[LOC_LEN];
    int  status;               /* one of the STATUS_ values */
} Survivor;

typedef struct TrieNode {
    struct TrieNode *child[ALPHABET];
    int      isEnd;            // 1 if a survivor name ends here 
    Survivor info;             // valid only when isEnd == 1 
} TrieNode;

/* Insert / search / delete */
int       trie_insert(Survivor s);                 // 1 = added, 0 = duplicate/invalid name 
Survivor *trie_search(const char *name);           // NULL if not found (case-insensitive) 
int       trie_delete(const char *name);           // 1 = deleted, 0 = not found 

/* Updates */
int trie_update_status(const char *name, int status);       // 1 = ok, 0 = fail 
int trie_update_location(const char *name, const char *loc);

/* Searching and listing */
int  trie_prefix_search(const char *prefix);       // prints matches, returns count 
int  trie_starts_with(const char *prefix);         // 1 if any name has this prefix 
void trie_display_all(void);                       // alphabetical order 
int  trie_list_by_status(int status);              // prints matches, returns count 

/* Counting */
int trie_count(void);
int trie_count_by_status(int status);

/* Helpers */
const char *trie_status_text(int status);
void        trie_free(void);                       /* free all memory at program exit */

#endif
