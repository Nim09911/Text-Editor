#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//no resize function must be done manually by changing macros

#define CTRL(x) ((x) & 31)
#define BUF_SIZE 1
#define PAGE_SIZE_Y 38
//dispalys PAGE_SIZE_X + 10, scrolling will start at PAGE_SIZE_X
#define PAGE_SIZE_X 130
#define TAB_SIZE 4
void move_cursor(char c);

//structure of buffer which represents a row
typedef struct gap_buffer {

    //size_t could be any of unsigned char, unsigned short, 
    //unsigned int, unsigned long or unsigned long long, 
    //depending on the implementation.
    char* buffer;
    size_t size;
    size_t gap_left;
    size_t gap_size;
    
    int render_row_size;

}gbuf;

//structure of editor
typedef struct editor {

    //size and position
    int y, x;
    int y_offset, x_offset;
    int numrows;
    
    //filename
    char* filename;
    
    //array of of text buffers
    gbuf* text;

    //editor states
    int read;
    int unsaved;

}editor;

//global variable
static editor E;

//initiates the editor structure
void init_editor(editor* E) {

    E->y = 0;
    E->x = 0;
    E->y_offset = 0;
    E->x_offset = 0; 
    E->numrows = 0;
    E->filename = NULL;
    E->text = NULL;
    E->read = 0;
    E->unsaved = 0;
}

//enable screen and raw mode
void init_raw() {
    
    //clear screen on opening
    clear();
    //init ncurses screen (stdscr)
    initscr();
    //read character by character, disable original functions of CTRL Keys
    //non canonical/raw mode    
    raw();
    //fix the delay time taken to read ESC
    set_escdelay(1);
    keypad(stdscr, TRUE);
    //disable echo
    noecho();

}

//disables screen and raw mode
void disable_raw() {

    //turn echo back on
    echo();
    //disable raw mode    
    noraw();
    //clear screen exit    
    clear();
    //close ncurses window on exit
    endwin();

}

//prints cursor, filename, page and mode information
void print_info() {

    move(0,0);
    clrtoeol();
    attron(A_STANDOUT);

    //print file infor    
    if(E.filename)
        mvaddstr(0,0,E.filename);
    else
        mvaddstr(0,0,"unsaved file");

    //print cursor location info    
    move(0, 75);
    printw("row: %d col: %d line_len: %d numrows: %d",
        E.y+(E.y_offset*PAGE_SIZE_Y) , E.x+E.x_offset, E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1].render_row_size, E.numrows);
    
    //print mode information
    move(PAGE_SIZE_Y+1, 0);
    clrtoeol();
    if(!E.read)
        addstr("--INSERT--");
    else
        addstr("--READ--");

    //calculations for total pages
    int pages = E.numrows/PAGE_SIZE_Y;
    if(E.numrows % PAGE_SIZE_Y != 0)
        pages++;

    //calculations for current page
    int cpage = (E.y + E.y_offset*PAGE_SIZE_Y)/PAGE_SIZE_Y;
    if( (E.y + E.y_offset*PAGE_SIZE_Y)%PAGE_SIZE_Y != 0)
        cpage++;

    move(PAGE_SIZE_Y+1,  75);
    printw("Pages %d out of %d", cpage, pages);
    
    attroff(A_STANDOUT);
    
    //move cursor back to original position    
    move(E.y, E.x);
    refresh();

}

