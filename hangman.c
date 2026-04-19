/* Hangman Game for group server */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "hangman.h"

HANGMAN_FSM hangman_fsm;

/*
Update the hangman game state based on the input letter. This function will be called when the user sends in a letter guess. It will update the guessed letters and attempts left accordingly.
Will return the message to be printed to the user after processing the guess. The message will indicate whether the guess is correct, incorrect, or if the game has been won or lost.
It will display the hangman itself and the current state of the guessed word (with unguessed letters as underscores).
*/
static char *update_hangman(HANGMAN_FSM *fsm, const char input)
{
    static char response[512];
    static const char *hangman_stages[HANGMAN_MAX_ATTEMPTS + 1] = {
        "  +---+\n"
        "  |   |\n"
        "      |\n"
        "      |\n"
        "      |\n"
        "      |\n"
        "=========",
        "  +---+\n"
        "  |   |\n"
        "  O   |\n"
        "      |\n"
        "      |\n"
        "      |\n"
        "=========",
        "  +---+\n"
        "  |   |\n"
        "  O   |\n"
        "  |   |\n"
        "      |\n"
        "      |\n"
        "=========",
        "  +---+\n"
        "  |   |\n"
        "  O   |\n"
        " /|   |\n"
        "      |\n"
        "      |\n"
        "=========",
        "  +---+\n"
        "  |   |\n"
        "  O   |\n"
        " /|\\  |\n"
        "      |\n"
        "      |\n"
        "=========",
        "  +---+\n"
        "  |   |\n"
        "  O   |\n"
        " /|\\  |\n"
        " / \\  |\n"
        "      |\n"
        "========="};
    char normalized_input;
    size_t word_length;
    int i;
    int found_letter;
    int already_guessed;
    int wrong_stage;
    char masked_word[(HANGMAN_MAX_WORD_LENGTH * 2) + 1];
    int masked_index;
    const char *wrong_guesses;
    const char *initial = "[Hangman System] ";

    if (fsm->hangman_word[0] == '\0')
    {
        return "No word selected for hangman.";
    }

    word_length = strlen(fsm->hangman_word);

    if (!fsm->hangman_round_initialized)
    {
        memset(fsm->revealed_word, HANGMAN_EMPTY_CHARACTER, word_length);
        fsm->revealed_word[word_length] = '\0';
        memset(fsm->guessed_letters, 0, sizeof(fsm->guessed_letters));
        fsm->attempts_left = HANGMAN_MAX_ATTEMPTS;
        fsm->hangman_round_initialized = 1;
    }

    normalized_input = (char)tolower((unsigned char)input);
    found_letter = 0;
    already_guessed = 0;

    if (isalpha((unsigned char)normalized_input))
    {
        for (i = 0; i < (int)word_length; i++)
        {
            if (fsm->hangman_word[i] == normalized_input)
            {
                found_letter = 1;
                if (fsm->revealed_word[i] == normalized_input)
                {
                    already_guessed = 1;
                }
            }
        }

        if (found_letter && !already_guessed)
        {
            for (i = 0; i < (int)word_length; i++)
            {
                if (fsm->hangman_word[i] == normalized_input)
                {
                    fsm->revealed_word[i] = normalized_input;
                }
            }
        }
        else if (!found_letter)
        {
            for (i = 0; fsm->guessed_letters[i] != '\0'; i++)
            {
                if (fsm->guessed_letters[i] == normalized_input)
                {
                    already_guessed = 1;
                    break;
                }
            }

            if (!already_guessed)
            {
                if (i < HANGMAN_MAX_ATTEMPTS)
                {
                    fsm->guessed_letters[i] = normalized_input;
                    fsm->guessed_letters[i + 1] = '\0';
                }

                if (fsm->attempts_left > 0)
                {
                    fsm->attempts_left--;
                }
            }
        }
    }

    masked_index = 0;
    for (i = 0; i < (int)word_length; i++)
    {
        masked_word[masked_index++] = fsm->revealed_word[i];
        if (i < ((int)word_length - 1))
        {
            masked_word[masked_index++] = ' ';
        }
    }
    masked_word[masked_index] = '\0';

    wrong_guesses = fsm->guessed_letters[0] == '\0' ? "-" : fsm->guessed_letters;
    wrong_stage = HANGMAN_MAX_ATTEMPTS - fsm->attempts_left;
    if (wrong_stage < 0)
    {
        wrong_stage = 0;
    }
    if (wrong_stage > HANGMAN_MAX_ATTEMPTS)
    {
        wrong_stage = HANGMAN_MAX_ATTEMPTS;
    }

    if (strcmp(fsm->revealed_word, fsm->hangman_word) == 0)
    {
        sprintf(response,
                "%sCorrect! You guessed the word: %s\n%s\nWord: %s\nWrong guesses: %s\nAttempts left: %d",
                initial,
                fsm->hangman_word,
                hangman_stages[wrong_stage],
                masked_word,
                wrong_guesses,
                fsm->attempts_left);
        fsm->state = HANGMAN_INITIAL;
        fsm->hangman_round_initialized = 0;
        return response;
    }

    if (fsm->attempts_left <= 0)
    {
        sprintf(response,
                "%sIncorrect. Game over! The word was: %s\n%s\nWord: %s\nWrong guesses: %s\nAttempts left: %d",
                initial,
                fsm->hangman_word,
                hangman_stages[HANGMAN_MAX_ATTEMPTS],
                masked_word,
                wrong_guesses,
                fsm->attempts_left);
        fsm->state = HANGMAN_INITIAL;
        fsm->hangman_round_initialized = 0;
        return response;
    }

    if (!isalpha((unsigned char)normalized_input))
    {
        sprintf(response,
                "%sInvalid guess. Please enter a letter.\n%s\nWord: %s\nWrong guesses: %s\nAttempts left: %d",
                initial,
                hangman_stages[wrong_stage],
                masked_word,
                wrong_guesses,
                fsm->attempts_left);
        return response;
    }

    if (already_guessed)
    {
        sprintf(response,
                "%sAlready guessed '%c'.\n%s\nWord: %s\nWrong guesses: %s\nAttempts left: %d",
                initial,
                normalized_input,
                hangman_stages[wrong_stage],
                masked_word,
                wrong_guesses,
                fsm->attempts_left);
        return response;
    }

    if (found_letter)
    {
        sprintf(response,
                "%sCorrect guess: '%c'.\n%s\nWord: %s\nWrong guesses: %s\nAttempts left: %d",
                initial,
                normalized_input,
                hangman_stages[wrong_stage],
                masked_word,
                wrong_guesses,
                fsm->attempts_left);
    }
    else
    {
        sprintf(response,
                "%sIncorrect guess: '%c'.\n%s\nWord: %s\nWrong guesses: %s\nAttempts left: %d",
                initial,
                normalized_input,
                hangman_stages[wrong_stage],
                masked_word,
                wrong_guesses,
                fsm->attempts_left);
    }

    return response;
}

