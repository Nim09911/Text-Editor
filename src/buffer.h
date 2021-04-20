#ifndef BUFFER_H
#define BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 1

//structure of buffer which represents a row
typedef struct gap_buffer {

    //size_t could be any of unsigned char, unsigned short, 
    //unsigned int, unsigned long or unsigned long long, 
    //depending on the implementation.
    //SOURCE: Stackoverflow
    
    char* buffer;
    size_t size;
    size_t gap_left;
    size_t gap_size;
    size_t render_row_size;

}gbuf;

void init_gbuf(gbuf* buf);
void destroy_gbuf(gbuf* buf);

void grow_gbuf(gbuf* buf);

void left_gbuf(gbuf* buf);
void right_gbuf(gbuf* buf);

void insert_gbuf(gbuf* buf, char* c, size_t length, size_t pos);
void insert_char_gbuf(gbuf* buf, char c, size_t pos);

void write_to_file_gbuf(gbuf buf, FILE* filename);

#endif