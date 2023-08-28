#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc <= 1) {
       fprintf(stderr, "This is basic ncurses menu.\n");
       fprintf(stderr, "- first argument is menu title\n");
       fprintf(stderr, "- other arguments are menu entries.\n");
       fprintf(stderr, "- menu entry selected with Enter is printed to stderr\n");
       return 0;
    }

    // Initialize variables
    int ch, highlight = 0, scroll = 0;
    int n_choices = argc - 2;  // Number of menu choices
    int max_y, max_x, max_height, max_width;  // Screen dimensions
    char *title = argv[1];  // Menu title

    // Initialize ncurses
    initscr();
    curs_set(0);  // Hide cursor
    noecho();  // Don't echo keypresses
    cbreak();  // Disable line buffering
    keypad(stdscr, TRUE);  // Enable special keys

    while (1) {
        // Get screen dimensions
        getmaxyx(stdscr, max_y, max_x);
        // Calculate maximum menu height and width (80% of screen dimensions)
        max_width = (int)(max_x * 0.4);
        max_height = (int)(max_y * 0.8) - 5;
        if (max_height > argc - 2) max_height = argc - 2;

        // Ensure at least 1 row for the menu
        if (max_height < 1) max_height = 1;

        // Calculate starting positions for menu and title
        int start_y = (max_y - max_height - 5) / 2 + 3;
        int start_x = (max_x - max_width) / 2;

        // Display title with left and right margins
        mvprintw(start_y - 2, start_x, " %.*s ", max_width - 2, title);

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
        if (highlight < scroll) scroll = highlight;
        else if (highlight >= scroll + max_height) scroll = highlight - max_height + 1;

        // Show "more" indicator if menu can be scrolled up or down
        if (scroll > 0) mvprintw(start_y - 1, start_x + max_width - 4, "more");
        if (scroll + max_height < n_choices) mvprintw(start_y + max_height, start_x + max_width - 4, "more");

        // Display menu items
        for (int i = 0; i < max_height && i + scroll < n_choices; ++i) {
            int y = start_y + i;
            int x = start_x;
            char *text = argv[i + scroll + 2];

            // Highlight the selected menu item
            if (i + scroll == highlight) attron(A_REVERSE);

            // Print each menu item with left and right margins
            mvprintw(y, x, " %.*s ", max_width - 2, text);

            // Fill the rest of the line with spaces
            for (int j = strlen(text) + 2; j < max_width; j++) addch(' ');

            // Turn off highlighting
            attroff(A_REVERSE);
        }

        // Refresh the screen
        refresh();

        // Handle keypresses
        ch = getch();
        switch (ch) {
            case KEY_UP: if (--highlight < 0) highlight = 0; break;
            case KEY_DOWN: if (++highlight == n_choices) highlight = n_choices - 1; break;
            case 10:  // Enter key
                fprintf(stderr, "%s\n", argv[highlight + 2]);
                endwin();
                return 0;
            case 27: case 'Q': case 'q':  // Esc or Q/q to quit
                endwin();
                return 0;
            case KEY_NPAGE:  // Page down
                highlight += max_height;
                if (highlight >= n_choices) highlight = n_choices - 1;
                scroll += max_height;
                if (scroll + max_height > n_choices) scroll = n_choices - max_height;
                break;
            case KEY_PPAGE:  // Page up
                highlight -= max_height;
                if (highlight < 0) highlight = 0;
                scroll -= max_height;
                if (scroll < 0) scroll = 0;
                break;
            case KEY_RESIZE:  // Resize terminal
                clear();
                break;
        }
    }

    // End ncurses mode
    endwin();
    return 0;
}