//initiate the gap buffer
void init_gbuf(gbuf* buf) {

    //BUF_SIZE id defined as 1
    if(buf->buffer = (char*)malloc(sizeof(char)*BUF_SIZE) ) {
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

        //gap_right_len = size - gap_right + 1
        //gap_right = gap_left + gap_size -1
        size_t gap_right_len = buf->size - buf->gap_size - buf->gap_left;
        
        if(buf->buffer = (char*)realloc(buf->buffer, buf->size*2)) {
            buf->gap_size += buf->size;
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
        //gap right = gap_left + gap_size -1
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


//renders the buffer
//please call move(E.y, 0) and clrtoeol before this function
//and call refresh() after this function
void show_gbuf(editor E, int row) {


    int index = row-1;    
    int gap_right = E.text[index].gap_left + E.text[index].gap_size; //-1
    size_t gap_right_len = E.text[index].size - gap_right; //+1
    
    E.text[index].render_row_size = E.text[index].gap_left + gap_right_len;

    char* render_row = (char*)malloc(E.text[index].render_row_size);
    
    int i = 0;
    for(i; i < E.text[index].gap_left; i++) {
        render_row[i] = E.text[index].buffer[i];
    }
    for(int j = gap_right; j < E.text[index].size; i++,j++) {
        render_row[i] = E.text[index].buffer[j];
    }

 
    //print like this becuase there mvaddstr/mvprintw causes garbage values
    int start = 0 + E.x_offset;
    //renders text of length PAGE_SIZE_X + 10
    //just for better scrolling experience    
    int end = start + PAGE_SIZE_X+10;
    
    if(end > E.text[index].render_row_size)
        end = E.text[index].render_row_size;

    for(int i = 0 + E.x_offset; i < end; i++ )
        printw("%c", render_row[i]);

    free(render_row);
    move(E.y, E.x);

}

//renders the entire page
void print_page(editor E) {

    int y = E.y, x = E.x;
    int start = 1 + E.y_offset*PAGE_SIZE_Y;
    int end = start + PAGE_SIZE_Y;
    if(end > E.numrows) {
        end = E.numrows+1;
        for(int i = 1; i <= PAGE_SIZE_Y; i++) {
            move(i, 0);
            clrtoeol();
        }
    }
    for(int i = start, j = 1; i < end; i++, j++) {
        move(j, 0);
        clrtoeol();
        show_gbuf(E, i);
    }
    E.y = y;
    E.x = x;
    move(E.y, E.x);
    refresh();
}


//for initially opening a file and merging buffers
void insert_gbuf(gbuf* buf, char* c, size_t length) {

    while(E.x+E.x_offset != buf->gap_left) {

        if(E.x+E.x_offset < buf->gap_left)
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
void insert_char_gbuf(gbuf* buf, char c) {

    while(E.x+E.x_offset != buf->gap_left) {

        if(E.x+E.x_offset < buf->gap_left)
            left_gbuf(buf);
        else
            right_gbuf(buf);
    }

    while(buf->gap_size < 1) {
        grow_gbuf(buf);
    }


    buf->buffer[buf->gap_left] = c;
    buf->gap_left++;
    buf->gap_size--;

    return;


}

//inserts tab
void insert_tabs(editor* E, int row) {

    int index = row-1;
    //calculation to help move to nearest multiple of TAB_SIZE
    int tab_minus =  (E->x + E->x_offset)%TAB_SIZE;
    for(int i = 0; i < TAB_SIZE-tab_minus; i++) {
        insert_char_gbuf(&E->text[index], (char)32 );
        move(E->y, ++E->x);
    }

    //adjust the offsets if any
    if(E->x > PAGE_SIZE_X) {
        E->x_offset = E->x - PAGE_SIZE_X;
        E->x = PAGE_SIZE_X;
        move(E->y, E->x);
        print_page(*E);
        refresh();
    }
    //if no offset
    else {
        move(E->y, 0);
        clrtoeol();
        show_gbuf(*E, row);
        move(E->y, E->x);
        refresh();
    }
    //adjust state
    E->unsaved = 1;
    return;
}


void merge_buffer_backspace(editor* E, int row) {

    int index = row-1;

    int gap_right = E->text[index].gap_left + E->text[index].gap_size;
    size_t gap_right_len = E->text[index].size - gap_right;
    
    E->text[index].render_row_size = E->text[index].gap_left + gap_right_len;

    char* render_row = (char*)malloc(E->text[index].render_row_size);
    
    int i = 0;
    for(i; i < E->text[index].gap_left; i++)
        render_row[i] = E->text[index].buffer[i];

    for(int j = gap_right; j < E->text[index].size; i++,j++)
        render_row[i] = E->text[index].buffer[j];

    int x = E->x, x_offset = E->x_offset, y = E->y, y_offset = E->y_offset; 
    
    E->x = E->text[index-1].render_row_size;
    E->y = E->y-1;
    insert_gbuf(&E->text[index-1], render_row, E->text[index].render_row_size);

    E->x = x;
    E->x_offset = x_offset;
    E->y = y; 
    E->y_offset = y_offset;

    free(render_row);
    
    return;

}

//shrink page
void shrink_editor(editor* E) {

    int x = E->text[E->y+(E->y_offset*PAGE_SIZE_Y)-2].render_row_size+1;    
    int x_offset = 0;
    if(x > PAGE_SIZE_X) {
        x_offset = x - PAGE_SIZE_X;
        x = PAGE_SIZE_X;
    }
    if(E->text[E->y+(E->y_offset*PAGE_SIZE_Y)-1].render_row_size)
        merge_buffer_backspace(E, E->y+(E->y_offset*PAGE_SIZE_Y));

    destroy_gbuf(&E->text[E->y+(E->y_offset*PAGE_SIZE_Y)-1]);
    if(E->y+(E->y_offset*PAGE_SIZE_Y) != E->numrows) {
        for(int i = E->y+(E->y_offset*PAGE_SIZE_Y)-1; i < E->numrows-1; i++) {
            E->text[i] = E->text[i+1];
        }
    }
    E->numrows--;    
    move_cursor((char)KEY_UP);
    E->x = x;
    E->x_offset = x_offset;
    move(E->y, E->x);
    print_page(*E);
}

//delete character from the buffer
void backspace_gbuf(gbuf* buf) {

    if(E.x + E.x_offset != 0) {

        while(E.x + E.x_offset != buf->gap_left) {

            if(E.x + E.x_offset < buf->gap_left)
                left_gbuf(buf);
            else
                right_gbuf(buf);
        }
        //buf->buffer[buf->gap_left] = 0;
        buf->gap_left--;
        buf->gap_size++;
    }
    else if(E.y == 1 && !E.y_offset) {
    }
    else  
        shrink_editor(&E);

    return;
}

//clears the current line CTRL+D
void clear_line(editor* E, int row) {

    int index = row-1;
    destroy_gbuf(&E->text[index]);
    init_gbuf(&E->text[index]);
    move_cursor((char)KEY_HOME);
    print_page(*E);
    E->unsaved = 1;
    return;

}


//expand page by appending a buffer
void expand_editor(editor* E) {

    E->text = (gbuf*)realloc(E->text, sizeof(gbuf)*(E->numrows + 1) );
    init_gbuf(&E->text[E->numrows]);
    if(E->y+(E->y_offset*PAGE_SIZE_Y)-1 != E->numrows) {
        //memmove(E->text + E->numrows, E->text + E->y-1, sizeof(gbuf)*(E->numrows - E->y+1));
        for(int i = E->numrows; i >= (E->y+(E->y_offset*PAGE_SIZE_Y)); i--) {
            E->text[i] = E->text[i-1];    
        }
        init_gbuf(&E->text[E->y+(E->y_offset*PAGE_SIZE_Y)-1]);
    }
    E->numrows++;
    print_page(*E);

    return;
}

//helps split the buffer if enter was pressed
char* split_buffer_enter(editor E, int row) {

    //first render the entire row
    int index = row-1;
    int gap_right = E.text[index].gap_left + E.text[index].gap_size;
    size_t gap_right_len = E.text[index].size - gap_right;
    
    E.text[index].render_row_size = E.text[index].gap_left + gap_right_len;
    char* render_row = (char*)malloc(E.text[index].render_row_size);
    
    int i = 0;
    for(i; i < E.text[index].gap_left; i++)
        render_row[i] = E.text[index].buffer[i];

    for(int j = gap_right; j < E.text[index].size; i++,j++)
        render_row[i] = E.text[index].buffer[j];

    //now extract only the part to split
    int split_buffer_size = E.text[index].render_row_size - E.x-E.x_offset;
    char* split_buffer = (char*)malloc(split_buffer_size);

    for(int j = E.x+E.x_offset, k = 0; j < E.text[index].render_row_size; j++, k++) {
        split_buffer[k] = render_row[j];
    }
    free(render_row);
    
    //return the split part
    return split_buffer;
}

//expand page by inserting a buffer
void expand_editor_enter(editor* E) {

    char* temp = NULL;
    if(E->x + E->x_offset != E->text[E->y+(E->y_offset*PAGE_SIZE_Y)-1].render_row_size)
        temp = split_buffer_enter(*E, E->y+(E->y_offset*PAGE_SIZE_Y));

    E->text = (gbuf*)realloc(E->text, sizeof(gbuf)*(E->numrows + 1) );
    init_gbuf(&E->text[E->numrows]);

    if(E->y-1 != E->numrows) {
        //memmove(E->text + E->numrows, E->text + E->y-1, sizeof(gbuf)*(E->numrows - E->y+1));
        for(int i = E->numrows; i >  E->y+(E->y_offset*PAGE_SIZE_Y); i--) {
            E->text[i] = E->text[i-1];    
        }
        init_gbuf(&E->text[E->y+(E->y_offset*PAGE_SIZE_Y)]);
    }

    if(temp != NULL) {
        
        int len = E->text[E->y+(E->y_offset*PAGE_SIZE_Y)-1].render_row_size - E->x - E->x_offset;
        insert_gbuf(&E->text[E->y+(E->y_offset*PAGE_SIZE_Y)], temp, len);

        //delete the split part form the prev buffer
        int y = E->y, x = E->x, x_offset = E->x_offset;
        E->x = E->text[E->y+(E->y_offset*PAGE_SIZE_Y)-1].render_row_size;
        if(E->x > PAGE_SIZE_X) {
            E->x_offset = E->x - PAGE_SIZE_X;
            E->x = PAGE_SIZE_X;
        }
        for(int j = 0; j < len; j++) {
            backspace_gbuf(&E->text[E->y+(E->y_offset*PAGE_SIZE_Y)-1]);
            move_cursor((char)KEY_LEFT);
        }

        //move back to original posn
        E->y = y;
        E->x = x;
        E->x_offset = x_offset;
        move(E->y, E->x);
    }
    free(temp);

    //update the editor
    E->numrows++;
    print_page(*E);

    return;
}


//writes a buffer to a file
void write_to_file_gbuf(gbuf buf, FILE* filename) {

    //first write the left half
    fwrite(buf.buffer, 1, buf.gap_left, filename);
    
    //then write the right half
    int gap_right = buf.gap_left + buf.gap_size;
    size_t gap_right_len = buf.size - buf.gap_size - buf.gap_left;
    fwrite(buf.buffer + gap_right, 1, gap_right_len, filename);
    
    return;
    
}

//only while when saving a file
char read_key_save() {

    int c = getch();
            
    if(c == KEY_LEFT ) {
        if(E.x > 0)
            move(E.y, --E.x);
    }
    else if(c == KEY_RIGHT) {
        if(E.x < 128);
            move(E.y, ++E.x);
    }
    else if(c == KEY_BACKSPACE) {
        if(E.x > 0) {
            backspace_gbuf(&E.text[E.numrows-1]);
            move(E.y, 0);
            clrtoeol();
            show_gbuf(E, E.numrows);
            refresh();
            move(E.y, --E.x);
        }
    }
    else if(c == 10) {
        //do nothing
    }
    else if(c == 27) {
        //do nothing
    }
    else if (c < 32 || c >= 127) {
        //non printable characters
    }
    else if(32 <= c < 127) {
            
        if(E.x < PAGE_SIZE_X) {
            insert_char_gbuf(&E.text[E.numrows-1], (char)c);
            move(E.y, 0);
            clrtoeol();
            show_gbuf(E, E.numrows);
            refresh();
            move(E.y, ++E.x);
        }        
    }
    else {
        //do nothing
    }
    refresh();

    return c;
}

//openeditor
void openeditor(char* filename) {

    init_raw();
    init_editor(&E);
    refresh();

    E.unsaved = 0;
    E.read = 0;

    FILE *file = fopen(filename, "r");
    
    if(file != NULL && !feof(file) ) {
        
        //come here if file is not empty
        char *line = NULL;
        size_t linecap = 0;
        ssize_t linelen;
        //getline function reads the line till \0 and returns the
        while((linelen = getline(&line,&linecap,file)) != -1) {
            
            //add buffers
            E.y++;
            expand_editor(&E);

            //we dont want to add the \n or \r
            //getline genrally doesnt include \0
            if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r' || line[linelen-1] == '\0'))
                linelen--;

            insert_gbuf(&E.text[E.y-1],line,linelen);
        }
        free(line);
        fclose(file);

    }
    else {
        E.y++;
        expand_editor(&E);
    }

    if(filename != NULL) {
        E.filename = filename;
        E.unsaved = 0;
    }
    else 
        E.unsaved = 1;

    E.y = 1;
    E.x = 0;
    move(1,0);
    print_info();
    print_page(E);
    refresh();

    return;
    
}

//save the current file
//prompt to save if no file
void savefile(editor* E, char* filename) {

    int y = E->y, x = E->x, y_offset = E->y_offset, x_offset = E->x_offset;
    
    if(!E->filename) {
        
        attron(A_STANDOUT);
        E->y = PAGE_SIZE_Y + 1;
        E->x = 0;
        expand_editor(E);
        mvaddstr(E->y, 0, "Enter Filename to save:");


        int c;
        while(c = read_key_save()) {
            //if enter is pressed then render then buffer and save
            if(c == 10) {
                int index = E->numrows-1;    
                int gap_right = E->text[index].gap_left + E->text[index].gap_size;
                size_t gap_right_len = E->text[index].size - gap_right;
                
                E->text[index].render_row_size = E->text[index].gap_left + gap_right_len;
                filename = (char*)malloc(E->text[index].render_row_size);
                
                int i = 0;
                for(i; i < E->text[index].gap_left; i++)
                    filename[i] = E->text[index].buffer[i];

                for(int j = gap_right; j < E->text[index].size; i++,j++)
                    filename[i] = E->text[index].buffer[j];

                E->filename = filename;

                //destroy and clear line after entering filename
                destroy_gbuf(&E->text[E->numrows-1]);
                move(E->y, 0);
                clrtoeol();
                E->y = y;
                E->x = x;
                move(E->y, E->x);
                E->numrows--;
                break;
            }
            //cancel the saving process if ESC pressed
            if(c == 27) {
                //destroy and clear after entering filename
                destroy_gbuf(&E->text[E->numrows-1]);
                E->numrows--;
                move(E->y, 0);
                clrtoeol();
                E->y = y;
                E->x = x;
                move(E->y, E->x);
                attroff(A_STANDOUT);
                refresh();             
                return;
            }
        }
    }
    attroff(A_STANDOUT);

    //now actually save the file
    FILE* file = fopen(filename, "w");
    for(int i = 0; i <= E->numrows-1; i++) {
        write_to_file_gbuf(E->text[i], file);
        fputc('\n', file);
    }

    fclose(file);
    //openeditor(filename);
    E->unsaved = 0;
    E->y = y;
    E->x = x;
    E->y_offset = y_offset;
    E->x_offset = x_offset;    
    print_page(*E);
    move(E->y, E->x);

    return;
}

//close the file
//prompt to save before exiting
void closeeditor() {

    if(E.filename == NULL || E.unsaved == 1) {
        E.y = PAGE_SIZE_Y+1; 
        move(E.y, 0);
        printw("%s","Save?[Y/n]:");
        char c = getch();
        if(c == 'y' || c == 'Y')
            savefile(&E, E.filename);
    }

    for(int i = 0; i < E.numrows; i++)
        destroy_gbuf(&E.text[i]);

    disable_raw();
    return;
}

//moves cursor
void move_cursor(char c) {

    switch(c) {

        //goes to the starting one line above
        //cant go above
        case (char)KEY_UP:
            
            if(E.y == 1 && !E.y_offset) {
                //do nothing
            }
            //if y offset
            else if(E.y == 1 && E.y_offset) {
                E.x = 0;
                E.x_offset = 0;
                E.y = PAGE_SIZE_Y;
                E.y_offset--;
                move(E.y, E.x);
                print_page(E);
            }
            //if no change in offset
            else {
                E.x = 0;
                //but if x offset
                if(E.x_offset) {
                    E.x_offset = 0;
                    print_page(E);
                }
                //move up
                move(--E.y, E.x);
            }
            break;
        
        case (char)KEY_DOWN:
                
                //increase y_offset if current line is multiple of PAGE_SIZE_Y
                //cant move below numrows
                if(E.y % PAGE_SIZE_Y == 0 && (E.y + E.y_offset*PAGE_SIZE_Y) < E.numrows) {
                    E.y_offset++;
                    E.y = 1;
                    E.x = 0;
                    E.x_offset = 0;
                    move(1,0);
                    print_page(E);
                }
                //if no change in y_offset is reqd
                else if(E.y % PAGE_SIZE_Y != 0 && (E.y + E.y_offset*PAGE_SIZE_Y) < E.numrows) {
                    E.x = 0;
                    //but if e_offset
                    if(E.x_offset) {
                        E.x_offset = 0;
                        print_page(E);
                    }
                    move(++E.y, 0);
                }
            break;
        
        case (char)KEY_LEFT:
            
            //base case
            if(E.x != 0 && !E.x_offset) {
                move(E.y, --E.x);
            }
            //if offset
            else if(E.x_offset) {
                E.x_offset--;
                print_page(E);
            }
            //if we have to go to the prev page
            //move to up prev line and then to the end of the line
            else{
                move_cursor((char)KEY_UP);
                move_cursor((char)KEY_END);
            }               
            break;
        
        case (char)KEY_RIGHT:

            //if at the end of the line move down
            if((E.x + E.x_offset) == E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1].render_row_size)
                move_cursor((char)KEY_DOWN);
            
            //else move right
            else if(E.x+E.x_offset < E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1].render_row_size) {
                
                //if no x offset move right
                if(!E.x_offset && E.x < PAGE_SIZE_X)
                    move(E.y, ++E.x);
                //just increase offset and print
                else {
                    E.x_offset++;
                    print_page(E);
                }
            }
            break;

        case (char)KEY_HOME:
            //move to the beginning of the line
            E.x = 0;
            if(E.x_offset) {
                E.x_offset = 0;
                print_page(E);
            }
            move(E.y, 0);
            break;

        case (char)KEY_END:
            //move to the end of the line
            E.x = E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1].render_row_size;
            if(E.x > PAGE_SIZE_X) {
                E.x_offset = E.x - PAGE_SIZE_X;
                E.x = PAGE_SIZE_X;
                print_page(E);
            }
            move(E.y, E.x);
            break;

        case (char)KEY_PPAGE:

            //move to the prev page
            if(E.y_offset || E.x_offset) {
                E.y_offset--;
                E.x_offset = 0;
                print_page(E);
            }
            E.y = 1;
            E.x = 0;
            move(E.y, E.x);
            break;

        case (char)KEY_NPAGE:
            
            //if next page end not E.numrows
            if( (E.y_offset+1)*PAGE_SIZE_Y < E.numrows) {
                E.y_offset++;
                E.y = 1;
                E.x = 0;
                E.x_offset = 0;
                move(E.y, E.x);
                print_page(E);
            }
            //if next page numrows
            else {
                //move to end of the last row
                E.y = E.numrows - (E.y_offset)*PAGE_SIZE_Y;
                E.x = E.text[E.numrows-1].render_row_size;
                if(E.x > PAGE_SIZE_X) {
                    E.x_offset = E.x - PAGE_SIZE_X;
                    E.x = PAGE_SIZE_X;
                    print_page(E);
                }
                move(E.y, E.x);
            }
            break;

        //if enter key is pressed
        case (char)(10):
            
            if(E.x == 0) {
                expand_editor(&E);
            }
            else {
                expand_editor_enter(&E);
            }
            move_cursor((char)KEY_DOWN);
            E.unsaved = 1;
            break;

        case (char)KEY_BACKSPACE:
            
            backspace_gbuf(&E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1]);
            if(!E.x_offset) {
                move(E.y, 0);
                clrtoeol();
                show_gbuf(E, E.y+(E.y_offset*PAGE_SIZE_Y));
            }
            move_cursor((char)KEY_LEFT);
            E.unsaved = 1;
            refresh();
            break;
    }
    refresh();
    return;

}

//process keypress for read mode
int read_key_read() {

    int ch;
        print_info();
    while(ch != 105 || ch != CTRL('q')) {
        print_info();
        ch = getch();
        if(ch == 104) //h
            ch = KEY_LEFT;
        else if (ch == 106) //j
            ch = KEY_DOWN;
        else if (ch == 107) //k
            ch = KEY_UP;
        else if (ch == 108) //l
            ch = KEY_RIGHT;
            
        if (ch == KEY_UP || ch == KEY_DOWN|| ch == KEY_LEFT || ch == KEY_RIGHT || 
            ch == KEY_HOME || ch == KEY_RIGHT || ch == KEY_END || ch == KEY_UP ||
            ch == KEY_PPAGE || ch == KEY_NPAGE) {
            move_cursor((char)ch);
        }
        else if(ch == CTRL('q') || ch == 105) {
            E.read = 0;
            return ch;
        }
    }

}

//handles and processes keypresses
int read_key() {

    int c = getch();

    //ESC key 
    if(c == 27) {
        E.read = 1;
        c = read_key_read();
    }
    //TAB key (CTRL + i)
    else if (c == 9) {
        insert_tabs(&E, E.y + E.y_offset*(PAGE_SIZE_Y));
    }
    else if(c == CTRL('q')) {
        //essentially break from here
    }
    else if(c == CTRL('d')) {
        clear_line(&E, E.y+(E.y_offset)*PAGE_SIZE_Y);
    }
    else if(c == CTRL('s') ) {
        savefile(&E, E.filename);
    }
    else if(c == KEY_UP || c == KEY_DOWN|| c == KEY_LEFT || c == KEY_RIGHT || 
            c == KEY_HOME || c == KEY_RIGHT || c == KEY_END || c == KEY_UP ||
            c == KEY_PPAGE || c == KEY_NPAGE || c == KEY_BACKSPACE || c == 10 
            ) {
                move_cursor((char)c);
    }
    else if (c < 32 || c > 127) {
        //non printable characters
    }
    else if(32 <= c < 127 ) {
        
        //insert
        insert_char_gbuf(&E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1], (char)c);
        
        //cursor movement
        if(E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1].render_row_size > PAGE_SIZE_X) {
            //this is just so to manage the cursor movement of KEY_RIGHT
            //the actual value is calculated while print_page is called
            //coincidenlty they will be the same
            E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1].render_row_size++;
            move_cursor((char)KEY_RIGHT);
            //print again as to take care of case if render_row_size > PAGE_SIZE_X
            //but currently we are inserting before that posn
            if(!E.x_offset)
                print_page(E);
        }
        else {
            move(E.y, 0);
            clrtoeol();
            show_gbuf(E, (&E)->y+((&E)->y_offset*PAGE_SIZE_Y));
            refresh();
            move_cursor((char)KEY_RIGHT);
        }
        E.unsaved = 1;
        refresh();
    }    

    return c;
}

//main
int main(int argc, char* argv[]) {

    char* filename = NULL;
    
    if(argc > 1)
        filename = argv[1];
       
    openeditor(filename);

    print_info();
    while(read_key() != CTRL('q')) {
        print_info();
    }

    closeeditor();

    return 0;
}
