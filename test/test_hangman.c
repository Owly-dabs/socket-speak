#include <stdio.h>
#include "hangman.h"

int main()
{
    HANGMAN_FSM hangman_fsm;
    char guess;
    char *message;

    message = process_hangman_state(&hangman_fsm, '\0');
    printf("%s", message);

    while (1)
    {
        if (scanf("%c", &guess) != 1)
        {
            break;
        }

        message = process_hangman_state(&hangman_fsm, guess);
        printf("%s\n", message);
        hangman_finish_broadcast(&hangman_fsm);
    }

    return 0;
}