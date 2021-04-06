#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*FIXES REQUIRED
    FIXED (using clrteoel):1.printing of cursor position due to transition from ones to tens to hundreds place and then back
    2. realloc error while adding rows
    3. backspace not working propally
    FIXED (using int c = getch()): 4. letter h not being read because of END, sim R and T... (considering to read ESC seq)
    
*/

/*TODO
    DONE        0.Make a structure for the entire page
    DONE        1.Make Buffer to store, display and manipulate text
    DONE        2.Read from file and display -> Text viewer
    NEEDS WORK 3.Handle more complex keypresses
    NEEDS WORK 4.Manipulate text -> Text editor
    DONE        5.Save text
                //6.Add syntax highlighting
                //7.Resizing of the window
                8.Scrolling
*/

/*DONE
    1.enable and disable rawmode
    2.add simple cursor movements
    3.add editor structre
    4.add buffer structure
*/

#define CTRL(x) ((x) & 31)
#define BUF_SIZE 1
#define PAGE_SIZE_Y 40
#define PAGE_SIZE_X 128
void move_cursor(char c);

//structure of buffer which represents a row
typedef struct gap_buffer {

    char* buffer;
    size_t size;
    size_t gap_left;
    size_t gap_size;
    
    char* render_row;
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

static editor E;

void init_editor(editor* E) {

    E->y = 0;
    E->x = 0;
    E->y_offset = 0;
    E->x_offset = 0; 
    E->numrows = 0;
    E->filename = NULL;
    E->text = NULL;
    E->read = 1;
    E->unsaved = 0;
}

//enable screen and raw mode
void init_raw() {
    
    clear();
    initscr();    
    raw();
    keypad(stdscr, TRUE);
    scrollok(stdscr, TRUE);
    noecho();

}

//disables screen and raw mode
void disable_raw() {

    echo();
    noraw();
    clear();
    endwin();

}

//prints cursor (and file name) location at top of screen
void print_cursor() {

    move(0,0);
    clrtoeol();
    mvaddstr(0,0,E.filename);
    move(0, 75);
    printw("row: %d col: %d line_len: %d numrows: %d",E.y+(E.y_offset*PAGE_SIZE_Y) , E.x, E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1].render_row_size, E.numrows);
    move(E.y, E.x);
    refresh();

}

void init_gbuf(gbuf* buf) {

    if(buf->buffer = (char*)malloc(sizeof(char)*BUF_SIZE) ) {
        buf->size = buf->gap_size = BUF_SIZE;
        buf->gap_left = 0;
        buf->buffer[0] = '0';
        buf->render_row = NULL;
        buf->render_row_size = 0;
    }
    return;

}

void destroy_gbuf(gbuf* buf) {

    free(buf->buffer);
    buf->gap_left = 0;
    buf->gap_size = 0;
    buf->render_row_size = 0;
    return;
}

/*
void grow_gbuf(gbuf* buf, size_t len) {

        
        size_t gap_right = buf->size - buf->gap_left;
        
        if(buf->buffer = (char*)realloc(buf->buffer, buf->size + len)) {
            buf->gap_size += len;
            buf->size += len;
            memmove(buf->buffer + buf->gap_left + buf->gap_size, buf->buffer + buf->gap_left, gap_right);
        }

    return;
}
*/

void grow_gbuf(gbuf* buf) {

        
        size_t gap_right_len = buf->size - buf->gap_size - buf->gap_left;
        
        if(buf->buffer = (char*)realloc(buf->buffer, buf->size*2)) {
            buf->gap_size += buf->size;
            buf->size *= 2;
            memmove(buf->buffer + buf->gap_left + buf->gap_size, buf->buffer + buf->gap_left, gap_right_len);
        }

    return;
}

//for initially opening a file
void insert_gbuf(gbuf* buf, char* c, size_t length) {
        
    size_t len = length;
    while(buf->gap_size < len) {
        //grow_gbuf(buf, len - buf->gap_size);
        grow_gbuf(buf);

    }

    memmove(buf->buffer + buf->gap_left, c, len);
    buf->gap_left += len;
    buf->gap_size -= len;

    return;
}

void left_gbuf(gbuf* buf) {

    if(buf->gap_left > 0) {
        buf->buffer[buf->gap_left + buf->gap_size -1] = buf->buffer[buf->gap_left -1];
        buf->buffer[buf->gap_left -1] = 0;
        buf->gap_left--;
    }
    
    return;

}

