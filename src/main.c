#include "buffer.h"
#include "editor.h"

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

//global variable
editor E;

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
    if(E.x_max > 130) {
        move(0, 75);
        printw("row: %d col: %d line_len: %d numrows: %d",
            E.y+(E.y_offset*E.y_max) , E.x+E.x_offset, E.text[E.y+(E.y_offset*E.y_max)-1].render_row_size, E.numrows);
    }
    else if(E.x_max > 120 ) {
        move(0, 75);
        printw("row: %d col: %d",  E.y+(E.y_offset*E.y_max) , E.x+E.x_offset);
    }

    //print mode information
    move(E.y_max+1, 0);
    clrtoeol();
    if(!E.read)
        addstr("--INSERT--");
    else
        addstr("--READ--");

    if(E.x_max > 100) {
        //calculations for total pages
        int pages = E.numrows/E.y_max;
        if(E.numrows % E.y_max != 0)
            pages++;

        //calculations for current page
        int cpage = (E.y + E.y_offset*E.y_max)/E.y_max;
        if( (E.y + E.y_offset*E.y_max)%E.y_max != 0)
            cpage++;

        move(E.y_max+1,  75);
        printw("Pages %d out of %d", cpage, pages);
    }
    
    attroff(A_STANDOUT);
    
    //move cursor back to original position    
    move(E.y, E.x);
    refresh();

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
            backspace_gbuf(&E, &E.text[E.numrows-1]);
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
    else if(32 <= c && c < 127) {
            
        if(E.x < E.x_max) {
            insert_char_gbuf(&E.text[E.numrows-1], (char)c, E.x + E.x_offset);
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

            insert_gbuf(&E.text[E.y-1],line,linelen, 0);
        }
        free(line);
        fclose(file);

    }
    //if no file given or empty file
    else {
        E.y++;
        expand_editor(&E);
    }

    //filename
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
//resizing doesnt work here
void savefile(editor* E, char* filename) {

    int y = E->y, x = E->x, y_offset = E->y_offset, x_offset = E->x_offset;
    
    if(!E->filename) {
        
        attron(A_STANDOUT);
        E->y = E->y_max+1;
        E->x = 0;
        expand_editor(E);
        mvaddstr(E->y, 0, "Enter Filename to save:");

        int c;
        while( (c = read_key_save()) ) {
            //if enter is pressed then render then buffer and save
            if(c == 10) {
                int index = E->numrows-1;    
                int gap_right = E->text[index].gap_left + E->text[index].gap_size;
                size_t gap_right_len = E->text[index].size - gap_right;
                
                E->text[index].render_row_size = E->text[index].gap_left + gap_right_len;
                filename = (char*)malloc(E->text[index].render_row_size);
                
                int i;
                for(i = 0; i < E->text[index].gap_left; i++)
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
        E.y = E.y_max+1; 
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

//process keypress for read mode
int read_key_read() {

    int c;
    print_info();
    while(c != 105 || c != CTRL('q')) {
        
        resize_check(&E);
        print_info();

        c = getch();
        if(c == 104) //h
            c = KEY_LEFT;
        else if (c == 106) //j
            c = KEY_DOWN;
        else if (c == 107) //k
            c = KEY_UP;
        else if (c == 108) //l
            c = KEY_RIGHT;
            
        if (c == KEY_UP || c == KEY_DOWN|| c == KEY_LEFT || c == KEY_RIGHT || 
            c == KEY_HOME || c == KEY_RIGHT || c == KEY_END || c == KEY_UP ||
            c == KEY_PPAGE || c == KEY_NPAGE) {
            move_cursor(&E, (char)c);
        }
        else if(c == CTRL('q') || c == 105) {
            break;
        }
    }

    E.read = 0;
    return c;

}

//handles and processes keypresses
int read_key() {
 
    resize_check(&E);
    print_info();

    int c = getch();

    //ESC key 
    if(c == 27) {
        E.read = 1;
        c = read_key_read();
    }
    else if( c == KEY_RESIZE) {
        //do nothing
    }
    //TAB key (CTRL + i)
    else if (c == 9) {
        insert_tabs(&E, E.y + E.y_offset*(E.y_max));
    }
    else if(c == CTRL('q')) {
        //essentially break from here
    }
    else if(c == CTRL('d')) {
        clear_line(&E, E.y+(E.y_offset)*E.y_max);
    }
    else if(c == CTRL('s') ) {
        savefile(&E, E.filename);
    }
    else if(c == KEY_UP || c == KEY_DOWN|| c == KEY_LEFT || c == KEY_RIGHT || 
            c == KEY_HOME || c == KEY_RIGHT || c == KEY_END || c == KEY_UP ||
            c == KEY_PPAGE || c == KEY_NPAGE || c == KEY_BACKSPACE || c == 10 
            ) {
                move_cursor(&E, (char)c);
    }
    else if (c < 32 || c > 127) {
        //non printable characters
    }
    else if(32 <= c && c < 127 ) {
        
        //insert
        insert_char_gbuf(&E.text[E.y+(E.y_offset*E.y_max)-1], (char)c, E.x + E.x_offset);
        
        //cursor movement
        if(E.text[E.y+(E.y_offset*E.y_max)-1].render_row_size > E.x_max) {
            //this is just so to manage the cursor movement of KEY_RIGHT
            //the actual value is calculated while print_page is called
            //coincidenlty they will be the same
            E.text[E.y+(E.y_offset*E.y_max)-1].render_row_size++;
            move_cursor(&E, (char)KEY_RIGHT);
            //print again as to take care of case if render_row_size > E.x_max
            //but currently we are inserting before that posn
            if(!E.x_offset)
                print_page(E);
        }
        else {
            move(E.y, 0);
            clrtoeol();
            show_gbuf(E, (&E)->y+((&E)->y_offset*E.y_max));
            refresh();
            move_cursor(&E, (char)KEY_RIGHT);
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
    //printf("%ld %ld", sizeof(gbuf), sizeof(editor));
    return 0;
    
}
