/* hangman.h */
#ifndef HANGMAN_H
#define HANGMAN_H
#define HANGMAN_FILE "hangman_words.txt"
#define HANGMAN_MAX_WORD_LENGTH 7
#define HANGMAN_MAX_ATTEMPTS 5
#define HANGMAN_EMPTY_CHARACTER '_'

typedef enum
{
    HANGMAN_INITIAL,
    HANGMAN_PLAYING,
    HANGMAN_UPDATE /* When user send in a letter, change to this state HANGMAN_PLAYING after broadcasting */
} STATE_HANGMAN;

typedef struct
{
    STATE_HANGMAN state;
    char hangman_word[HANGMAN_MAX_WORD_LENGTH + 1];
    char guessed_letters[HANGMAN_MAX_ATTEMPTS + 1];
    int attempts_left;
    char revealed_word[HANGMAN_MAX_WORD_LENGTH + 1];
    int hangman_round_initialized;
} HANGMAN_FSM;

extern HANGMAN_FSM hangman_fsm;

char *process_hangman_state(HANGMAN_FSM *fsm, const char input);
void set_random_word(HANGMAN_FSM *fsm);
void hangman_finish_broadcast(HANGMAN_FSM *fsm);

#endif