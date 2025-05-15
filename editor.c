#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_LINES 100
#define MAX_LINE_LENGTH 256
#define MAX_UNDO 100
#define CLIPBOARD_SIZE 10000

typedef struct {
    char lines[MAX_LINES][MAX_LINE_LENGTH];
    int line_count;
    int cursor_line;
    int cursor_col;
} EditorState;

typedef struct {
    char lines[MAX_LINES][MAX_LINE_LENGTH];
    int line_count;
    int current_line;
    int current_col;
    EditorState undo_stack[MAX_UNDO];
    EditorState redo_stack[MAX_UNDO];
    int undo_count;
    int redo_count;
    char clipboard[CLIPBOARD_SIZE];
    int selection_start_line;
    int selection_start_col;
    int selection_end_line;
    int selection_end_col;
    int selection_mode;
} Editor;

#define COLOR_RESET "\033[0m"
#define COLOR_KEYWORD "\033[1;34m"
#define COLOR_STRING "\033[0;32m" 
#define COLOR_NUMBER "\033[0;33m" 
#define COLOR_COMMENT "\033[0;36m"

const char *keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "int", "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
    NULL
};

int getch()
{
    struct termios oldt, newt;
    int ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}   

int read_key()
{
    int ch = getch();
    if (ch == 27)
    {
        if (getch() == '[')
        {
            switch (getch())
            {
            case 'A':
                return 1000;
            case 'B':
                return 1001;
            case 'C':
                return 1002;
            case 'D':
                return 1003;
            }
        }
        return 27;
    }
    return ch;
}

void clear_screen()
{
    printf("\033[H\033[J");
}

void move_cursor(int row, int col)
{
    printf("\033[%d;%dH", row, col);
}

