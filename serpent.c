//////////////////////////////////////////////////////////////////
/// File: serpent.c                                            ///
/// Date: 2023-04-27 18:27:34                                  ///
/// Creator: Pawel Hermansdorfer                               ///
/// (C) Copyright by Pawel Hermansdorfer, all rights reserved. ///
//////////////////////////////////////////////////////////////////

#include <sys/unistd.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <sys/select.h>
#include <sys/termios.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "utils.h"
#include "utils.c"

#define STARTING_LENGTH 10

typedef struct Food Food;
struct Food
{
    unsigned int x;
    unsigned int y;
    char symbol;
};

typedef struct SnakeNode SnakeNode;
struct SnakeNode
{
    unsigned int x;
    unsigned int y;
    SnakeNode *next;
    SnakeNode *prev;
};

typedef enum
{
    DirectionUp,
    DirectionRight,
    DirectionDown,
    DirectionLeft,
} SnakeDirection ;

typedef struct Snake Snake;
struct Snake
{
    SnakeNode *first;
    SnakeNode *last;
    SnakeDirection direction;
    char symbol;
};

static void
snake_append(Snake *snake, unsigned int x, unsigned int y)
{
    SnakeNode *new_node = (SnakeNode*)memory_alloc(sizeof(SnakeNode));

    new_node->next = 0;
    new_node->prev = 0;
    new_node->x = x;
    new_node->y = y;
    if(!snake->first || !snake->last)
    {
        snake->first = snake->last = new_node;
    }
    else
    {
        snake->last->next = new_node;
        new_node->prev = snake->last;
        snake->last = new_node;
    }
}

// Cleanup
void
cleanup_src(struct termios *old_attr, int *is_running, Snake *snake, char *grid, unsigned int grid_size)
{
    static int init = 0;
    static int *r_is_running = 0;
    static struct termios *r_attr = 0;
    static Snake *r_snake = {0};
    static char *r_grid = 0;
    static unsigned int r_grid_size = 0;

    // Only initialize vars in first call
    if(!init)
    {
        init = 1;
        r_is_running  = is_running;
        r_attr = old_attr;
        r_snake = snake;
        r_grid = grid;
        r_grid_size = grid_size;
        return;
    }

    // Restore originat attributes
    tcsetattr(0, TCSANOW, r_attr);

    // Clear before exiting
    clear();
    show_cursor();
    exit_alt_buff();

    // Break game loop
    *r_is_running = 0;

    // Free grid
    memory_free(r_grid, r_grid_size);

    //Free snake parts
    SnakeNode *node, *prev_node;
    prev_node = node = r_snake->first;
    r_snake->first = 0;

    int score = 0;
    while(prev_node)
    {
        node = node->next;
        memory_free(prev_node, sizeof(SnakeNode));
        prev_node = node;
        ++score;
    }
    score -= STARTING_LENGTH;
    if(score < 0) score = 0;

    // Output score
    char output_buffer[7 + 10 + 1] = "Score: 0";
    char *p = output_buffer + 7;

    int end_of_zeros = 0;
    for(int div = 1000000000; div > 0; div /= 10)
    {
        int a = score / div;
        score -= a * div;
        if (a != 0 || end_of_zeros)
        {
            *p++ = '0' + a;
            end_of_zeros = 1;
        }
    }
    output_buffer[17] = '\n';
    write(0, "END OF GAME\n", 12);
    write(0, output_buffer, 18);
    usleep(100000);
    _exit(0);
}

void
setup_cleanup(struct termios *old_attr, int *is_running, Snake *snake, char *grid, unsigned int grid_size)
{
    cleanup_src(old_attr, is_running, snake, grid, grid_size);
}

void
cleanup(int sign)
{
    cleanup_src(0, 0, (Snake *){0}, 0, 0);
}

