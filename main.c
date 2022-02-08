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
#include <ncurses.h>
#include <locale.h>
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
    sscanf(title, "%199[^\n]", x->title);
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
    printf("title: %s\n", p->title);
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

void draw_options(list *videos, int video_index, int length)
{
    char *title;
    int i, xmax, ymax;
    getmaxyx(stdscr, ymax, xmax);
    clear();
    move(0, 0);
    i = (video_index/ymax) * ymax;
    for (i = (video_index/ymax) * ymax; i <= length && getcury(stdscr) < ymax - 1; i++) {
        if (i == video_index) attron(A_REVERSE);
        else attroff(A_REVERSE);
        if (!i) continue;

        printw("%2d. ", i);
        title = list_get_node(videos, i)->title;
        addnstr(title, xmax - 15);
        if (strlen(title) > xmax - 15) addstr("...");
        addch('\n');
        attroff(A_REVERSE);
    }
    refresh();
}

int get_user_video(list *videos)
{
    list *q;
    int length, key;
    int video_index = 1;

    for (q = videos->next, length = 0; q != NULL; q = q->next, length++);

    setlocale(LC_ALL, "");
    initscr();

    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    draw_options(videos, video_index, length);
    do {
        key = getch();
        switch (key) {
            case KEY_UP:
            case 'k':
                video_index--;
                break;
            case KEY_DOWN:
            case 'j':
                video_index++;
                break;
            default:
                break;
        }
        if (video_index > length) video_index = 1;
        else if (video_index < 1) video_index = length;
        draw_options(videos, video_index, length);
    } while(key != '\n');

    echo();
    curs_set(1);
    endwin();
    return video_index;
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

    int curl_exit_status;
    if (waitpid(curl_pid, &curl_exit_status, 0) == -1) {
        error("Can't wait curl process");
    }

    if (WEXITSTATUS(curl_exit_status) < 0) {
        fprintf(stderr, "curl exit error, exit status: %i", 
                WEXITSTATUS(curl_exit_status));
    }


    close(curl_main_pipe[0]);
    if (!freopen("/dev/tty", "r", stdin)) {
        error("/dev/tty");
    }

    list *q;
    int index_video;
    index_video = get_user_video(videos);
    
    q = list_get_node(videos, index_video);
    char url_watch[45];
    sprintf(url_watch, "https://www.youtube.com/watch?v=%s", q->id);
    list_free(videos);

    if (execlp("mpv", "mpv", url_watch, NULL) == -1) {
        error("Can't run mpv");
    }
    return 0;
}