void right_gbuf(gbuf* buf) {

    size_t gap_right_len = buf->size - buf->gap_size - buf->gap_left;
    if(gap_right_len > 0) {
        buf->buffer[buf->gap_left] = buf->buffer[buf->gap_left + buf->gap_size];
        //buf->buffer[buf->gap_left + buf->gap_size] = 0;
        buf->gap_left++;
    }
}

//basically the insert function for char
void insert_char_gbuf(gbuf* buf, char c) {

    while(E.x != buf->gap_left) {

        if(E.x < buf->gap_left)
            left_gbuf(buf);
        else
            right_gbuf(buf);
    }

    while(buf->gap_size < 1) {
        //buf->gap_size = 0;
        //grow_gbuf(buf, 1);
        grow_gbuf(buf);

    }

    //memmove(buf->buffer + buf->gap_left, &c, 1);
    buf->buffer[buf->gap_left] = c;
    buf->gap_left++;
    buf->gap_size--;

    return;

    return;

}


//please call move(row, 0) and clrtoeol before this function
//and call refresh() after this function
void show_gbuf(editor E, int row) {

    int index = row-1;    
    int gap_right = E.text[index].gap_left + E.text[index].gap_size;
    size_t gap_right_len = E.text[index].size - gap_right;
    
    E.text[index].render_row_size = E.text[index].gap_left + gap_right_len;

    E.text[index].render_row = (char*)malloc(E.text[index].render_row_size);
    
    int i = 0;
    for(i; i < E.text[index].gap_left; i++)
        E.text[index].render_row[i] = E.text[index].buffer[i];

    for(int j = gap_right; j < E.text[index].size; i++,j++)
        E.text[index].render_row[i] = E.text[index].buffer[j];

 
    //print like this becuase there is no \0 or \n or \r at the end
    for(int i = 0; i < E.text[index].render_row_size; i++ )
        printw("%c", E.text[index].render_row[i]);
    free(E.text[index].render_row);
    move(row, E.x);

}