////////////////////////////////////////
int
main(int argc, char **argv)
{
    struct termios old_attr = {0};
    struct termios new_attr = {0};

    // Get current terminal attributes and store them
    tcgetattr(0, &old_attr);
    new_attr = old_attr;
    // Set raw input
    new_attr.c_lflag &= ~(ICANON | ECHO);
    // Set new attributes
    tcsetattr(0, TCSANOW, &new_attr);

    enter_alt_buff();
    hide_cursor();
    clear();

    // Get terminal dimensions
    struct winsize window;
    ioctl(0, TIOCGWINSZ, &window);

    // Init food
    Food food = {window.ws_col / 2, window.ws_row / 4, '@'};
    move_cursor(food.x, food.y);
    print(&food.symbol, 1);

    // Init snake
    Snake snake = {0, 0, DirectionLeft, 'O'};
    for(int i = 0; i < STARTING_LENGTH ; ++i)
    {
        snake_append(&snake, window.ws_col / 2, window.ws_row / 2);
    }

    int is_running = 1;

    unsigned int grid_size = window.ws_col * window.ws_row; 
    char *grid = (char *)memory_alloc(sizeof(char) * grid_size);

    // Handlers for signals
    setup_cleanup(&old_attr, &is_running, &snake, grid, grid_size);
    if(signal(SIGINT, cleanup) == SIG_ERR)
    {
        cleanup(0);
    }

    // Main game loop
    while(is_running)
    {
        unsigned long long frame_start = get_time_ms();
        //////////////////////////////////////// Inputs
        fd_set stdin_fds;
        FD_ZERO(&stdin_fds);
        FD_SET(0, &stdin_fds); // Add stdin to sd_set

        struct timeval timeout = {0, 1};
        select(1, &stdin_fds, 0, 0, &timeout);

        while(FD_ISSET(0, &stdin_fds))
        {
            char input = 0;
            read(0, &input, sizeof(char));
            if(input == 'a' && snake.first->next->x != snake.first->x - 1)
            {
                snake.direction = DirectionLeft;
            }
            else if(input == 'w' && snake.first->next->y != snake.first->y - 1)
            {
                snake.direction = DirectionUp;
            }
            else if(input == 'd' && snake.first->next->x != snake.first->x + 1)
            {
                snake.direction = DirectionRight;
            }
            else if(input == 's' && snake.first->next->y != snake.first->y + 1)
            {
                snake.direction = DirectionDown;
            }
            // quit
            else if(input == 'q' || input == 27)
            {
                is_running = 0;
                break;
            }

            /* TODO(Aa_Pawelek): Do i need this?*/
            /* FD_SET(0, &stdin_fds); */
            select(1, &stdin_fds, 0, 0, &timeout);
        }
        if(!is_running)
        {
            cleanup(0);
            break;
        }

        //////////////////////////////////////// Update
        // Clear grid
        for(unsigned int i = 0; i < grid_size; ++i)
        {
            grid[i] = 0;
        }

        // Update snake
        for(SnakeNode *node = snake.last;
            node != 0;
            node = node->prev)
        {

            if(node == snake.last)
            {
                move_cursor(node->x, node->y);
                print(" ", 1);
            }
            if(node == snake.first)
            {
                switch(snake.direction)
                {
                    case DirectionUp:
                        {
                            node->y--;
                            if(node->y + 1 == 0)
                            {
                                node->y = window.ws_row - 1;
                            }
                        }break;
                    case DirectionDown:  
                        {
                            node->y++;
                            if(node->y >= window.ws_row)
                            {
                                node->y = 0;
                            }
                        }break;
                    case DirectionRight:
                        {
                            node->x++;
                            if(node->x >= window.ws_col)
                            {
                                node->x = 0;
                            }
                        }break;
                    case DirectionLeft:  
                        {
                            node->x--;
                            if(node->x + 1 == 0)
                            {
                                node->x = window.ws_col - 1;
                            }
                        }break;
                }

                for(SnakeNode *collide_node = snake.first->next;
                    collide_node;
                    collide_node = collide_node->next)
                {
                    if(collide_node->x == node->x && collide_node->y == node->y)
                    {
                        cleanup(0);
                    }
                }
            }
            else
            {
                node->x = node->prev->x;
                node->y = node->prev->y;
            }

            move_cursor(node->x, node->y);
            print(&snake.symbol, 1);

            // Mark spot as occupied
            grid[node->x + node->y * window.ws_col] = 1;
        }

        // Cheak if head hit food
        if(snake.first->x == food.x &&
           snake.first->y == food.y)
        {
            unsigned int x = rnd() % window.ws_col;
            unsigned int y = rnd() % window.ws_row;

            // While spot is occupied search for free one 
            unsigned long checked_tiles = 0;
            while(grid[x + y * window.ws_col] != 0)
            {
                ++x;
                ++checked_tiles;
                if(x > window.ws_col)
                {
                    x = 0;
                    y++;
                }
                if(y > window.ws_row)
                {
                    y = 0;
                }
                if(checked_tiles == grid_size)
                {
                    cleanup(0);
                    break;
                }
            }
            food.x = x;
            food.y = y;

            move_cursor(food.x, food.y);
            print(&food.symbol, 1);
            snake_append(&snake, snake.last->x, snake.last->y);
        }

        unsigned long long frame_end = get_time_ms();
        usleep(50000 - (frame_end - frame_start));
    }

    cleanup(0);
    return(0);
}
