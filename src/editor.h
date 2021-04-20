#ifndef EDITOR_H
#define EDITOR_H

#include "buffer.h"
#include <ncurses.h>

#define CTRL(x) ((x) & 31)
#define TAB_SIZE 4

//structure of editor
typedef struct editor {

    //size and position
    size_t y, x;                //cursor position
    size_t y_offset, x_offset;  //for scrolling
    size_t y_max, x_max;        //for page size
    size_t numrows;             //total rows
    
    //filename
    char* filename;             //to open and save
    
    //array of of text buffers
    gbuf* text;                 

    //editor states
    unsigned char read;
    unsigned char unsaved;

}editor;

void init_editor(editor* E);

void show_gbuf(editor E, size_t row);
void print_page(editor E);

void insert_tabs(editor* E, size_t row);

void merge_buffer_backspace(editor* E, size_t row);
void shrink_editor(editor* E);
void backspace_gbuf(editor* E, gbuf* buf);
void clear_line(editor* E, size_t row);

void expand_editor(editor* E);
char* split_buffer_enter(editor E, size_t row);
void expand_editor_enter(editor* E);

void resize_check(editor* E);

void move_cursor(editor* E, char c);

#endif