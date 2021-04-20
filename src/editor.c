#include "editor.h"

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
    getmaxyx(stdscr, E->y_max, E->x_max);
    E->y_max -= 2;
    E->x_max -= 10;

    return;
}

//renders the buffer
//please call move(E.y, 0) and clrtoeol before this function
//and call refresh() after this function
void show_gbuf(editor E, size_t row) {

    size_t index = row-1;    
    size_t gap_right = E.text[index].gap_left + E.text[index].gap_size; //-1
    size_t gap_right_len = E.text[index].size - gap_right; //+1
    
    E.text[index].render_row_size = E.text[index].gap_left + gap_right_len;

    char* render_row = (char*)malloc(E.text[index].render_row_size);
    
    size_t i;
    for(i = 0; i < E.text[index].gap_left; i++) {
        render_row[i] = E.text[index].buffer[i];
    }
    for(size_t j = gap_right; j < E.text[index].size; i++,j++) {
        render_row[i] = E.text[index].buffer[j];
    }

 
    //print like this becuase there mvaddstr/mvprintw causes garbage values
    size_t start = 0 + E.x_offset;
    //renders text of length E.x_max + 10
    //just for better scrolling experience    
    size_t end = start + E.x_max+10;
    
    if(end > E.text[index].render_row_size)
        end = E.text[index].render_row_size;

    for(size_t i = 0 + E.x_offset; i < end; i++ )
        printw("%c", render_row[i]);

    free(render_row);
    move(E.y, E.x);

}

//renders the entire page
void print_page(editor E) {

    size_t y = E.y, x = E.x;
    size_t start = 1 + E.y_offset*E.y_max;
    size_t end = start + E.y_max;
    if(end > E.numrows) {
        end = E.numrows+1;
        for(size_t i = 1; i <= E.y_max; i++) {
            move(i, 0);
            clrtoeol();
        }
    }
    for(size_t i = start, j = 1; i < end; i++, j++) {
        move(j, 0);
        clrtoeol();
        show_gbuf(E, i);
    }
    E.y = y;
    E.x = x;
    move(E.y, E.x);
    refresh();
}

