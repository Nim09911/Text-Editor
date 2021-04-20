#include "buffer.h"

//initiate the gap buffer
void init_gbuf(gbuf* buf) {

    //BUF_SIZE is defined as 1
    if( (buf->buffer = (char*)malloc(sizeof(char)*BUF_SIZE)) ) {
        buf->size = buf->gap_size = BUF_SIZE;
        buf->gap_left = 0;
        buf->buffer[0] = '\n';
        buf->render_row_size = 0;
    }
    return;

}

//destroy the buffer by freeing the buffer
void destroy_gbuf(gbuf* buf) {

    free(buf->buffer);
    buf->gap_left = 0;
    buf->gap_size = 0;
    buf->render_row_size = 0;
    return;
}

//doubles the size of the buffer
void grow_gbuf(gbuf* buf) {

        //gap_right_len = size - gap_right
        //gap_right = gap_left + gap_size
        size_t gap_right_len = buf->size - buf->gap_size - buf->gap_left;
        
        if( (buf->buffer = (char*)realloc(buf->buffer, buf->size*2)) ) {
            buf->gap_size = buf->size;
            buf->size *= 2;
            //memmove copies data to an intermediate buffer and then copies it to the destination
            //overcomes memcpy() overlap problem
            memmove(buf->buffer + buf->gap_left + buf->gap_size, buf->buffer + buf->gap_left, gap_right_len);
        }

    return;
}

//moves the gap left by one position
void left_gbuf(gbuf* buf) {

    if(buf->gap_left > 0) {
        //gap right = gap_left + gap_size
        buf->buffer[buf->gap_left + buf->gap_size -1] = buf->buffer[buf->gap_left -1];
        buf->gap_left--;
    }
    
    return;

}

//moves the gap right by one positon
void right_gbuf(gbuf* buf) {

    size_t gap_right_len = buf->size - buf->gap_size - buf->gap_left;
    if(gap_right_len > 0) {
        buf->buffer[buf->gap_left] = buf->buffer[buf->gap_left + buf->gap_size];
        buf->gap_left++;
    }
}

//for initially opening a file and merging buffers
void insert_gbuf(gbuf* buf, char* c, size_t length, size_t pos) {

    while(pos != buf->gap_left) {

        if(pos < buf->gap_left)
            left_gbuf(buf);
        else
            right_gbuf(buf);
    }

    while(buf->gap_size < length)
        grow_gbuf(buf);

    memmove(buf->buffer + buf->gap_left, c, length);
    buf->gap_left += length;
    buf->gap_size -= length;

    return;
}

//basically the insert function for char
void insert_char_gbuf(gbuf* buf, char c, size_t pos) {

    while(pos != buf->gap_left) {

        if(pos < buf->gap_left)
            left_gbuf(buf);
        else
            right_gbuf(buf);
    }

    while(buf->gap_size < 1)
        grow_gbuf(buf);


    buf->buffer[buf->gap_left] = c;
    buf->gap_left++;
    buf->gap_size--;

    return;
}

//writes a buffer to a file
void write_to_file_gbuf(gbuf buf, FILE* filename) {

    //first write the left half
    fwrite(buf.buffer, 1, buf.gap_left, filename);
    
    //then write the right half
    size_t gap_right = buf.gap_left + buf.gap_size;
    size_t gap_right_len = buf.size - buf.gap_size - buf.gap_left;
    fwrite(buf.buffer + gap_right, 1, gap_right_len, filename);
    
    return;
    
}
