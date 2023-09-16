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
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            options = realloc(options, (n_choices + 1) * sizeof(char *));
            options[n_choices++] = strdup(argv[++i]);
            n_o_choices++;
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            filename = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0) {
            auto_refresh = 1;
        }
    }

    read_options_from_file();

    if (n_choices == 0) {
        fprintf(stderr, "This is a basic ncurses menu.\n");
        fprintf(stderr, "- Use -t for menu title\n");
        fprintf(stderr, "- Use -o for menu entries.\n");
        fprintf(stderr, "- Use -f to read menu entries from a file.\n");
        fprintf(stderr, "- Use -s to auto-refresh entries from file every second.\n");
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

        // Calculate maximum menu height and width (80% of screen dimensions)
        max_width = (int)(max_x * 0.8);
        max_height = (int)(max_y * 0.8) - 5;
        if (max_height > n_choices) max_height = n_choices;

        // Ensure at least 1 row for the menu
        if (max_height < 1) max_height = 1;

        // Calculate starting positions for menu and title
        int start_y = (max_y - max_height - 5) / 2 + 3;
        int start_x = (max_x - max_width) / 2;

        // Display title with left and right margins
        attron(A_BOLD);
        mvprintw(start_y - 2, start_x, " %.*s ", max_width - 2, title);
        attroff(A_BOLD);

        // Draw borders around title and menu
        mvaddch(start_y - 3, start_x - 1, '+');
        mvaddch(start_y - 3, start_x + max_width, '+');
        mvhline(start_y - 3, start_x, '-', max_width);
        mvvline(start_y - 2, start_x - 1, '|', 1);
        mvvline(start_y - 2, start_x + max_width, '|', 1);

        // Draw menu box
        mvaddch(start_y - 1, start_x - 1, '+');
        mvaddch(start_y + max_height, start_x - 1, '+');
        mvaddch(start_y - 1, start_x + max_width, '+');
        mvaddch(start_y + max_height, start_x + max_width, '+');
        mvhline(start_y - 1, start_x, '-', max_width);
        mvhline(start_y + max_height, start_x, '-', max_width);
        mvvline(start_y, start_x - 1, '|', max_height);
        mvvline(start_y, start_x + max_width, '|', max_height);

        // Update scroll position based on highlighted item
        if (highlight < scrollpos) scrollpos = highlight;
        else if (highlight >= scrollpos + max_height) scrollpos = highlight - max_height + 1;

        // Show "more" indicator if menu can be scrolled up or down
        attron(A_BOLD);
        if (scrollpos > 0) mvprintw(start_y - 1, start_x + max_width - 4, "more");
        if (scrollpos + max_height < n_choices) mvprintw(start_y + max_height, start_x + max_width - 4, "more");
        attroff(A_BOLD);

        // Display menu items
        for (int i = 0; i < max_height && i + scrollpos < n_choices; ++i) {
            int y = start_y + i;
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
            timeout(1000);  // Wait up to 1 second for input
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
            case KEY_DOWN: if (++highlight == n_choices) highlight = n_choices - 1; break;
            case 10:  // Enter key
                fprintf(stderr, "%s\n", options[highlight]);
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