char *process_hangman_state(HANGMAN_FSM *fsm, const char input)
{
    static char response[1024];
    char *bar = "\n==================================================";
    switch (fsm->state)
    {
    case HANGMAN_INITIAL:
        set_random_word(fsm);
        fsm->state = HANGMAN_PLAYING;
        return "Welcome to Hangman! Guess a letter to start playing.";
    case HANGMAN_PLAYING:
        fsm->state = HANGMAN_UPDATE;
        sprintf(response, "%s%s", update_hangman(fsm, input), bar);
        response[sizeof(response) - 1] = '\0'; /* Ensure null termination */
        return response;
    case HANGMAN_UPDATE:
        return "Game is updating..., please wait for your turn.";
    default:
        return "Invalid game state.";
    }
}

/* Sets a random word into hangman_word */
void set_random_word(HANGMAN_FSM *fsm)
{
    char word[1024];
    int count = 0;
    static int random_seeded = 0;
    FILE *file = fopen(HANGMAN_FILE, "r");

    if (!random_seeded)
    {
        srand((unsigned int)time(NULL));
        random_seeded = 1;
    }

    if (file == NULL)
    {
        perror("Failed to open hangman words file");
        return;
    }

    while (fgets(word, sizeof(word), file) != NULL)
    {
        size_t len = strlen(word);

        /* Remove newline character if present */
        if (len > 0 && word[len - 1] == '\n')
        {
            word[len - 1] = '\0';
        }

        /* Check if the word length is valid */
        if (strlen(word) <= HANGMAN_MAX_WORD_LENGTH)
        {
            count++;
            if (rand() % count == 0)
            {
                strncpy(fsm->hangman_word, word, HANGMAN_MAX_WORD_LENGTH);
                fsm->hangman_word[HANGMAN_MAX_WORD_LENGTH] = '\0'; /* Ensure null termination */
            }
        }
    }

    fclose(file);
}

void hangman_finish_broadcast(HANGMAN_FSM *fsm)
{
    if (fsm->state == HANGMAN_UPDATE)
    {
        fsm->state = HANGMAN_PLAYING;
    }
}