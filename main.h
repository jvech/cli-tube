#define TITLE_LENGTH 200
#define ID_LENGTH 12

typedef struct list {
    char id[ID_LENGTH];
    char name[101];
    struct list *next;
} list;
