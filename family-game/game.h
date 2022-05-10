#ifndef __GAME_H
#define __GAME_H

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ASCII_BOLD_ITALIC "\033[3m\033[1m"
#define ASCII_QUIET "\033[2m"
#define ASCII_CLEAR_FORMATTING "\033[0m"

#define COUNTDOWN_SECS 4

typedef struct _card {
    size_t prompt_offset;
    int32_t seconds;
} Card;

typedef struct _card_list {
    Card* card_buf;
    char* prompt_buf;
    int64_t prompt_buf_size;
    int64_t card_count;
} CardList;

CardList parse_cards_file(FILE* file);
void free_card_list(CardList list);
Card choose_card(CardList cards);
void countdown(int32_t time);

void ingame_sigint_handler(int num);
void countdown_sigint_handler(int num);

#endif