//inserts tab (blank_spaces) to next multiple of TAB_SIZE
void insert_tabs(editor* E, size_t row) {

    size_t index = row-1;
    
    //calculation to help move to nearest multiple of TAB_SIZE
    size_t tab_minus =  (E->x + E->x_offset)%TAB_SIZE;
    
    //enter tabs
    for(size_t i = 0; i < TAB_SIZE-tab_minus; i++) {
        insert_char_gbuf(&E->text[index], (char)32, E->x + E->x_offset);

        //adjust x_offset
        if(E->x_offset || E->x == E->x_max)
            E->x_offset++;
        else
            E->x++;
    }

    //if x_offset
    if(E->x_offset) {
        print_page(*E);
        refresh();
    }
    //if no offset
    else if(!E->x_offset) {
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

//for back space at beginning of line
void merge_buffer_backspace(editor* E, size_t row) {

    size_t index = row-1;

    size_t gap_right = E->text[index].gap_left + E->text[index].gap_size;
    size_t gap_right_len = E->text[index].size - gap_right;
    
    E->text[index].render_row_size = E->text[index].gap_left + gap_right_len;

    char* render_row = (char*)malloc(E->text[index].render_row_size);
    
    size_t i;
    for(i = 0; i < E->text[index].gap_left; i++)
        render_row[i] = E->text[index].buffer[i];

    for(size_t j = gap_right; j < E->text[index].size; i++,j++)
        render_row[i] = E->text[index].buffer[j];

    size_t x = E->x, x_offset = E->x_offset, y = E->y, y_offset = E->y_offset; 
    
    E->x = E->text[index-1].render_row_size;
    E->y = E->y-1;
    insert_gbuf(&E->text[index-1], render_row, E->text[index].render_row_size, E->x);

    E->x = x;
    E->x_offset = x_offset;
    E->y = y; 
    E->y_offset = y_offset;

    free(render_row);
    
    return;

}

//shrink page
void shrink_editor(editor* E) {

    size_t row = E->y+(E->y_offset*E->y_max);
    size_t x = E->text[row-2].render_row_size+1;    
    size_t x_offset = 0;
    if(x > E->x_max) {
        x_offset = x - E->x_max;
        x = E->x_max;
    }
    if(E->text[row-1].render_row_size)
        merge_buffer_backspace(E, row);

    destroy_gbuf(&E->text[row-1]);
    if(row != E->numrows) {
        for(size_t i = row-1; i < E->numrows-1; i++) {
            E->text[i] = E->text[i+1];
        }
    }
    E->numrows--;    
    move_cursor(E, (char)KEY_UP);
    E->x = x;
    E->x_offset = x_offset;
    move(E->y, E->x);
    print_page(*E);
}

//delete character from the buffer
void backspace_gbuf(editor* E, gbuf* buf) {

    size_t pos = E->x + E->x_offset;
    if(pos != 0) {

        while(pos != buf->gap_left) {

            if(pos < buf->gap_left)
                left_gbuf(buf);
            else
                right_gbuf(buf);
        }

        buf->gap_left--;
        buf->gap_size++;
    }
    else if(E->x == 0 && E->y == 1 && !E->y_offset) {
    }
    else  
        shrink_editor(E);

    return;
}

//clears the current line CTRL+D
void clear_line(editor* E, size_t row) {

    size_t index = row-1;
    destroy_gbuf(&E->text[index]);
    init_gbuf(&E->text[index]);
    move_cursor(E, (char)KEY_HOME);
    print_page(*E);
    E->unsaved = 1;
    return;

}

//expand page by appending a buffer
void expand_editor(editor* E) {

    size_t row = E->y+(E->y_offset*E->y_max);
    E->text = (gbuf*)realloc(E->text, sizeof(gbuf)*(E->numrows + 1) );
    init_gbuf(&E->text[E->numrows]);
    if(row-1 != E->numrows) {

        for(size_t i = E->numrows; i >= (row); i--) {
            E->text[i] = E->text[i-1];    
        }
        init_gbuf(&E->text[row-1]);
    }
    E->numrows++;
    print_page(*E);

    return;
}

//helps split the buffer if enter was pressed
char* split_buffer_enter(editor E, size_t row) {

    //first render the entire row
    size_t index = row-1;
    size_t gap_right = E.text[index].gap_left + E.text[index].gap_size;
    size_t gap_right_len = E.text[index].size - gap_right;
    
    E.text[index].render_row_size = E.text[index].gap_left + gap_right_len;
    char* render_row = (char*)malloc(E.text[index].render_row_size);
    
    size_t i;
    for(i = 0; i < E.text[index].gap_left; i++)
        render_row[i] = E.text[index].buffer[i];

    for(size_t j = gap_right; j < E.text[index].size; i++,j++)
        render_row[i] = E.text[index].buffer[j];

    //now extract only the part to split
    size_t split_buffer_size = E.text[index].render_row_size - E.x-E.x_offset;
    char* split_buffer = (char*)malloc(split_buffer_size);

    for(size_t j = E.x+E.x_offset, k = 0; j < E.text[index].render_row_size; j++, k++) {
        split_buffer[k] = render_row[j];
    }
    //free render_row
    free(render_row);
    
    //return the split part
    return split_buffer;
}

//expand page by inserting a buffer
void expand_editor_enter(editor* E) {

    char* temp = NULL;
    size_t row = E->y+(E->y_offset*E->y_max);
    if(E->x + E->x_offset != E->text[row-1].render_row_size)
        temp = split_buffer_enter(*E, row);

    E->text = (gbuf*)realloc(E->text, sizeof(gbuf)*(E->numrows + 1) );
    init_gbuf(&E->text[E->numrows]);

    if(E->y-1 != E->numrows) {
        for(size_t i = E->numrows; i >  row; i--) {
            E->text[i] = E->text[i-1];    
        }
        init_gbuf(&E->text[row]);
    }

    if(temp != NULL) {
        
        size_t len = E->text[row-1].render_row_size - E->x - E->x_offset;
        size_t y = E->y, x = E->x, x_offset = E->x_offset;

        E->x = 0;
        E->x_offset = 0;
        insert_gbuf(&E->text[row], temp, len, E->x);

        //delete the split part form the prev buffer
        E->x = E->text[row-1].render_row_size;
        if(E->x > E->x_max) {
            E->x_offset = E->x - E->x_max;
            E->x = E->x_max;
        }
        for(size_t j = 0; j < len; j++) {
            backspace_gbuf(E, &E->text[row-1]);
            
            //move_cursor(E, (char)KEY_LEFT)
            if(E->x == 0 && E->y == 1 && !E->y_offset) {
            }
            //base case
            else if(E->x != 0 && !E->x_offset) {
                move(E->y, --E->x);
            }
            //if offset
            else if(E->x_offset) {
                E->x_offset--;
            }
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

//re render editor
void resize_check(editor* E) {
    

    size_t y_max = E->y_max, x_max = E->x_max;
    size_t x = E->x + E->x_offset, y = E->y + E->y_offset*E->y_max;

    getmaxyx(stdscr, E->y_max, E->x_max);
    E->y_max -= 2;
    E->x_max -= 10;
    
    
    //if any changes were made only change values not cursor
    if(E->y_max != y_max || E->x_max != x_max) {

        E->x = x;
        if(E->x > E->x_max) {
            E->x_offset = E->x - E->x_max;
            E->x = E->x_max;
        }
        else if(E->x < E->x_max) {
            E->x_offset = 0;
        }

        E->y = y;
        E->y_offset = y/E->y_max;
        if(E->y_offset && y%E->y_max == 0) {
            E->y_offset--;
        }
        E->y = y - E->y_offset*E->y_max;


        print_page(*E);
    }
}

//moves cursor
void move_cursor(editor* E, char c) {


    switch(c) {

        //goes to the starting one line above
        //cant go above
        case (char)KEY_UP:
            
            if(E->y == 1 && !E->y_offset) {
                //do nothing
            }
            //if y offset
            else if(E->y == 1 && E->y_offset) {
                E->x = 0;
                E->x_offset = 0;
                E->y = E->y_max;
                E->y_offset--;
                move(E->y, E->x);
                print_page(*E);
            }
            //if no change in offset
            else {
                E->x = 0;
                //but if x offset
                if(E->x_offset) {
                    E->x_offset = 0;
                    print_page(*E);
                }
                //move up
                move(--E->y, E->x);
            }
            break;
        
        case (char)KEY_DOWN:
                
                //increase y_offset if current line is multiple of E->y_max
                //cant move below numrows
                if(E->y % E->y_max == 0 && (E->y + E->y_offset*E->y_max) < E->numrows) {
                    E->y_offset++;
                    E->y = 1;
                    E->x = 0;
                    E->x_offset = 0;
                    move(1,0);
                    print_page(*E);
                }
                //if no change in y_offset is reqd
                else if(E->y % E->y_max != 0 && (E->y + E->y_offset*E->y_max) < E->numrows) {
                    E->x = 0;
                    //but if e_offset
                    if(E->x_offset) {
                        E->x_offset = 0;
                        print_page(*E);
                    }
                    move(++E->y, 0);
                }
            break;
        
        case (char)KEY_LEFT:
            
            if(E->x == 0 && E->y == 1 && !E->y_offset) {
                break;
            }
            //base case
            else if(E->x != 0 && !E->x_offset) {
                move(E->y, --E->x);
            }
            //if offset
            else if(E->x_offset) {
                E->x_offset--;
                print_page(*E);
            }
            //if we have to go to the prev page
            //move to up prev line and then to the end of the line
            else{
                move_cursor(E, (char)KEY_UP);
                move_cursor(E, (char)KEY_END);
            }               
            break;
        
        case (char)KEY_RIGHT:

            //if at the end of the line move down
            if((E->x + E->x_offset) == E->text[E->y+(E->y_offset*E->y_max)-1].render_row_size)
                move_cursor(E, (char)KEY_DOWN);
            
            //else move right
            else if(E->x+E->x_offset < E->text[E->y+(E->y_offset*E->y_max)-1].render_row_size) {
                
                //if no x offset move right
                if(!E->x_offset && E->x < E->x_max)
                    move(E->y, ++E->x);
                //just increase offset and print
                else {
                    E->x_offset++;
                    print_page(*E);
                }
            }
            break;

        case (char)KEY_HOME:
            //move to the beginning of the line
            E->x = 0;
            if(E->x_offset) {
                E->x_offset = 0;
                print_page(*E);
            }
            move(E->y, 0);
            break;

        case (char)KEY_END:
            //move to the end of the line
            E->x = E->text[E->y+(E->y_offset*E->y_max)-1].render_row_size;
            if(E->x > E->x_max) {
                E->x_offset = E->x - E->x_max;
                E->x = E->x_max;
                print_page(*E);
            }
            move(E->y, E->x);
            break;

        case (char)KEY_PPAGE:

            //move to the prev page
            if(E->y_offset || E->x_offset) {
                E->y_offset--;
                E->x_offset = 0;
                print_page(*E);
            }
            E->y = 1;
            E->x = 0;
            move(E->y, E->x);
            break;

        case (char)KEY_NPAGE:
            
            //if next page end not E->numrows
            if( (E->y_offset+1)*E->y_max < E->numrows) {
                E->y_offset++;
                E->y = 1;
                E->x = 0;
                E->x_offset = 0;
                move(E->y, E->x);
                print_page(*E);
            }
            //if next page numrows
            else {
                //move to end of the last row
                E->y = E->numrows - (E->y_offset)*E->y_max;
                E->x = E->text[E->numrows-1].render_row_size;
                if(E->x > E->x_max) {
                    E->x_offset = E->x - E->x_max;
                    E->x = E->x_max;
                    print_page(*E);
                }
                move(E->y, E->x);
            }
            break;

        //if enter key is pressed
        case (char)(10):
            
            if(E->x == 0) {
                expand_editor(E);
            }
            else {
                expand_editor_enter(E);
            }
            move_cursor(E, (char)KEY_DOWN);
            E->unsaved = 1;
            break;

        case (char)KEY_BACKSPACE:
            
            if(E->y == 1 && !E->y_offset && E->x == 0 ) {
                break;
            }           
            backspace_gbuf(E, &E->text[E->y+(E->y_offset*E->y_max)-1]);
            if(!E->x_offset) {
                move(E->y, 0);
                clrtoeol();
                show_gbuf(*E, E->y+(E->y_offset*E->y_max));
            }
            move_cursor(E, (char)KEY_LEFT);
            E->unsaved = 1;
            refresh();
            break;
    }
    refresh();
    return;

}
