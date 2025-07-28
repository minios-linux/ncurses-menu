#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int n_choices = 0;
char *title = NULL;
char **options = NULL;
char *filename = NULL;
int auto_refresh = 0;
int highlight = 0;
int scrollpos = 0;
int n_o_choices = 0;

// Processes backslash escapes like \n in a string
void process_escapes(char *str) {
    if (!str) return;
    char *p_read = str;
    char *p_write = str;
    while (*p_read) {
        if (*p_read == '\\' && *(p_read + 1) == 'n') {
            *p_write++ = '\n';
            p_read += 2;
        } else {
            *p_write++ = *p_read++;
        }
    }
    *p_write = '\0';
}

// Replaces all tab characters in a string with spaces
void replace_tab_with_space(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\t') {
            str[i] = ' ';
        }
    }
}

// Reads menu options from a file and updates the list
void read_options_from_file() {
    // Store the text of the currently highlighted option
    char *previously_highlighted = NULL;
    if (highlight < n_choices) {
        previously_highlighted = strdup(options[highlight]);
    }

    FILE *file = fopen(filename, "r");
    if (file) {
        char **file_options = NULL;
        int file_n_choices = 0;
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;  // Remove newline
            replace_tab_with_space(line);  // Replace tabs with a space
            file_options = realloc(file_options, (file_n_choices + 1) * sizeof(char *));
            file_options[file_n_choices++] = strdup(line);
        }
        fclose(file);

        // Resize the options array to accommodate -o options and file options
        options = realloc(options, (n_o_choices + file_n_choices) * sizeof(char *));

        // Copy file options to the options array after the -o options
        for (int i = 0; i < file_n_choices; i++) {
            if (i + n_o_choices < n_choices) {
                free(options[i + n_o_choices]);  // Free the old file option before overwriting
            }
            options[i + n_o_choices] = file_options[i];
        }

        // Update n_choices
        n_choices = n_o_choices + file_n_choices;

        // Free the temporary file_options array (but not the strings, since they've been transferred to the options array)
        free(file_options);
    }

    // After updating the options, search for the previously highlighted option
    if (previously_highlighted) {
        int new_highlight = -1;
        for (int i = 0; i < n_choices; i++) {
            if (strcmp(previously_highlighted, options[i]) == 0) {
                new_highlight = i;
                break;
            }
        }
        if (new_highlight != -1) {
            highlight = new_highlight;
        }
        free(previously_highlighted);
    }
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            title = strdup(argv[++i]);
            process_escapes(title);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            options = realloc(options, (n_choices + 1) * sizeof(char *));
            options[n_choices] = strdup(argv[++i]);
            replace_tab_with_space(options[n_choices]);
            n_o_choices++;
            n_choices++;
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            filename = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0) {
            auto_refresh = 1;
        }
    }

    read_options_from_file();

    // Show help if requested
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
            fprintf(stderr, "%s - interactive terminal menu\n", argv[0]);
            fprintf(stderr, "Usage: %s [OPTIONS]\n", argv[0]);
            fprintf(stderr, "Options:\n");
            fprintf(stderr, "  -t <title>      Set menu title (supports \\n for newlines)\n");
            fprintf(stderr, "  -o <option>     Add menu entry (can be used multiple times)\n");
            fprintf(stderr, "  -f <file>       Read menu entries from file (one per line)\n");
            fprintf(stderr, "  -s              Auto-refresh entries from file every 400ms\n");
            fprintf(stderr, "  -h, --help      Show this help message and exit\n");
            return 0;
        }
    }
    // Show brief help if no options are provided
    if (n_choices == 0 && title == NULL) {
        fprintf(stderr, "Usage: %s [-t <title>] [-o <option>]... [-f <file>] [-s] [-h]\n", argv[0]);
        return 0;
    }

    // Initialize ncurses
    initscr();
    curs_set(0);  // Hide cursor
    noecho();  // Don't echo keypresses
    cbreak();  // Disable line buffering
    keypad(stdscr, TRUE);  // Enable special keys
    nodelay(stdscr, TRUE);  // Make getch() non-blocking
    start_color();

    // Initialize variables
    int ch = 0;
    int max_y, max_x, max_height, max_width;  // Screen dimensions

    while (1) {
        // Get screen dimensions
        getmaxyx(stdscr, max_y, max_x);

        int title_lines = 0;
        if (title) {
            title_lines = 1;
            char *p = title;
            while ((p = strchr(p, '\n')) != NULL) {
                title_lines++;
                p++;
            }
        }

        // Calculate maximum menu height and width (80% of screen dimensions)
        max_width = (int)(max_x * 0.8);
        int title_box_height = title ? (2 + title_lines) : 0;
        max_height = (int)(max_y * 0.8) - title_box_height - 2;
        if (max_height > n_choices) max_height = n_choices;

        // Ensure at least 1 row for the menu
        if (max_height < 1) max_height = 1;

        // Calculate starting positions for menu and title
        int total_height = title_box_height + max_height + 1;
        int start_y = (max_y - total_height) / 2;
        int start_x = (max_x - max_width) / 2;
        int menu_start_y = start_y + title_box_height;

        // Display title and borders
        if (title) {
            attron(A_BOLD);
            char *title_copy = strdup(title);
            if (title_copy) {
                char *line = strtok(title_copy, "\n");
                int i = 0;
                while (line) {
                    // Center the title line
                    int line_len = strlen(line);
                    // Calculate centered x position
                    int centered_x = start_x + (max_width - line_len) / 2;
                    // Ensure centered_x does not go out of bounds
                    if (centered_x < start_x) {
                        centered_x = start_x;
                    }
                    // Print the centered line, truncating it if it doesn't fit in max_width
                    mvprintw(start_y + 1 + i, centered_x, "%.*s", max_width, line);
                    line = strtok(NULL, "\n");
                    i++;
                }
                free(title_copy);
            }
            attroff(A_BOLD);
            
            // Draw borders around title
            mvaddch(start_y, start_x - 1, '+');
            mvaddch(start_y, start_x + max_width, '+');
            mvhline(start_y, start_x, '-', max_width);
            mvvline(start_y + 1, start_x - 1, '|', title_lines);
            mvvline(start_y + 1, start_x + max_width, '|', title_lines);
        }

        // Draw menu box
        mvaddch(menu_start_y - 1, start_x - 1, '+');
        mvaddch(menu_start_y + max_height, start_x - 1, '+');
        mvaddch(menu_start_y - 1, start_x + max_width, '+');
        mvaddch(menu_start_y + max_height, start_x + max_width, '+');
        mvhline(menu_start_y - 1, start_x, '-', max_width);
        mvhline(menu_start_y + max_height, start_x, '-', max_width);
        mvvline(menu_start_y, start_x - 1, '|', max_height);
        mvvline(menu_start_y, start_x + max_width, '|', max_height);

        // Update scroll position based on highlighted item
        if (highlight < scrollpos) scrollpos = highlight;
        else if (highlight >= scrollpos + max_height) scrollpos = highlight - max_height + 1;

        // Show "more" indicator if menu can be scrolled up or down
        attron(A_BOLD);
        if (scrollpos > 0) mvprintw(menu_start_y - 1, start_x + max_width - 4, "more");
        if (scrollpos + max_height < n_choices) mvprintw(menu_start_y + max_height, start_x + max_width - 4, "more");
        attroff(A_BOLD);

        // Display menu items
        for (int i = 0; i < max_height && i + scrollpos < n_choices; ++i) {
            int y = menu_start_y + i;
            int x = start_x;
            char *text = options[i + scrollpos];

            // Highlight the selected menu item
            if (i + scrollpos == highlight) attron(A_REVERSE);

            // Print each menu item with left and right margins
            mvprintw(y, x, " %.*s ", max_width - 2, text);

            // Fill the rest of the line with spaces
            for (int j = strlen(text) + 2; j < max_width; j++) addch(' ');

            // Turn off highlighting
            attroff(A_REVERSE);
        }

        // Refresh the screen
        refresh();

        if (auto_refresh) {
            timeout(400);  // Wait up to 400ms for input
        } else {
            timeout(-1);  // Wait indefinitely for input
        }

        // Handle keypresses
        ch = getch();

        if (auto_refresh) {  // No key was pressed, but auto-refresh is enabled
            read_options_from_file();
            refresh();
        }

        switch (ch) {
            case KEY_UP: if (--highlight < 0) highlight = 0; break;
            case KEY_DOWN: if (n_choices > 0 && ++highlight == n_choices) highlight = n_choices - 1; break;
            case 10:  // Enter key
                if (highlight < n_choices) {
                    fprintf(stderr, "%s\n", options[highlight]);
                }
                endwin();
                return 0;
            case 27: case 'Q': case 'q':  // Esc or Q/q to quit
                endwin();
                return 0;
            case KEY_NPAGE:  // Page down
                highlight += max_height;
                if (highlight >= n_choices) highlight = n_choices - 1;
                scrollpos += max_height;
                if (scrollpos + max_height > n_choices) scrollpos = n_choices - max_height;
                break;
            case KEY_PPAGE:  // Page up
                highlight -= max_height;
                if (highlight < 0) highlight = 0;
                scrollpos -= max_height;
                if (scrollpos < 0) scrollpos = 0;
                break;
            case KEY_RESIZE:  // Resize terminal
                clear();
                break;
        }
    }

    // End ncurses mode
    endwin();

    // Free allocated memory
    free(title);
    for (int i = 0; i < n_choices; i++) {
        free(options[i]);
    }
    free(options);

    return 0;
}
