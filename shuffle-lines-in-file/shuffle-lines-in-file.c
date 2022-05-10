#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct _line_grid {
    char* line_arr;
    int64_t line_count;
    int64_t line_buf_size;
} LineGrid;

LineGrid break_into_lines(FILE* file)
{
    // Find longest line and count number of lines
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
        else if (curr == '\r')
        {
            // Don't include carriage return in line length
            --curr_line_length;
        }
    }
    
    rewind(file);

    // Add a byte for terminating 0
    ++max_line_length;
    
    char* grid = malloc(max_line_length * line_count);

    if (!grid)
    {
        printf("malloc failure\r\n");
        exit(1);
    }

    int64_t pos_in_line = 0;
    int64_t line_number = 0;

    bool was_last_br = false;
    for(char curr = fgetc(file); curr != EOF; curr = fgetc(file), ++pos_in_line)
    {
        if (curr == '\n' || curr == '\r')
        {
            // If curr is '\r', assume the next char will be '\n'
            if (!was_last_br)
            {
                *(grid + max_line_length * line_number + pos_in_line) = 0;
                ++line_number;
            }

            pos_in_line = -1;
            was_last_br = true;
            continue;
        }

        *(grid + max_line_length * line_number + pos_in_line) = curr;
        was_last_br = false;
    }

    *(grid + max_line_length * line_number + pos_in_line) = 0;

    LineGrid line_grid = {
        .line_arr = (char*) grid,
        .line_count = line_count,
        .line_buf_size = max_line_length,
    };
    
    return line_grid;
}

void swap_lines(char* line1, char* line2, int64_t line_buf_size)
{
    char temp[line_buf_size];
    memcpy(temp, line1, line_buf_size);
    memmove(line1, line2, line_buf_size);
    memcpy(line2, temp, line_buf_size);
}

int main(int argc, char** argv)
{
    char* in_file = 0;
    char* out_file = 0;
    bool are_args_valid = true;
    
    if (argc == 5)
    {
        for(int i = 1; i < argc - 1; ++i)
        {
            if (!strcmp(argv[i], "-in"))
                in_file = argv[++i];
            else if (!strcmp(argv[i], "-out"))
                out_file = argv[++i];
        }
    }

    if (argc != 5 || !in_file || !out_file)
    {
        printf("Invalid arguments. Arguments should be in the following format:\r\n");
        printf("%s -in [IN_FILE] -out [OUT_FILE]\r\n", argv[0]);
        exit(1);
    }
    
    srand(time(NULL));

    FILE* file = fopen(in_file, "r");
    if (!file)
    {
        printf("Failed to open input file '%s'\r\n", in_file);
        exit(1);
    }

    LineGrid lines = break_into_lines(file);
    fclose(file);
    
    // Choose two lines at random and swap them 100 times the line count
    for (uint64_t i = 0; i < lines.line_count * 100; ++i)
    {
        uint64_t line1_num = rand() % lines.line_count;
        uint64_t line2_num = rand() % lines.line_count;
        
        swap_lines(lines.line_arr + (line1_num * lines.line_buf_size + 1),
                   lines.line_arr + (line2_num * lines.line_buf_size + 1),
                   lines.line_buf_size);
    }

    file = fopen(out_file, "w");
    if (!file)
    {
        printf("Failed to open output file '%s'\r\n", out_file);
        exit(1);
    }
    
    char* curr = lines.line_arr;
    for (int64_t i = 0; i < lines.line_count; ++i, curr += lines.line_buf_size)
        fprintf(file, "%s\r\n", curr);


    fclose(file);
    free(lines.line_arr);

    return 0;
}