void print_page(editor E) {

    int y = E.y, x = E.x;
    int start = 1 + E.y_offset*PAGE_SIZE_Y;
    int end = start + PAGE_SIZE_Y;
    if(end > E.numrows) {
        end = E.numrows;
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

//expand page
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

void merge_buffer_backspace(editor E, int row) {

    int index = row-1;
    int y = index; 
    int x = E.text[index-1].render_row_size+1;

    int gap_right = E.text[index].gap_left + E.text[index].gap_size;
    size_t gap_right_len = E.text[index].size - gap_right;
    
    E.text[index].render_row_size = E.text[index].gap_left + gap_right_len;

    E.text[index].render_row = (char*)malloc(E.text[index].render_row_size);
    
    int i = 0;
    for(i; i < E.text[index].gap_left; i++)
        E.text[index].render_row[i] = E.text[index].buffer[i];

    for(int j = gap_right; j < E.text[index].size; i++,j++)
        E.text[index].render_row[i] = E.text[index].buffer[j];

    insert_gbuf(&E.text[index-1], E.text[index].render_row, E.text[index].render_row_size);
    
    E.y = y;
    E.x = x;
    move(E.y, E.x);
    return;

}

//shrink page
void shrink_editor(editor* E) {

    if(E->text[E->y-1].render_row_size)
        merge_buffer_backspace(*E, E->y+(E->y_offset*PAGE_SIZE_Y));

    destroy_gbuf(&E->text[E->y+(E->y_offset*PAGE_SIZE_Y)-1]);
    if(E->y != E->numrows) {
        //memmove(E->text + E->y, E->text + E->y + 1, sizeof(gbuf)*(E->numrows - E->y));
        for(int i = E->y-1; i < E->numrows-1; i++) {
            E->text[i] = E->text[i+1];
        }
    }
    move(E->numrows, 0);
    clrtoeol();
    E->numrows--;
    move_cursor((char)KEY_UP);
    E->x = E->text[E->y-1].render_row_size+1;
    move(E->y, E->x);
    print_page(*E);
}

void backspace_gbuf(gbuf* buf) {


    if(E.x != 0) {

        while(E.x != buf->gap_left) {

            if(E.x < buf->gap_left)
                left_gbuf(buf);
            else
                right_gbuf(buf);
        }
        //grow_gbuf(&E.text[E.y-1], -1);
        //buf->buffer[buf->gap_left] = 0;
        buf->gap_left--;
        buf->gap_size++;
    }
    else if(E.y != 1) 
        shrink_editor(&E);

    return;
}

char* split_buffer_enter(editor E, int row) {

    int index = row-1;
    int gap_right = E.text[index].gap_left + E.text[index].gap_size;
    size_t gap_right_len = E.text[index].size - gap_right;
    
    E.text[index].render_row_size = E.text[index].gap_left + gap_right_len;

    E.text[index].render_row = (char*)malloc(E.text[index].render_row_size);
    
    int i = 0;
    for(i; i < E.text[index].gap_left; i++)
        E.text[index].render_row[i] = E.text[index].buffer[i];

    for(int j = gap_right; j < E.text[index].size; i++,j++)
        E.text[index].render_row[i] = E.text[index].buffer[j];

    int split_buffer_size = E.text[index].render_row_size - E.x;
    char* split_buffer = (char*)malloc(split_buffer_size);

    for(int j = E.x, k = 0; j < E.text[index].render_row_size; j++, k++) {
        split_buffer[k] = E.text[index].render_row[j];
    }
    free(E.text[index].render_row);
    return split_buffer;
}

//expand page
void expand_editor_enter(editor* E) {

    char* temp = NULL;
    if(E->x != E->text[E->y-1].render_row_size)
        temp = split_buffer_enter(*E, E->y+(E->y_offset*PAGE_SIZE_Y));

    E->text = (gbuf*)realloc(E->text, sizeof(gbuf)*(E->numrows + 1) );
    init_gbuf(&E->text[E->numrows]);

    if(E->y-1 != E->numrows) {
        //memmove(E->text + E->numrows, E->text + E->y-1, sizeof(gbuf)*(E->numrows - E->y+1));
        for(int i = E->numrows; i > E->y; i--) {
            E->text[i] = E->text[i-1];    
        }
        init_gbuf(&E->text[E->y+(E->y_offset*PAGE_SIZE_Y)]);
    }

    if(temp != NULL) {
            insert_gbuf(&E->text[E->y], temp, E->text[E->y-1].render_row_size - E->x);

        //delete the split part form the prev buffer
        int y = E->y, x = E->x;
        E->x = E->text[E->y+(E->y_offset*PAGE_SIZE_Y)-1].render_row_size;
        move(E->y, E->x);
        int len = strlen(temp);
        for(int j = 0; j < len; j++) {
            backspace_gbuf(&E->text[E->y+(E->y_offset*PAGE_SIZE_Y)-1]);
            move_cursor((char)KEY_LEFT);
        }
        E->y = y;
        E->x = x;
        move(E->y, E->x);
        free(temp);
    }

    //update the 
    E->numrows++;
    print_page(*E);

    return;
}

void write_to_file_gbuf(gbuf buf, FILE* filename) {

    //buf.buffer[buf.size-1] = '\n';
    fwrite(buf.buffer, 1, buf.gap_left, filename);
    
    int gap_right = buf.gap_left + buf.gap_size;
    size_t gap_right_len = buf.size - buf.gap_size - buf.gap_left;
    fwrite(buf.buffer + gap_right, 1, gap_right_len, filename);
    
    return;
    
}

//openeditor
void openeditor(char* filename) {

    init_raw();
    init_editor(&E);
    E.filename = filename;
    mvaddstr(0,0,filename);
    refresh();

    FILE *file = fopen(filename, "r");
    if(file != NULL) {
        
        char *line = NULL;
        size_t linecap = 0;
        ssize_t linelen;
        while((linelen = getline(&line,&linecap,file)) != -1) {
            E.y++;
            expand_editor(&E);
            if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r' || line[linelen-1] == '\0'))
                linelen--;

            insert_gbuf(&E.text[E.y-1],line,linelen);
        }
        fclose(file);
        print_page(E);

    }
    else expand_editor(&E);

    E.unsaved = 0;
    E.read = 1;
    E.y = 1;
    E.x = 0;
    move(1,0);
    refresh();

    return;
    
}

void savefile(editor* E, char* filename) {

    int y = E->y, x = E->x;
    FILE* file = fopen(filename, "w");

    for(int i = 0; i < E->numrows-1; i++) {
        if(E->text[i].render_row_size != 0)
            write_to_file_gbuf(E->text[i], file);
        fputc('\n', file);
    }
    //put \0 for last line
    write_to_file_gbuf(E->text[E->numrows-1], file);
    //fputc('\0', file);

    fclose(file);
    openeditor(E->filename);
    move(y, x);
    E->y = y;
    E->x = x;

    return;
}


//moves cursor
void move_cursor(char c) {

    switch(c) {

        case (char)KEY_UP:
            if(E.y != 1 && !E.y_offset) {
                E.x = 0;
                move(--E.y, 0);
            }
            else if(E.y_offset) {
                if(E.y = 1) {
                    E.y_offset--;
                    E.y = 40;
                    move(E.y, 0);
                    print_page(E);
                }
                else {
                    E.x = 0;
                    move(--E.y, 0);
                }
            }
            break;
        
        case (char)KEY_DOWN:
                if(E.y % PAGE_SIZE_Y == 0) {
                    E.y_offset++;
                    E.y = 1;
                    move(1,0);
                    print_page(E);
                }
                else if(E.y + E.y_offset*PAGE_SIZE_Y < E.numrows)
                    move(++E.y, 0);
            break;
        
        case (char)KEY_LEFT:
            
            if(E.x != 0) {
                move(E.y, --E.x);
            }
            else if(E.y != 1) {
                E.y--;
                E.x = E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1].render_row_size;
                move(E.y, E.x);
            }
            break;
        
        case (char)KEY_RIGHT:
            if(E.x < E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1].render_row_size) {
                move(E.y, ++E.x);
            }
            else if(E.y < E.numrows) {
                move(++E.y, 0);
                E.x = 0;
            }
            break;

        case (char)KEY_HOME:
            move(E.y, 0);
            E.x = 0;
            break;

        case (char)KEY_END:
            move(E.y, E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1].render_row_size);
            E.x = E.text[E.y+(E.y_offset*PAGE_SIZE_Y)-1].render_row_size;
            break;

        case (char)KEY_PPAGE:
            move(1, 0);
            E.y = 1;
            E.x = 0;
            break;

        case (char)KEY_NPAGE:
            move(E.numrows, 0);
            E.y = E.numrows;
            E.x = 0;
            break;

        case (char)(10):
            if(E.x == 0)
                expand_editor(&E);
            else
                expand_editor_enter(&E);
            move_cursor((char)KEY_DOWN);
            break;

        case (char)KEY_BACKSPACE:
            backspace_gbuf(&E.text[E.y-1]);
            move(E.y, 0);
            clrtoeol();
            show_gbuf(E, E.y);
            refresh();
            move_cursor((char)KEY_LEFT);
            refresh();
            break;
    }
    refresh();
    return;

}

