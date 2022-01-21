#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "main.h"

void error(char *c)
{
    fprintf(stderr, "%s: %s\n", c, strerror(errno));
    exit(1);
}

list *list_create(char title[], char id[])
{
    list *x = malloc(sizeof(list));
    sscanf(title, "%100[^\n]", x->name);
    sscanf(id, "%11s", x->id);
    x->next = NULL;
    return x;
}

void list_free(list *start)
{
    list *i;
    list *next = NULL;
    for (i = start; i != NULL; i = next) {
        next = i->next;
        free(i);
    }
}

void list_append(list *list_first, char title[], char id[])
{
    list *i; 
    for(i = list_first; i->next != NULL; i = i->next);
    i->next = list_create(title, id);
}

void list_print_item(list *x, int i)
{
    list *p;
    if (i == -1){
        for (p = x; p->next != NULL; p = p->next);
    } else {
        for(p = x; i != 0; p = p->next, i--){
            if (p == NULL) {
                printf("List index out of range");
                exit(1);
            }
        }
    }
    printf("title: %s\n", p->name);
    printf("id: %s\n\n", p->id);
}

list *parse_stream(FILE *file)
{
    list *videos = list_create("", "");
    return videos;
}

int main(int argc, char **argv)
{
    FILE *file;
    list *videos = list_create("", "");
    char buffer_id[ID_LENGTH], buffer_name[TITLE_LENGTH];
    int i;

    if (argc != 2){
        printf("You must provide 1 argument not %i\n", argc - 1);
        exit(1);
    }

    if (!strcmp(argv[1], "-")){
        file = stdin;

    } else {
        file = fopen(argv[1], "r");
        if (file == NULL){
            error("Can't open file stream");
        }
    }

    do {
        i = fscanf(file, "\"videoRenderer\":{\"videoId\":\"%11s\"", buffer_id);
        if (i == 1){
            do{
                i = fscanf(file, "\"title\":{\"runs\":[{\"text\":\"%100[^\"]\"", buffer_name);
                if (i == 1){
                    list_append(videos, buffer_name, buffer_id);
                    break;
                }
            } while (fscanf(file, "%*c") != EOF);
        }
    } while (fscanf(file, "%*c") != EOF);

    list *q;
    for (q = videos; q != NULL; q = q->next){
        printf("title: %s\n", q->name);
        printf("vidId: %s\n", q->id);
        printf("\n\n");
    }

    list_free(videos);
    return 0;
}
