/* See LICENSE file for copyright and license details. */

#define TITLE_LENGTH 200
#define ID_LENGTH 12

typedef struct list {
    char id[ID_LENGTH];
    char title[TITLE_LENGTH];
    struct list *next;
} list;

void error(char *c);
list *list_create(char title[], char id[]);
void list_free(list *start);
void list_append(list *list_first, char title[], char id[]);
void list_print_item(list *x, int i);
list *list_get_node(list *x, int index);
list *parse_stream(FILE *file);