//handles and processes keypresses
char read_key() {

    int c = getch();
    
    switch(c) {
        
        case CTRL('q'):
            break;
        case CTRL('s'):
            savefile(&E, E.filename);
            break;
        case (int)KEY_RESIZE:
            //need to handle resising here so it doesnt
            //add in the buffer
            break;
        case (int)KEY_UP:
        case (int)KEY_DOWN:
        case (int)KEY_LEFT:
        case (int)KEY_RIGHT:
        case (int)KEY_HOME:
        case (int)KEY_END:
        case (int)KEY_PPAGE:
        case (int)KEY_NPAGE:
        case (int)KEY_BACKSPACE:
        case 10:
            move_cursor((char)c);
        break;
        default:
            insert_char_gbuf(&E.text[E.y-1], c);
            move(E.y, 0);
            clrtoeol();
            show_gbuf(E, E.y);
            refresh();
            move_cursor((char)KEY_RIGHT);
            refresh();        
    }

    return c;
}

void closeeditor() {

    for(int i = 0; i < E.numrows; i++)
        destroy_gbuf(&E.text[i]);

    disable_raw();
    return;
}

//main
int main(int argc, char* argv[]) {

    char* filename = NULL;
    
    if(argc > 1) {
        filename = argv[1];
    }
    else 
        filename = "new_file.txt";   
    
    openeditor(filename);

    print_cursor();
    while(read_key() != CTRL('q')) {
        print_cursor();
    }

    closeeditor();

    return 0;
}
