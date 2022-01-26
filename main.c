/*
 * cli-tube, just a simple youtube viewer
 * Copyright (C) 2021  vech
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "main.h"

void error(char *c)
{
    fprintf(stderr, "%s: %s\n", c, strerror(errno));
    exit(1);
}

list *list_create(char title[], char id[])
{
    list *x = (list *)malloc(sizeof(list));
    sscanf(title, "%199[^\n]", x->name);
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
                fprintf(stderr, "List index out of range");
                exit(1);
            }
        }
    }
    printf("title: %s\n", p->name);
    printf("id: %s\n\n", p->id);
}

list *list_get_node(list *x, int index)
{
    list *p;
    for (p = x; index != 0; p = p->next, index--) {
        if (p == NULL) {
            fprintf(stderr, "List index out of range");
            exit(1);
        }
    }
   return p; 
}

list *parse_stream(FILE *file)
{
    int i;
    list *videos = list_create("", "");
    char buffer_id[ID_LENGTH], buffer_title[TITLE_LENGTH];
    do {
        i = fscanf(file, "\"videoRenderer\":{\"videoId\":\"%11s\"", buffer_id);
        if (i == 1){
            do{
                i = fscanf(file, "\"title\":{\"runs\":[{\"text\":%199[^}]\"}", buffer_title);
                if (i == 1){
                    list_append(videos, buffer_title, buffer_id);
                    break;
                }
            } while (fscanf(file, "%*c") != EOF);
        }
    } while (fscanf(file, "%*c") != EOF);
    return videos;
}

char *argv_concat(char *concat_char, char **argv, int argc)
{
    int i;
    size_t concat_length = 0;
    concat_char[0] = '\0';
    
    for (i = 1; i < argc; i++){
        concat_length += strlen(argv[i]);
    }

    concat_length += argc - 2;
    if (concat_length > TITLE_LENGTH - 1){
        fprintf(stderr, 
                "Input characters exceded: you can enter %i "
                "chars at most not %i\n", 
                TITLE_LENGTH - 1, (int)concat_length);
        exit(1);
    }

    strcat(concat_char, argv[1]);
    for (i = 2; i < argc; i++) {
        strcat(concat_char, "+");
        strcat(concat_char, argv[i]);
    }
    return concat_char;
}

int main(int argc, char **argv)
{
    char query[TITLE_LENGTH];

    if (argc < 2) {
        fprintf(stderr, "You must provide at least 1 argument not %i\n", argc - 1);
        fprintf(stderr, "Usage: cli-tube <query>...\n");
        exit(1);
    }

    argv_concat(query, argv, argc);
    char curl_query[TITLE_LENGTH + 43] = {"https://www.youtube.com/results?search_query="};
    strcat(curl_query, query);

    int curl_main_pipe[2];
    if (pipe(curl_main_pipe) == -1) {
        error("Can't create the pipe");
    }

    pid_t curl_pid = fork();

    if (curl_pid == -1) {
        error("Can't fork curl process");
    } else if (!curl_pid) {
        close(curl_main_pipe[0]);
        dup2(curl_main_pipe[1], 1);
        if (execlp("curl", "curl", "--silent",curl_query, NULL) == -1) {
            error("Can't run curl command");
        }
    }
    dup2(curl_main_pipe[0], 0);
    close(curl_main_pipe[1]);

    list *videos = parse_stream(stdin);
    if (waitpid(curl_pid, NULL, 0) == -1) {
        error("Can't wait curl process");
    }
    close(curl_main_pipe[0]);
    if (!freopen("/dev/tty", "r", stdin)) {
        error("/dev/tty");
    }

    list *q;
    int i = 0;
    char index_video[3];
    do {
        printf(
        " ====================================================================\n"
        " |                      YOUTUBE RESULTS                             |\n"
        " ====================================================================\n"
        );
        for (q = videos->next, i = 1; q != NULL; q = q->next, i++) {
            printf("%3d. %s\n", i, q->name);
        }
        printf("Select one video: ");
        fgets(index_video, 3, stdin);
        printf("\n\n");
    } while (atoi(index_video) >= i || atoi(index_video) < 1);
    
    q = list_get_node(videos, atoi(index_video));
    char url_watch[45];
    sprintf(url_watch, "https://www.youtube.com/watch?v=%s", q->id);
    list_free(videos);

    if (execlp("mpv", "mpv", url_watch, NULL) == -1) {
        error("Can't run mpv");
    }
    return 0;
}
