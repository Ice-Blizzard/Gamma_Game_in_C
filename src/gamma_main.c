/** @file
 * Implementacja wejścia gry gamma.
 *
 * @author Rafał Szulc <r.s.szulc@gmail.com>
 * @date 11.05.2020
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include "gamma.h"

/** @brief Przeprowadza rozgrywkę w trybie wsadowym.
 * @param[in] game         – wskaźnik na strukturę przechowującą stan gry.
 * @param[in] line_number  – numer linijki.
 */
void batch_mode(gamma_t *game, unsigned long long int line_number) {
  while (true) {
    char *line = NULL;
    uint64_t char_number = 0;
    uint64_t allocated_chars_number = 1;
    int c = '0';

    // wzięcie linijki
    while (c != '\n' && c != EOF) {
      c = getchar();

      if (c != '\n' && c != EOF) {
        if (char_number == 0) {
          line = malloc(sizeof (char));
          if (!line) {
            gamma_delete(game);
            exit(1);
          }
        }

        if (char_number == allocated_chars_number) {
          allocated_chars_number = 2 * allocated_chars_number;
          char *new_line = line =
            realloc(line, allocated_chars_number *sizeof (char));
          if (!new_line) {
            free(line);
            gamma_delete(game);
            exit(1);
          }
          line = new_line;
        }

        line[char_number] = c;
        char_number++;
      }
    }

    // nasza linijka musi być stringiem, więc dodaje '\0'
    if (char_number != 0) {
      if (char_number == allocated_chars_number) {
        allocated_chars_number = allocated_chars_number + 1;
        char *new_line = line =
          realloc(line, allocated_chars_number *sizeof (char));
        if (!new_line) {
          free(line);
          gamma_delete(game);
          exit(1);
        }
        line = new_line;
      }

      line[char_number] = '\0';
    }

    // koniec pliku
    if (c == EOF) {
      if (char_number != 0) {
        if (line[0] != '#') {
          free(line);
          fprintf(stderr, "ERROR %llu\n", line_number);
          gamma_delete(game);
          return;
        }
      }

      if (char_number != 0) {
        free(line);
      }
      gamma_delete(game);
      return;
    }

    // wzięcie linii
    if (char_number != 0) {
      if (line[0] != '#') {
        if (line[0] != 'm' && line[0] != 'g' && line[0] != 'b' &&
          line[0] != 'f' && line[0] != 'q' && line[0] != 'p') {

          fprintf(stderr, "ERROR %llu\n", line_number);
        }
        else {
          // sprawdzanie czy mamy '\0' bo to "rozwala" strtok
          bool broken_sign = false;
          for (uint32_t i = 1; i < char_number; i++) {
            if (line[i] == '\0') {
              broken_sign = true;
            }
          }

          if (broken_sign == true) {
            fprintf(stderr, "ERROR %llu\n", line_number);
          }
          else {
            char *words[5];
            words[0] = strtok (line," \t\v\f\r");
            for (int i = 1; i < 5; i++) {
              words[i] = strtok (NULL," \t\v\f\r");
            }

            if (words[0] == NULL) {
              fprintf(stderr, "ERROR %llu\n", line_number);
            }
            else {
              if (words[4] != NULL) {
                fprintf(stderr, "ERROR %llu\n", line_number);   
              }
              else {
                if (words[1] == NULL) {
                  if (strcmp(words[0], "p") != 0) {
                    fprintf(stderr, "ERROR %llu\n", line_number);
                  }
                  else {
                    char *result = gamma_board(game);

                    if (result == NULL) {
                      fprintf(stderr, "ERROR %llu\n", line_number);
                    }
                    else {
                      printf("%s", result);
                      free(result);
                    }
                  }
                }
                else {
                  if (words[2] == NULL) {
                    if ((strcmp(words[0], "b") != 0) &&
                      (strcmp(words[0], "f") != 0) &&
                      (strcmp(words[0], "q") != 0)) {

                      fprintf(stderr, "ERROR %llu\n", line_number);
                    }
                    else {
                      uint32_t number;
                      bool error = false;
      
                      // sprawdzanie czy nasze drugie słowo jest liczbą
                      char *not_numbers_word;
                      not_numbers_word = strtok (words[1],"0123456789");
                      if (not_numbers_word != NULL) {
                        fprintf(stderr, "ERROR %llu\n", line_number);
                        error = true;
                      }

                      if (error == false) {
                        unsigned long long tmp = strtoull(words[1], NULL, 10);

                        if (errno == ERANGE || tmp > UINT32_MAX) {
                          fprintf(stderr, "ERROR %llu\n", line_number);
                        }
                        else {
                          number = tmp;

                          if (strcmp(words[0], "b") == 0) {
                            printf("%li\n", gamma_busy_fields(game, number));
                          }

                          if (strcmp(words[0], "f") == 0) {
                            printf("%li\n", gamma_free_fields(game, number));
                          }

                          if (strcmp(words[0], "q") == 0) {
                            if (gamma_golden_possible(game, number)
                              == false) {

                              printf("0\n");
                            }
                            else {
                              printf("1\n");
                            }
                          }
                        }
                      }
                    }
                  }
                  else {
                    if (words[3] == NULL) {
                      fprintf(stderr, "ERROR %llu\n", line_number);
                    }
                    else {
                      if ((strcmp(words[0], "m") != 0) &&
                        (strcmp(words[0], "g") != 0)) {

                        fprintf(stderr, "ERROR %llu\n", line_number);
                      }
                      else {
                        uint32_t numbers[3];
                        bool error = false;

                        // sprawdzanie czy nasze słowa
                        // (poza pierwszym) są liczbami
                        char *not_numbers[3];
                        for (int i = 0; i < 3; i++) {
                          if (error == false) {
                            not_numbers[i] =
                              strtok (words[i + 1],"0123456789");
                            if (not_numbers[i] != NULL) {
                              fprintf(stderr, "ERROR %llu\n", line_number);
                              error = true;
                            }
                          }
                        }

                        for (int i = 0; i < 3; i++) {
                          if (error == false) {
                            unsigned long long tmp =
                              strtoull(words[i + 1], NULL, 10);

                            if (errno == ERANGE || tmp > UINT32_MAX) {
                              fprintf(stderr, "ERROR %llu\n", line_number);
                              error = true;
                            }
                            else {
                              numbers[i] = tmp;
                            }
                          }
                        }

                        if (error == false) {
                          if (strcmp(words[0], "m") == 0) {
                            if (gamma_move(game, numbers[0],
                              numbers[1], numbers[2]) == false) {

                              printf("0\n");
                            }
                            else {
                              printf("1\n");
                            }
                          }
                          else {
                            if (gamma_golden_move(game, numbers[0],
                              numbers[1], numbers[2]) == false) {

                              printf("0\n");
                            }
                            else {
                              printf("1\n");
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    if (char_number != 0) {
      free(line);
    }
    line_number++;
  }
}

/** @brief Przeprowadza rozgrywkę w trybie interaktywnym.
 * @param[in] game         – wskaźnik na strukturę przechowującą stan gry.
 */
void interactive_mode(gamma_t *game) {
  bool game_over = false;
  uint32_t number_of_players = return_players(game);
  uint32_t length_of_number = number_of_digits(number_of_players);
  uint32_t failed_rounds = 0;
  uint32_t current_player = 1;
  uint32_t x_coordinate_cursor = return_height(game);
  uint32_t y_coordinate_cursor = length_of_number;
  uint32_t x_coordinate = 0;
  uint32_t y_coordinate = 0;

  // najpierw sprawdzamy, czy terminal jest wystarczająco duży do tej gry
  struct winsize w;
  ioctl(0, TIOCGWINSZ, &w);
  if (w.ws_row < (return_height(game) + 1) ||
    w.ws_col < (return_width(game) * (length_of_number + 1))) {

    printf("ERROR, TOO SMALL TERMINAL\n");
    gamma_delete(game);
    exit(1);
  }

  // gra się odbywa, terminal jest ok
  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON|ECHO|ECHOK|ECHOE|ECHONL|ISIG|IEXTEN);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  while (game_over == false) {
    // omijamy gracza, bo nie może zrobić ruchu
    if (gamma_free_fields(game, current_player) == 0 &&
      gamma_golden_possible(game, current_player) == false) {

      failed_rounds++;
      if (failed_rounds == number_of_players) {
        game_over = true;
      }
    }
    // gracz ma ruch do zrobienia
    else {
      bool move_done = false;
      int arrow = 0;
      while (move_done == false) {
        printf("\x1b[2J");
        printf("\x1b[H");
        // wyśweitlanie planszy z kolorowaniem pól gracza, która ma ruch
        // i podświetlaniem kursora dla wielocyfrowej planszy
        char *state_of_game = gamma_board(game);
        // plasza z więcej niż 9 graczami
        if (return_players(game) > 9) {
          uint64_t i = 0;
          while (state_of_game[i] != '\0') {
            if (state_of_game[i] == ' ' || state_of_game[i] == '.' ||
              state_of_game[i] == '\n') {
              // kursor na pustym polu
              if (state_of_game[i] == '.' &&
                i % (return_width(game) * (length_of_number + 1) + 1)
                == y_coordinate_cursor - 1 &&
                i / (return_width(game) * (length_of_number + 1) + 1)
                == x_coordinate_cursor - 1) {

                printf("\033[1;45m");
                printf("%c", state_of_game[i]);
                printf("\033[0m");
              }
              else {
                printf("\033[0m");
                printf("%c", state_of_game[i]);
              }
              i++;
            }
            else {
              uint32_t player_on_this_place = 0;
              while (state_of_game[i] != ' ' && state_of_game[i] != '.' &&
                state_of_game[i] != '\n') {

                player_on_this_place = player_on_this_place * 10;
                player_on_this_place = player_on_this_place +
                  state_of_game[i] - '0';
                i++;
              }
              // kursor na polu zajętym przez gracza, który ma ruch
              if (player_on_this_place == current_player &&
                (i - 1) % (return_width(game) * (length_of_number + 1) + 1)
                == y_coordinate_cursor - 1 &&
                (i - 1) / (return_width(game) * (length_of_number + 1) + 1)
                == x_coordinate_cursor - 1) {

                printf("\033[1;36;45m");
              }
              else {
                // pole zajęte przez gracza, który ma ruch
                if (player_on_this_place == current_player) {
                  printf("\033[1;36m");
                }
                // pole z kursorem
                if ((i - 1) % (return_width(game) * (length_of_number + 1)
                + 1) == y_coordinate_cursor - 1 &&
                (i - 1) / (return_width(game) * (length_of_number + 1)
                +1) == x_coordinate_cursor - 1) {

                  printf("\033[1;45m");
                }
              }
              printf("%u", player_on_this_place);
            }
          }
        }
        // plansza z max 9 graczami
        else {
          uint64_t i = 0;
          while (state_of_game[i] != '\0') {
            if (state_of_game[i] == ' ' || state_of_game[i] == '.' ||
              state_of_game[i] == '\n') {

              printf("%c", state_of_game[i]);
              i++;
            }
            else {
              uint32_t player_on_this_place = state_of_game[i] - '0';
              if (current_player == player_on_this_place) {
                printf("\033[1;36m");
                printf("%c", state_of_game[i]);
                printf("\033[0m");
                i++;
              }
              else {
                printf("%c", state_of_game[i]);
                i++;
              }
            }
          }
        }
        free(state_of_game);
        printf("\033[0;36m");
        printf("PLAYER ");
        printf("%u", current_player);
        printf(" ");
        printf("\033[1;36m");
        printf("%lu", return_fields_taken(game, current_player));
        printf(" ");
        if (gamma_golden_possible(game, current_player) == true) {
          printf("\033[1;32m");
          printf("%lu", return_free_fields_around(game, current_player));
          printf("\033[1;33m");
          printf(" G\n");
        }
        else {
          printf("\033[1;32m");
          printf("%lu\n", return_free_fields_around(game, current_player));
        }
        printf("\033[0m");

        printf("\033[?25h");
        printf("\033[%d;%dH", x_coordinate_cursor, y_coordinate_cursor);
        if (return_players(game) > 9) {
          printf("\033[?25l");
        }
        int c = getchar();
        if (c == EOF) {
          tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
          gamma_delete(game);
          exit(1);
        }

        if (c == 'c' || (c == 'C' && arrow != 2)) {
          move_done = true;
        }
        else {
          if (c == 'g' || c == 'G') {
            if (gamma_golden_move(game, current_player,
              x_coordinate, y_coordinate) == true) {

              arrow = 0;
              move_done = true;
            }         
          }
          else {
            if (isspace(c)) {
              if (gamma_move(game, current_player,
                x_coordinate, y_coordinate) == true) {

                arrow = 0;
                move_done = true;
              }
            }
            else {
              if (c == '\4') {
                move_done = true;
                game_over = true;
              }
              else {
                if (c == '\x1b') {
                  arrow = 1;
                }
                else {
                  if (arrow == 1 && c == '[') {
                    arrow = 2;
                  }
                  else {
                    if (arrow == 2 && c == 'A') {
                      if (y_coordinate != (return_height(game) - 1)) {
                        y_coordinate++;
                        x_coordinate_cursor--;
                      }
                    }

                    if (arrow == 2 && c == 'B') {
                      if (y_coordinate != 0) {
                        y_coordinate--;
                        x_coordinate_cursor++;
                      }
                    }

                    if (arrow == 2 && c == 'C') {
                      if (x_coordinate != (return_width(game) - 1)) {
                        x_coordinate++;
                        if (length_of_number == 1) {
                          y_coordinate_cursor++;
                        }
                        else {
                          y_coordinate_cursor = y_coordinate_cursor +
                            length_of_number + 1;
                        }
                      }
                    }

                    if (arrow == 2 && c == 'D') {
                      if (x_coordinate != 0) {
                        x_coordinate--;
                        if (length_of_number == 1) {
                          y_coordinate_cursor--;
                        }
                        else {
                          y_coordinate_cursor = y_coordinate_cursor -
                            length_of_number - 1;
                        }
                      }
                    }

                    arrow = 0;
                  }
                }
              }
            }
          }
        }
      }

      printf("\033[?25l");
    }

    if (current_player == number_of_players) {
      current_player = 1;
    }
    else {
      current_player++;
    }
  }

  // koniec trybu interaktywnego, wyświetlenie końcowej planszy
  printf("\x1b[2J");
  printf("\x1b[H");
  char *result = gamma_board(game);
  printf("%s", result);
  free(result);
  // szukanie zwycięscy/zwycięsców
  uint64_t max_fields_taken = 0;
  for (uint32_t i = 1; i <= number_of_players; i++) {
    if (return_fields_taken(game, i) > max_fields_taken) {
      max_fields_taken = return_fields_taken(game, i);
    }
  }
  // końcowe podsumowanie
  for (uint32_t i = 1; i <= number_of_players; i++) {
    if (return_fields_taken(game, i) < max_fields_taken) {
      printf("\033[0;31m");
    }
    else {
      printf("\033[1;32m");
    }
    printf("PLAYER ");
    printf("%u", i);
    printf(" ");
    if (return_fields_taken(game, i) < max_fields_taken) {
      printf("%lu\n", return_fields_taken(game, i));
    }
    else {
      printf("%lu", return_fields_taken(game, i));
      printf("\033[1;33m");
      printf(" VICTORY\n");
    }
    printf("\033[0m");
  }
  printf("\033[?25h");
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  gamma_delete(game);
  return;
}

/** @brief Przeprowadza grę w gamma.
 * Wczytuje wejście decydując czy tryb gry jest wsadowy czy interaktywny
 * następnie przeprowadza rozgrywkę w wybranym trybie.
 * @return Zero, gdy gra przebiegła poprawnie,
 * a w przeciwnym przypadku kod zakończenia programu jest kodem błędu.
 */
int main() {
  bool batch = false;
  bool interactive = false;
  unsigned long long int line_number = 1;
  gamma_t *game;

  // sprawdzanie czy tryb gry jest interaktywny czy wsadowy
  while (batch == false && interactive == false) {
    char *line = NULL;
    uint64_t char_number = 0;
    uint64_t allocated_chars_number = 1;
    int c = '0';

    // wzięcie linijki
    while (c != '\n' && c != EOF) {
      c = getchar();

      if (c != '\n' && c != EOF) {
        if (char_number == 0) {
          line = malloc(sizeof (char));
          if (!line) {
            exit(1);
          }
        }

        if (char_number == allocated_chars_number) {
          allocated_chars_number = 2 * allocated_chars_number;
          char *new_line = line =
            realloc(line, allocated_chars_number *sizeof (char));
          if (!new_line) {
            free(line);
            exit(1);
          }
          line = new_line;
        }

        line[char_number] = c;
        char_number++;
      }
    }

    // nasza linijka musi być stringiem, więc dodaje '\0'
    if (char_number != 0) {
      if (char_number == allocated_chars_number) {
        allocated_chars_number = allocated_chars_number + 1;
        char *new_line = line =
          realloc(line, allocated_chars_number *sizeof (char));
        if (!new_line) {
          free(line);
          exit(1);
        }
        line = new_line;
      }

      line[char_number] = '\0';
    }

    // koniec pliku
    if (c == EOF) {
      if (char_number != 0) {
        if (line[0] != '#') {
          free(line);
          fprintf(stderr, "ERROR %llu\n", line_number);
          return 0;
        }
      }

      if (char_number != 0) {
        free(line);
      }
      return 0;
    }

    // koniec linii
    if (char_number != 0) {
      if (line[0] != '#') {
        if (line[0] != 'B' && line[0] != 'I') {
          fprintf(stderr, "ERROR %llu\n", line_number);
        }
        else {
          // sprawdzanie czy mamy '\0' bo to "rozwala" strtok
          bool broken_sign = false;
          for (uint32_t i = 1; i < char_number; i++) {
            if (line[i] == '\0') {
              broken_sign = true;
            }
          }

          if (broken_sign == true) {
            fprintf(stderr, "ERROR %llu\n", line_number);
          }
          else {
            char *words[6];
            words[0] = strtok (line," \t\v\f\r");
            for (int i = 1; i < 6; i++) {
              words[i] = strtok (NULL," \t\v\f\r");
            }

            if (words[4] == NULL || words[5] != NULL) {
              fprintf(stderr, "ERROR %llu\n", line_number);   
            }
            else {
              if (strcmp(words[0], "B") != 0 && strcmp(words[0], "I") != 0) {
                fprintf(stderr, "ERROR %llu\n", line_number);
              }
              else {
                uint32_t numbers[4];
                bool error = false;

                // sprawdzanie czy nasze słowa (poza pierwszym) są liczbami
                char *not_numbers[4];
                for (int i = 0; i < 4; i++) {
                  if (error == false) {
                    not_numbers[i] = strtok (words[i + 1],"0123456789");
                    if (not_numbers[i] != NULL) {
                      fprintf(stderr, "ERROR %llu\n", line_number);
                      error = true;
                    }
                  }
                }

                for (int i = 0; i < 4; i++) {
                  if (error == false) {
                    unsigned long long tmp = strtoull(words[i + 1], NULL, 10);

                    if (errno == ERANGE || tmp > UINT32_MAX) {
                      fprintf(stderr, "ERROR %llu\n", line_number);
                      error = true;
                    }
                    else {
                      numbers[i] = tmp;
                    }
                  }
                }

                if (error == false) {
                  game = gamma_new(numbers[0], numbers[1],
                    numbers[2], numbers[3]);

                  if (game == NULL) {
                    fprintf(stderr, "ERROR %llu\n", line_number);
                  }
                  else {
                    if (strcmp(words[0], "B") == 0) {
                      printf("OK %llu\n", line_number);
                      batch = true;
                    }
                    else {
                      interactive = true;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    if (char_number != 0) {
      free(line);
    }
    line_number++;
  }

  if (batch == true) {
    batch_mode(game, line_number);
  }

  if (interactive == true) {
    interactive_mode(game);
  }

  return 0;
}