int is_keyword(const char *word) {
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(word, keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}
void print_syntax_highlighted(const char *line, int is_selected) {
    if (is_selected) {
        printf("\033[7m");
    }   
    char word[MAX_LINE_LENGTH];
    int word_len = 0;
    int in_string = 0;
    int in_comment = 0;
    for (int i = 0; line[i] != '\0'; i++) {
        if (in_comment) {
            if (is_selected) printf("%s", COLOR_RESET);
            printf("%s%c", COLOR_COMMENT, line[i]);
            if (is_selected) printf("\033[7m");
            continue;
        }
        if (line[i] == '"' && !in_comment) {
            if (is_selected) printf("%s", COLOR_RESET);
            printf("%s%c", in_string ? COLOR_RESET : COLOR_STRING, line[i]);
            if (is_selected) printf("\033[7m");
            in_string = !in_string;
            continue;
        }
        if (in_string) {
            if (is_selected) printf("%s", COLOR_RESET);
            printf("%s%c", COLOR_STRING, line[i]);
            if (is_selected) printf("\033[7m");
            continue;
        }
        if (line[i] == '/' && line[i + 1] == '/') {
            if (is_selected) printf("%s", COLOR_RESET);
            printf("%s//", COLOR_COMMENT);
            if (is_selected) printf("\033[7m");
            in_comment = 1;
            i++;
            continue;
        }
        if ((line[i] >= 'a' && line[i] <= 'z') || 
            (line[i] >= 'A' && line[i] <= 'Z') || 
            (line[i] == '_')) {
            word[word_len++] = line[i];
        } else {
            word[word_len] = '\0';
            if (word_len > 0) {
                if (is_keyword(word)) {
                    if (is_selected) printf("%s", COLOR_RESET);
                    printf("%s%s%s", COLOR_KEYWORD, word, COLOR_RESET);
                    if (is_selected) printf("\033[7m");
                } else {
                    printf("%s", word);
                }
            }
            printf("%c", line[i]);
            word_len = 0;
        }
    }
    if (word_len > 0) {
        word[word_len] = '\0';
        if (is_keyword(word)) {
            if (is_selected) printf("%s", COLOR_RESET);
            printf("%s%s%s", COLOR_KEYWORD, word, COLOR_RESET);
            if (is_selected) printf("\033[7m");
        } else {
            printf("%s", word);
        }
    }
    if (is_selected) {
        printf("%s", COLOR_RESET);
    }
}
void save_state(Editor *editor) {
    if (editor->undo_count >= MAX_UNDO) {
        memmove(editor->undo_stack, editor->undo_stack + 1, 
                (MAX_UNDO - 1) * sizeof(EditorState));
        editor->undo_count--;
    }   
    EditorState *state = &editor->undo_stack[editor->undo_count];
    memcpy(state->lines, editor->lines, sizeof(editor->lines));
    state->line_count = editor->line_count;
    state->cursor_line = editor->current_line;
    state->cursor_col = editor->current_col;
    editor->undo_count++;
    editor->redo_count = 0;
}
void undo(Editor *editor) {
    if (editor->undo_count <= 0) return;   
    if (editor->redo_count < MAX_UNDO) {
        EditorState *redo_state = &editor->redo_stack[editor->redo_count++];
        memcpy(redo_state->lines, editor->lines, sizeof(editor->lines));
        redo_state->line_count = editor->line_count;
        redo_state->cursor_line = editor->current_line;
        redo_state->cursor_col = editor->current_col;
    }
    editor->undo_count--;
    EditorState *state = &editor->undo_stack[editor->undo_count];
    memcpy(editor->lines, state->lines, sizeof(editor->lines));
    editor->line_count = state->line_count;
    editor->current_line = state->cursor_line;
    editor->current_col = state->cursor_col;
}
void redo(Editor *editor) {
    if (editor->redo_count <= 0) return;   
    if (editor->undo_count < MAX_UNDO) {
        EditorState *undo_state = &editor->undo_stack[editor->undo_count++];
        memcpy(undo_state->lines, editor->lines, sizeof(editor->lines));
        undo_state->line_count = editor->line_count;
        undo_state->cursor_line = editor->current_line;
        undo_state->cursor_col = editor->current_col;
    }
    editor->redo_count--;
    EditorState *state = &editor->redo_stack[editor->redo_count];
    memcpy(editor->lines, state->lines, sizeof(editor->lines));
    editor->line_count = state->line_count;
    editor->current_line = state->cursor_line;
    editor->current_col = state->cursor_col;
}
void copy_selection(Editor *editor) {
    if (!editor->selection_mode) return;   
    int start_line = editor->selection_start_line;
    int start_col = editor->selection_start_col;
    int end_line = editor->selection_end_line;
    int end_col = editor->selection_end_col;
    if (start_line > end_line || (start_line == end_line && start_col > end_col)) {
        int temp = start_line;
        start_line = end_line;
        end_line = temp;
        temp = start_col;
        start_col = end_col;
        end_col = temp;
    }
    int clipboard_pos = 0;
    for (int i = start_line; i <= end_line && clipboard_pos < CLIPBOARD_SIZE - 1; i++) {
        int col_start = (i == start_line) ? start_col : 0;
        int col_end = (i == end_line) ? end_col : strlen(editor->lines[i]);
        for (int j = col_start; j < col_end && clipboard_pos < CLIPBOARD_SIZE - 1; j++) {
            editor->clipboard[clipboard_pos++] = editor->lines[i][j];
        }
        if (i < end_line && clipboard_pos < CLIPBOARD_SIZE - 1) {
            editor->clipboard[clipboard_pos++] = '\n';
        }
    }
    editor->clipboard[clipboard_pos] = '\0';
}
void refresh_screen(Editor *editor) {
    clear_screen();   
    printf("Editor - ESC(2 times) to save, Ctrl+U for undo, Ctrl+R for redo\n");
    printf("Ctrl+X to copy, Ctrl+V to paste, Ctrl+B to start/end selection\n\n");
    if (editor->line_count == 0) {
        editor->line_count = 1;
        editor->lines[0][0] = '\0';
    }
    for (int i = 0; i < editor->line_count; i++) {
        int is_selected = 0;
        if (editor->selection_mode) {
            int start_line = editor->selection_start_line;
            int end_line = editor->selection_end_line;
            if (start_line > end_line) {
                int temp = start_line;
                start_line = end_line;
                end_line = temp;
            }
            if (i >= start_line && i <= end_line) {
                is_selected = 1;
            }
        }   
        print_syntax_highlighted(editor->lines[i], is_selected);
        if (i < editor->line_count - 1 && 
            (strlen(editor->lines[i]) == 0 || 
             editor->lines[i][strlen(editor->lines[i]) - 1] != '\n')) {
            printf("\n");
        }
    }
    move_cursor(editor->current_line + 4, editor->current_col + 1);
    fflush(stdout);
}
void paste_text(Editor *editor) {
    if (!editor->clipboard[0]) return;   
    save_state(editor);
    editor->selection_mode = 0;
    char temp_buffer[CLIPBOARD_SIZE];
    strcpy(temp_buffer, editor->clipboard);
    char *line = strtok(temp_buffer, "\n");
    int first_line = 1;
    while (line != NULL) {
        if (!first_line) {
            if (editor->line_count < MAX_LINES - 1) {
                for (int i = editor->line_count; i > editor->current_line + 1; i--) {
                    strcpy(editor->lines[i], editor->lines[i-1]);
                }
                editor->line_count++;
                editor->current_line++;
                editor->current_col = 0;
            }
        }
        int available_space = MAX_LINE_LENGTH - 1 - editor->current_col;
        int line_len = strlen(line);
        if (line_len > available_space) {
            line_len = available_space;
        }
        if (!first_line || editor->current_col < strlen(editor->lines[editor->current_line])) {
            memmove(&editor->lines[editor->current_line][editor->current_col + line_len],
                   &editor->lines[editor->current_line][editor->current_col],
                   strlen(&editor->lines[editor->current_line][editor->current_col]) + 1);
        }
        strncpy(&editor->lines[editor->current_line][editor->current_col], line, line_len);
        editor->current_col += line_len;
        first_line = 0;
        line = strtok(NULL, "\n");
    }    
    editor->lines[editor->current_line][MAX_LINE_LENGTH - 1] = '\0';
    clear_screen();
    refresh_screen(editor);
}
void handle_delete_key(Editor *editor) {
    save_state(editor);   
    int current_line_length = strlen(editor->lines[editor->current_line]);
    if (editor->current_col < current_line_length) {
        memmove(editor->lines[editor->current_line] + editor->current_col,
                editor->lines[editor->current_line] + editor->current_col + 1,
                current_line_length - editor->current_col);
    } 
    else if (editor->current_line < editor->line_count - 1) {
        current_line_length = strlen(editor->lines[editor->current_line]);
        if (current_line_length > 0 && 
            editor->lines[editor->current_line][current_line_length - 1] == '\n') {
            editor->lines[editor->current_line][current_line_length - 1] = '\0';
            current_line_length--;
        }
        strcat(editor->lines[editor->current_line], editor->lines[editor->current_line + 1]);        
        for (int i = editor->current_line + 1; i < editor->line_count - 1; i++) {
            strcpy(editor->lines[i], editor->lines[i + 1]);
        }
        editor->line_count--;
    }
}
void editor(const char *filename) {
    Editor editor = {0};
    editor.lines[0][0] = '\0';
    editor.line_count = 1;

    FILE *file = fopen(filename, "r");
    if (file) {
        while (fgets(editor.lines[editor.line_count], MAX_LINE_LENGTH, file) && 
               editor.line_count < MAX_LINES) {
            editor.line_count++;
        }
        fclose(file);
    }
    printf("Editor - ESC(2 times) to save, Ctrl+U for undo, Ctrl+R for redo\n");
    printf("Ctrl+X to copy, Ctrl+V to paste, Ctrl+B to start/end selection\n");
    printf("Press Enter to start editing...\n");
    getchar();
    while (1) {
        clear_screen();
        printf("Editor - ESC(2 times) to save, Ctrl+U for undo, Ctrl+R for redo\n");
        printf("Ctrl+X to copy, Ctrl+V to paste, Ctrl+B to start/end selection\n");
        refresh_screen(&editor);
        int ch = read_key();
        if (ch == 27) {
            file = fopen(filename, "w");
            if (file) {
                for (int i = 0; i < editor.line_count; i++) {
                    fputs(editor.lines[i], file);
                    if (i < editor.line_count - 1 && 
                        editor.lines[i][strlen(editor.lines[i]) - 1] != '\n') {
                        fputc('\n', file);
                    }
                }
                fclose(file);
                printf("\nFile saved successfully. Press Enter to continue...\n");
                getchar();
            }
            return;
        } else if (ch == 21) {
            undo(&editor);
        } else if (ch == 18) {
            redo(&editor);
        } else if (ch == 24) {
            copy_selection(&editor);
            editor.selection_mode = 0;
        } else if (ch == 22) {
            paste_text(&editor);
        } else if (ch == 2) {
            if (!editor.selection_mode) {
                editor.selection_mode = 1;
                editor.selection_start_line = editor.current_line;
                editor.selection_start_col = editor.current_col;
                editor.selection_end_line = editor.current_line;
                editor.selection_end_col = editor.current_col;
            } else {
                editor.selection_mode = 0;
            }
        } else if (ch == 10) {
            save_state(&editor);
            if (editor.line_count < MAX_LINES - 1) {
                memmove(editor.lines[editor.current_line + 1], 
                        editor.lines[editor.current_line] + editor.current_col,
                        strlen(editor.lines[editor.current_line]) - editor.current_col + 1);
                editor.lines[editor.current_line][editor.current_col] = '\n';
                editor.lines[editor.current_line][editor.current_col + 1] = '\0';
                editor.current_line++;
                editor.current_col = 0;
                editor.line_count++;
            }
        } else if (ch == 127 || ch ==8) {
            save_state(&editor);
            if (editor.current_col > 0) {
                memmove(editor.lines[editor.current_line] + editor.current_col - 1,
                        editor.lines[editor.current_line] + editor.current_col,
                        strlen(editor.lines[editor.current_line]) - editor.current_col + 1);
                editor.current_col--;
            } else if (editor.current_line > 0) {
                int prev_length = strlen(editor.lines[editor.current_line - 1]);
                if (prev_length > 0 && editor.lines[editor.current_line - 1][prev_length - 1] == '\n') {
                    editor.lines[editor.current_line - 1][prev_length - 1] = '\0';
                    prev_length--;
                }
                if (prev_length + strlen(editor.lines[editor.current_line]) < MAX_LINE_LENGTH) {
                    strcat(editor.lines[editor.current_line - 1], 
                           editor.lines[editor.current_line]);
                    memmove(editor.lines + editor.current_line,
                            editor.lines + editor.current_line + 1,
                            (editor.line_count - editor.current_line) * MAX_LINE_LENGTH);
                    editor.line_count--;
                    editor.current_line--;
                    editor.current_col = prev_length;
                }
            }
        }else if (ch == 126 || ch == 4){
            handle_delete_key(&editor);
        } else if (ch == 1000 && editor.current_line > 0) {
            editor.current_line--;
            int line_length = strlen(editor.lines[editor.current_line]);
            if (line_length > 0 && editor.lines[editor.current_line][line_length - 1] == '\n') {
                line_length--;
            }
            editor.current_col = (editor.current_col > line_length) ? line_length : editor.current_col;
        } else if (ch == 1001 && editor.current_line < editor.line_count - 1) {
            editor.current_line++;
            int line_length = strlen(editor.lines[editor.current_line]);
            if (line_length > 0 && editor.lines[editor.current_line][line_length - 1] == '\n') {
                line_length--;
            }
            editor.current_col = (editor.current_col > line_length) ? line_length : editor.current_col;
        } else if (ch == 1002) {
            int line_length = strlen(editor.lines[editor.current_line]);
            if (line_length > 0 && editor.lines[editor.current_line][line_length - 1] == '\n') {
                line_length--;
            }
            if (editor.current_col < line_length) {
                editor.current_col++;
            } else if (editor.current_line < editor.line_count - 1) {
                editor.current_line++;
                editor.current_col = 0;
            }
        } else if (ch == 1003) {
            if (editor.current_col > 0) {
                editor.current_col--;
            } else if (editor.current_line > 0) {
                editor.current_line--;
                int line_length = strlen(editor.lines[editor.current_line]);
                if (line_length > 0 && editor.lines[editor.current_line][line_length - 1] == '\n') {
                    line_length--;
                }
                editor.current_col = line_length;
            }
        } else if (ch >= 32 && ch <= 126) {
            save_state(&editor);
            if (strlen(editor.lines[editor.current_line]) < MAX_LINE_LENGTH - 1) {
                memmove(editor.lines[editor.current_line] + editor.current_col + 1,
                        editor.lines[editor.current_line] + editor.current_col,
                        strlen(editor.lines[editor.current_line]) - editor.current_col + 1);
                editor.lines[editor.current_line][editor.current_col] = ch;
                editor.current_col++;
            }
        }
        
        if (editor.selection_mode) {
            editor.selection_end_line = editor.current_line;
            editor.selection_end_col = editor.current_col;
        }
    }
}