#include "game.h"

int main(int argc, char** argv)
{
    char* cards_file_path = 0;
    bool are_args_valid = true;
    
    if (argc >= 2)
    {
        cards_file_path = argv[1];
    }

    if (argc < 0 || !cards_file_path)
    {
        printf("No card file specified. Run the program agin specifying the cards file:\n");
        printf("%s [CARDS_FILE_PATH]\n", argv[0]);
        exit(1);
    }

    FILE* cards_file = fopen(cards_file_path, "r");
    if (!cards_file)
    {
        printf("Failed to open cards file '%s'\n", cards_file_path);
        exit(1);
    }

    CardList cards = parse_cards_file(cards_file);
    fclose(cards_file);

    signal(SIGINT, ingame_sigint_handler);

    printf("Welcome to The Game. Let's begin.\n");
    srand(time(0));
    
    while(true)
    {
        printf("\nPress ENTER when you are ready for a card.\n");
        getc(stdin);

        Card card = choose_card(cards);
        printf("-----------------------------------------------------------------\n");
        printf("You will have %d seconds to complete the folling prompt prompt:\n\n", card.seconds);

        printf(ASCII_BOLD_ITALIC);
        printf("%s\n", cards.prompt_buf + card.prompt_offset);
        printf(ASCII_CLEAR_FORMATTING);
        printf("-----------------------------------------------------------------\n\n\n");

        printf("Get ready...\n");

        countdown(COUNTDOWN_SECS);
        signal(SIGINT, ingame_sigint_handler);
        
        printf("\nGo!\n");
        
        countdown(card.seconds);
        signal(SIGINT, ingame_sigint_handler);

        printf("\nTIMER DONE!\n");
    }
        
    free_card_list(cards);

    return 0;
}

CardList parse_cards_file(FILE* file)
{
    // Find longest line and count number of lines
    // NOTE: Memory footprint can be shrunk by only setting the buffer to the longest
    //       length of text not in a comment and before a tilde (~)
    int64_t line_count = 0;
    int32_t max_line_length = 0;
    int32_t curr_line_length = 0;
    for(char curr = fgetc(file); curr != EOF; curr = fgetc(file), ++curr_line_length)
    {
        if (curr == '\n')
        {
            ++line_count;

            if (curr_line_length > max_line_length)
                max_line_length = curr_line_length;

            curr_line_length = -1;
        }
    }
    
    rewind(file);

    // Add a byte for terminating 0
    ++max_line_length;
    
    char* prompt_buffer = malloc(max_line_length * line_count);
    Card* card_buffer = malloc(sizeof(Card) * line_count);

    if (!prompt_buffer || !card_buffer)
    {
        printf("malloc failure\n");
        exit(1);
    }

    int64_t pos_in_line = 0;
    int64_t line_number = 0;

    Card* card_buffer_cursor = card_buffer;

    bool hit_tilde = false;
    int64_t tilde_pos_in_line = 0;
 
    for(char curr = fgetc(file); curr != EOF; curr = fgetc(file), ++pos_in_line)
    {
        if (curr == '#' && pos_in_line == 0)
        {
            pos_in_line = -1;
            --line_count;

            do
            {
                if (curr == EOF)
                    goto end_of_outer_loop; // Break out of outer loop
                
                curr = fgetc(file);
            }
            while (curr != '\n');

            continue;
        }
        else if (curr == '\n')
        {
            *(prompt_buffer + max_line_length * line_number + pos_in_line) = 0;
                        
            if (!hit_tilde)
            {
                // Ignore line
                --line_count;
                --line_number;
            }
            else
            {
                int32_t seconds = strtol(prompt_buffer
                                         + max_line_length * line_number
                                         + tilde_pos_in_line
                                         + 1,
                                         0, 10);
                if (seconds == 0)
                {
                    printf("Invalid line in game file. The time to perform the action was invalid ");
                    printf("for the following action:\n\"%s\"\n", prompt_buffer + max_line_length * line_number);
                    exit(1);
                }

                if (seconds < 0)
                {
                    seconds = -seconds;
                }
                    
                Card card = {
                    .prompt_offset = max_line_length * line_number,
                    .seconds = seconds,
                };

                *card_buffer_cursor = card;
                ++card_buffer_cursor;
            }

            ++line_number;
            pos_in_line = -1;
            
            hit_tilde = false;
            
            continue;
        }
        else if (curr == '~')
        {
            hit_tilde = true;
            tilde_pos_in_line = pos_in_line;

            char* curr_pos = prompt_buffer + max_line_length * line_number + pos_in_line;
            *(curr_pos) = 0;
            
            // Remove trailing space from prompt
            if (*(curr_pos - 1) == ' ' || *(curr_pos - 1) == '\t')
                *(curr_pos - 1) = 0;
        }

        *(prompt_buffer + max_line_length * line_number + pos_in_line) = curr;
    }
end_of_outer_loop:

    *(prompt_buffer + max_line_length * line_number + pos_in_line) = 0;

    CardList cards = {
        .card_buf = realloc(card_buffer, sizeof(Card) * line_count),
        .prompt_buf = realloc(prompt_buffer, max_line_length * line_count),
        .prompt_buf_size = max_line_length,
        .card_count = line_count,
    };
    
    return cards;
}

void free_card_list(CardList list)
{
    free(list.card_buf);
    free(list.prompt_buf);
}

Card choose_card(CardList cards)
{
    int32_t rand_card_index = (rand() ^ time(0)) % cards.card_count;
    return *(cards.card_buf + rand_card_index);
}

static bool countdown_continue = false;

void countdown(int32_t seconds)
{
    const int64_t checks_per_second = 60;
        
    signal(SIGINT, countdown_sigint_handler);

    printf(ASCII_QUIET);
    printf("Press Ctrl + C to stop the timer\n\n");
    printf(ASCII_CLEAR_FORMATTING);

    countdown_continue = true;
    for (uint8_t check_count = 0; seconds >= 0 && countdown_continue; --check_count)
    {
        if (check_count == 0)
        {
            int32_t minutes = seconds / 60;
            int32_t second_less_minutes = seconds - (60 * minutes);

            printf("%02d:%02d\n", minutes, second_less_minutes);

            --seconds;
            check_count = checks_per_second + 1;
        }
            
        usleep((1.0 / (float) checks_per_second) * 1000000);
    }
}

void countdown_sigint_handler(int num)
{
    countdown_continue = false;
}

static bool awaiting_response_to_quit = false;

void ingame_sigint_handler(int num)
{
    if (!awaiting_response_to_quit)
    {
        awaiting_response_to_quit = true;
        
        printf("\nAre you sure you want to quit? (y/n) ");
        char response = getc(stdin);

        if (response == 'y')
            exit(0);

        awaiting_response_to_quit = false;
    }
}
