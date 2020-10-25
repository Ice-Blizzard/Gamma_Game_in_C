/** @file
 * Implementacja silnika gry gamma.
 *
 * @author Rafał Szulc <r.s.szulc@gmail.com>
 * @date 16.04.2020
 */

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "gamma.h"

/** @struct gamma
 * Deklaracja struktury gamma.
*/
struct gamma {
  uint32_t width; ///< Długość planszy.
  uint32_t height; ///< Wysokość planszy.
  uint32_t players; ///< Liczba graczy.
  uint32_t areas; ///< Maksymalna liczba aren pojedynczego gracza.
  uint64_t free_fields; ///< Ilość wolnych pól na planszy.
  bool *golden; ///< Tablica, golden[i] mówi czy i + 1 gracz wykonał złoty ruch.
  uint64_t *fields_taken; /**< Tablica, analogicznie do golden, zabrane pola
  * przez gracza. */
  uint64_t *areas_taken; /**< Tablica, analogicznie do golden, aktualna
  * ilość aren zajmowana przez gracza. */
  uint64_t *free_fields_around; /**< Tablica, analogicznie do golden, ilość
  * legalnych pól do zajęcia przez gracza. */
  uint64_t **rank; ///< Tablica pomocnicza do find&union.
  uint64_t **parent; ///< Tablica pomocnicza do find&union.
  uint64_t **copy_rank; ///< Tablica pomocnicza dla złotego ruchu.
  uint64_t **copy_parent; ///< Tablica pomocnicza dla złotego ruchu.
  uint32_t **board; ///< Tablica dwuwymiarowa reprezentująca planszę.
};

gamma_t* gamma_new(uint32_t width, uint32_t height,
                   uint32_t players, uint32_t areas) {

  gamma_t *g = NULL;

  if ((width == 0 || height == 0) || (players == 0 || areas == 0)) {
    return g;
  }
  else {
    gamma_t *g = malloc(sizeof(gamma_t));
    if (g == NULL) {
      free(g);
      return NULL;
    }

    g->golden = malloc(players * sizeof(bool));
    if (g->golden == NULL) {
      free(g);
      return NULL;
    }

    g->fields_taken = malloc(players * sizeof(uint64_t));
    if (g->fields_taken == NULL) {
      free(g->golden);
      free(g);
      return NULL;
    }

    g->free_fields_around = malloc(players * sizeof(uint64_t));
    if (g->free_fields_around == NULL) {
      free(g->golden);
      free(g->fields_taken);
      free(g);
      return NULL;
    }

    g->areas_taken = malloc(players * sizeof(uint64_t));
    if (g->areas_taken == NULL) {
      free(g->golden);
      free(g->fields_taken);
      free(g->free_fields_around);
      free(g);
      return NULL;
    }

    g->board = malloc(width * sizeof(uint32_t *));
    if (g->board == NULL) {
      free(g->golden);
      free(g->fields_taken);
      free(g->free_fields_around);
      free(g->areas_taken);
      free(g);
      return NULL;
    }
    for (uint32_t i = 0; i < width; i++) {
      g->board[i] = malloc(height * sizeof(uint32_t));
      if (g->board[i] == NULL) {
        free(g->golden);
        free(g->fields_taken);
        free(g->free_fields_around);
        free(g->areas_taken);
        for (uint32_t k = 0; k < i; k++) {
          free(g->board[k]);
        }
        free(g->board);
        free(g);
        return NULL;
      }
    }

    g->rank = malloc(width * sizeof(uint64_t *));
    if (g->rank == NULL) {
      free(g->golden);
      free(g->fields_taken);
      free(g->free_fields_around);
      free(g->areas_taken);
      for (uint32_t k = 0; k < width; k++) {
        free(g->board[k]);
      }
      free(g->board);
      free(g);
      return NULL;
    }
    for (uint32_t i = 0; i < width; i++) {
      g->rank[i] = malloc(height * sizeof(uint64_t));
      if (g->rank[i] == NULL) {
        free(g->golden);
        free(g->fields_taken);
        free(g->free_fields_around);
        free(g->areas_taken);
        for (uint32_t k = 0; k < width; k++) {
          free(g->board[k]);
        }
        for (uint32_t k = 0; k < i; k++) {
          free(g->rank[k]);
        }
        free(g->board);
        free(g->rank);
        free(g);
        return NULL;
      }
    }

    g->parent = malloc(width * sizeof(uint64_t *));
    if (g->parent == NULL) {
      free(g->golden);
      free(g->fields_taken);
      free(g->free_fields_around);
      free(g->areas_taken);
      for (uint32_t k = 0; k < width; k++) {
        free(g->board[k]);
        free(g->rank[k]);
      }
      free(g->board);
      free(g->rank);
      free(g);
      return NULL;
    }
    for (uint32_t i = 0; i < width; i++) {
      g->parent[i] = malloc(height * sizeof(uint64_t));
      if (g->parent[i] == NULL) {
        free(g->golden);
        free(g->fields_taken);
        free(g->free_fields_around);
        free(g->areas_taken);
        for (uint32_t k = 0; k < i; k++) {
          free(g->parent[k]);
        }
        for (uint32_t k = 0; k < width; k++) {
          free(g->board[k]);
          free(g->rank[k]);
        }
        free(g->board);
        free(g->rank);
        free(g->parent);
        free(g);
        return NULL;
      }
    }

    g->copy_rank = malloc(width * sizeof(uint64_t *));
    if (g->copy_rank == NULL) {
        free(g->golden);
        free(g->fields_taken);
        free(g->free_fields_around);
        free(g->areas_taken);
        for (uint32_t k = 0; k < width; k++) {
          free(g->board[k]);
          free(g->rank[k]);
          free(g->parent[k]);
        }
        free(g->board);
        free(g->rank);
        free(g->parent);
        free(g);
        return NULL;
    }
    for (uint32_t i = 0; i < width; i++) {
      g->copy_rank[i] = malloc(height * sizeof(uint64_t));
      if (g->copy_rank[i] == NULL) {
        free(g->golden);
        free(g->fields_taken);
        free(g->free_fields_around);
        free(g->areas_taken);
        for (uint32_t k = 0; k < i; k++) {
          free(g->copy_rank[k]);
        }
        for (uint32_t k = 0; k < width; k++) {
          free(g->board[k]);
          free(g->rank[k]);
          free(g->parent[k]);
        }
        free(g->board);
        free(g->rank);
        free(g->parent);
        free(g->copy_rank);
        free(g);
        return NULL;
      }
    }

    g->copy_parent = malloc(width * sizeof(uint64_t *));
    if (g->copy_parent == NULL) {
        free(g->golden);
        free(g->fields_taken);
        free(g->free_fields_around);
        free(g->areas_taken);
        for (uint32_t k = 0; k < width; k++) {
          free(g->board[k]);
          free(g->rank[k]);
          free(g->parent[k]);
          free(g->copy_rank[k]);
        }
        free(g->board);
        free(g->rank);
        free(g->parent);
        free(g->copy_rank);
        free(g);
        return NULL;
    }
    for (uint32_t i = 0; i < width; i++) {
      g->copy_parent[i] = malloc(height * sizeof(uint64_t));
      if (g->copy_parent[i] == NULL) {
        free(g->golden);
        free(g->fields_taken);
        free(g->free_fields_around);
        free(g->areas_taken);
        for (uint32_t k = 0; k < i; k++) {
          free(g->copy_parent[k]);
        }
        for (uint32_t k = 0; k < width; k++) {
          free(g->board[k]);
          free(g->rank[k]);
          free(g->parent[k]);
          free(g->copy_rank[k]);
        }
        free(g->board);
        free(g->rank);
        free(g->parent);
        free(g->copy_rank);
        free(g->copy_parent);
        free(g);
        return NULL;
      }
    }

    g->width = width;
    g->height = height;
    g->players = players;
    g->areas = areas;
    g->free_fields = (width * height);

    for (uint32_t i = 0; i < players; i++) {
      g->golden[i] = 0;
      g->fields_taken[i] = 0;
      g->areas_taken[i] = 0;
      g->free_fields_around[i] = 0;
    }

    for (uint32_t j = 0; j < height; j++) {
      for (uint32_t i = 0; i < width; i++) {
        g->board[i][j] = 0;
        g->rank[i][j] = 0;
        g->parent[i][j] = i + (j * g->width);
        g->copy_rank[i][j] = 0;
        g->copy_parent[i][j] = 0;
      }
    }

    return g;
  }
}

void gamma_delete(gamma_t *g) {
  if (g != NULL) {
    free(g->golden);
    free(g->fields_taken);
    free(g->free_fields_around);
    free(g->areas_taken);
    for (uint32_t k = 0; k < g->width; k++) {
      free(g->board[k]);
      free(g->rank[k]);
      free(g->parent[k]);
      free(g->copy_rank[k]);
      free(g->copy_parent[k]);
    }
    free(g->board);
    free(g->rank);
    free(g->parent);
    free(g->copy_rank);
    free(g->copy_parent);
    free(g);
  }
}

/** @brief find z algorytmu Find & Union
 * @param[in] name - numer planszy liczony: i + j * width z board[i][j].
 * @param[in] g    – wskaźnik na strukturę przechowującą stan gry.
 * @return numer planszy do której dojdzie algorytm.
 */
static uint64_t Find (gamma_t *g, uint64_t name) {
  if (g->parent[name % (g->width)][name / (g->width)] == name) {
    return name;
  }

  g->parent[name % (g->width)][name / (g->width)] =
    Find(g, g->parent[name % (g->width)][name / (g->width)]);
  return g->parent[name % (g->width)][name / (g->width)];
}

/** @brief union z algorytmu Find & Union, łączy pola 1 i 2.
 * @param[in,out] g – wskaźnik na strukturę przechowującą stan gry.
 * @param[in] name1 - numer planszy liczony: i + j * width z board[i][j].
 * @param[in] name2 - numer planszy liczony: i + j * width z board[i][j].
 */
static void Union(gamma_t *g, uint64_t name1, uint64_t name2) {
  uint64_t name_1 = Find(g, name1);
  uint64_t name_2 = Find(g, name2);

  if (g->rank[name_1  % (g->width)][name_1 / (g->width)] >
    g->rank[name_2 % (g->width)][name_2 / (g->width)]) {

    g->parent[name_2 % (g->width)][name_2 / (g->width)] = name_1;
  }

  else if (g->rank[name_1  % (g->width)][name_1 / (g->width)] <
    g->rank[name_2 % (g->width)][name_2 / (g->width)]) {

    g->parent[name_1 % (g->width)][name_1 / (g->width)] = name_2;
  }

  else if (name_1 != name_2) {
    g->parent[name_2 % (g->width)][name_2 / (g->width)] = name_1;
    g->rank[name_1  % (g->width)][name_1 / (g->width)] =
      g->rank[name_1  % (g->width)][name_1 / (g->width)] + 1;
  }
}

/** @brief Unionuje pole [x][y] z sąsiadami jeśli są tego samego gracza.
 * Łączy pola tego samego gracza, jednocześnie aktualizując ilość obszarów.
 * Zakłada poprawność danych jako, że jest to funckja pomocnicza.
 * @param[in,out] g   – wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player  – numer gracza, liczba dodatnia niewiększa od wartości
 *                      @p players z funkcji @ref gamma_new,
 * @param[in] x       – numer kolumny, liczba nieujemna mniejsza od wartości
 *                      @p width z funkcji @ref gamma_new,
 * @param[in] y       – numer wiersza, liczba nieujemna mniejsza od wartości
 *                      @p height z funkcji @ref gamma_new.
 */
static void Union_helper(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
  if (x != 0) {
    if (g->board[x - 1][y] == player) {
      if (Find(g, x + (y * g->width)) != Find(g, x - 1 + (y * g->width))) {
        g->areas_taken[player - 1]--;
        Union(g, x + (y * g->width), x - 1 + (y * g->width));
      }
    }
  }

  if (y != 0) {
    if (g->board[x][y - 1] == player) {
      if (Find(g, x + (y * g->width)) != Find(g, x + ((y - 1) * g->width))) {
        g->areas_taken[player - 1]--;
        Union(g, x + (y * g->width), x + ((y - 1) * g->width));
      }
    }
  }

  if (x != (g->width - 1)) {
    if (g->board[x + 1][y] == player) {
      if (Find(g, x + (y * g->width)) != Find(g, x + 1 + (y * g->width))) {
        g->areas_taken[player - 1]--;
        Union(g, x + (y * g->width), x + 1 + (y * g->width));
      }
    }
  }

  if (y != (g->height - 1)) {
    if (g->board[x][y + 1] == player) {
      if (Find(g, x + (y * g->width)) != Find(g, x + ((y + 1) * g->width))) {
        g->areas_taken[player - 1]--;
        Union(g, x + (y * g->width), x + ((y + 1) * g->width));
      }
    }
  }
}

/** @brief zmiana wolnych pól po dodaniu pionka player na pole [i][j]
 * @param[in] g – wskaźnik na strukturę przechowującą stan gry.
 * @param[in] i - pierwsza współrzędna polożenia pionka.
 * @param[in] j - druga współrzędna polożenia pionka.
 * @param[in] player – numer gracza którego pionek stawiamy.
 * @param[in] is_golden – czy wykonujemy to dodanie złotym ruchem.
 * @return Ile trzeba dodać playerowi wolnych pól po postawieniu pionka.
 */
static int delta_free_fields_around(gamma_t *g,
  uint32_t i, uint32_t j, uint32_t player, bool is_golden) {

  int result = 0;
  bool less = 0;
  bool more_north = 1;
  bool more_west = 1;
  bool more_south = 1;
  bool more_east = 1;

  // sprawdzanie czy pole [i][j] przestało być wolnym polem
  if (i != 0) {
    if (g->board[i - 1][j] == player) {
      less = 1;
    }
  }

  if (j != 0) {
    if (g->board[i][j - 1] == player) {
      less = 1;
    }
  }

  if (i != (g->width - 1)) {
    if (g->board[i + 1][j] == player) {
      less = 1;
    }
  }

  if (j != (g->height - 1)) {
    if (g->board[i][j + 1] == player) {
      less = 1;
    }
  }
  
  if (is_golden == 0) {
    result = result - less;
  }

  // sprawdzanie czy wolnych pól przybyło, te cztery naokoło
  // na potrzeby tej funkcji [0][0] jest na północnym zachodzie
  // sprawdzanie czy pole na północ od [i][j] jest niesąsiadem playera
  if (j == 0) {
    more_north = 0;
  }
  else {
    if (g->board[i][j - 1] != 0) {
      more_north = 0;
    }
    else {
      if (j != 1) {
        if (g->board[i][j - 2] == player) {
          more_north = 0;
        }
      }

      if (i != 0) {
        if (g->board[i - 1][j - 1] == player) {
          more_north = 0;
        }
      }

      if (i != (g->width - 1)) {
        if (g->board[i + 1][j - 1] == player) {
          more_north = 0;
        }
      }
    }
  }
  // sprawdzanie czy pole na zachód od [i][j] jest niesąsiadem playera
  if (i == 0) {
    more_west = 0;
  }
  else {
    if (g->board[i - 1][j] != 0) {
      more_west = 0;
    }
    else {
      if (i != 1) {
        if (g->board[i - 2][j] == player) {
          more_west = 0;
        }
      }

      if (j != 0) {
        if (g->board[i - 1][j - 1] == player) {
          more_west = 0;
        }
      }

      if (j != (g->height - 1)) {
        if (g->board[i - 1][j + 1] == player) {
          more_west = 0;
        }
      }
    }
  }
  // sprawdzanie czy pole na południe od [i][j] jest niesąsiadem playera
  if (j == (g->height - 1)) {
    more_south = 0;
  }
  else {
    if (g->board[i][j + 1] != 0) {
      more_south = 0;
    }
    else {
      if (j != (g->height - 2)) {
        if (g->board[i][j + 2] == player) {
          more_south = 0;
        }
      }

      if (i != 0) {
        if (g->board[i - 1][j + 1] == player) {
          more_south = 0;
        }
      }

      if (i != (g->width - 1)) {
        if (g->board[i + 1][j + 1] == player) {
          more_south = 0;
        }
      }
    }
  }
  // sprawdzanie czy pole na wschód od [i][j] jest niesąsiadem playera
  if (i == (g->width - 1)) {
    more_east = 0;
  }
  else {
    if (g->board[i + 1][j] != 0) {
      more_east = 0;
    }
    else {
      if (i != (g->width - 2)) {
        if (g->board[i + 2][j] == player) {
          more_east = 0;
        }
      }

      if (j != 0) {
        if (g->board[i + 1][j - 1] == player) {
          more_east = 0;
        }
      }

      if (j != (g->height - 1)) {
        if (g->board[i + 1][j + 1] == player) {
          more_east = 0;
        }
      }
    }
  }
  result = result + more_north + more_west + more_south + more_east;
  return result;
}

bool gamma_move(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
  if ((g == NULL || player == 0) || ((x >= g->width) || (y >= g->height))) {
    return false;
  }
  else {
    if ((g->board[x][y] != 0) || (player > g->players)) {
      return false;
    }
    else {
      if (g->areas_taken[player - 1] == g->areas) {
        bool over_areas = 1;
        
        if (x != 0) {
          if (g->board[x - 1][y] == player) {
            over_areas = 0;
          }
        }

        if (y != 0) {
          if (g->board[x][y - 1] == player) {
            over_areas = 0;
          }
        }

        if (x != (g->width - 1)) {
          if (g->board[x + 1][y] == player) {
            over_areas = 0;
          }
        }

        if (y != (g->height - 1)) {
          if (g->board[x][y + 1] == player) {
            over_areas = 0;
          }
        }

        if (over_areas == 1) {
          return false;
        }
      }      

      g->fields_taken[player - 1]++;
      g->board[x][y] = player;
      g->areas_taken[player - 1]++;
      g->free_fields--;
      g->free_fields_around[player - 1] = g->free_fields_around[player - 1] +
        delta_free_fields_around(g, x, y, player, 0);

      uint32_t north_neighbor = 0;
      uint32_t west_neighbor = 0;
      uint32_t south_neighbor = 0;
      uint32_t east_neighbor = 0;

      if (y != 0) {
        if (g->board[x][y - 1] != 0 && g->board[x][y - 1] != player) {
          north_neighbor = g->board[x][y - 1];
        }
      }

      if (x != 0) {
        if (g->board[x - 1][y] != 0 && g->board[x - 1][y] != player) {
          west_neighbor = g->board[x - 1][y];
        }
      }

      if (y != (g->height - 1)) {
        if (g->board[x][y + 1] != 0 && g->board[x][y + 1] != player) {
          south_neighbor = g->board[x][y + 1];
        }
      }

      if (x != (g->width - 1)) {
        if (g->board[x + 1][y] != 0 && g->board[x + 1][y] != player) {
          east_neighbor = g->board[x + 1][y];
        }
      }

      // eliminating case of more than more same neighbor
      if (north_neighbor == west_neighbor) {
        west_neighbor = 0;
      }
      if (north_neighbor == south_neighbor) {
        south_neighbor = 0;
      }
      if (north_neighbor == east_neighbor) {
        east_neighbor = 0;
      }
      if (west_neighbor == south_neighbor) {
        south_neighbor = 0;
      }
      if (west_neighbor == east_neighbor) {
        east_neighbor = 0;
      }
      if (south_neighbor == east_neighbor) {
        east_neighbor = 0;
      }

      if (north_neighbor != 0) {
        g->free_fields_around[north_neighbor - 1]--;
      }
      if (west_neighbor != 0) {
        g->free_fields_around[west_neighbor - 1]--;
      }
      if (south_neighbor != 0) {
        g->free_fields_around[south_neighbor - 1]--;
      }
      if (east_neighbor != 0) {
        g->free_fields_around[east_neighbor - 1]--;
      }

      Union_helper(g, player, x, y);

      return true;
    }
  }
}

bool gamma_golden_move(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
  if ((g == NULL || player == 0) || ((x >= g->width) || (y >= g->height))) {
    return false;
  }
  else {
    if ((g->board[x][y] == 0) || (player > g->players)) {
      return false;
    }
    else {
      if ((g->golden[player - 1] == 1) || (g->board[x][y] == player)) {
        return false;
      }
      else {
        if (g->areas_taken[player - 1] == g->areas) {
          bool specific_case = 1;

          if (x != 0) {
            if (g->board[x - 1][y] == player) {
              specific_case = 0;
            }
          }

          if (y != 0) {
            if (g->board[x][y - 1] == player) {
              specific_case = 0;
            }
          }

          if (x != (g->width - 1)) {
            if (g->board[x + 1][y] == player) {
              specific_case = 0;
            }
          }

          if (y != (g->height - 1)) {
            if (g->board[x][y + 1] == player) {
              specific_case = 0;
            }
          }
  
          if (specific_case == 1) {
            return false;
          }
        }

        uint32_t robbed_player = g->board[x][y];
        uint64_t copy_areas_taken_player = g->areas_taken[player - 1];
        uint64_t copy_areas_taken_robbed_player =
          g->areas_taken[robbed_player - 1];
        g->board[x][y] = player;
        g->areas_taken[player - 1] = g->fields_taken[player - 1] + 1;
        g->areas_taken[robbed_player - 1] =
          g->fields_taken[robbed_player - 1] - 1;

        for (uint32_t i = 0; i < g->width; i++) {
          for (uint32_t j = 0; j < g->height; j++) {
            g->copy_rank[i][j] = g->rank[i][j];
            g->copy_parent[i][j] = g->parent[i][j];

            if(g->board[i][j] == player) {
              g->parent[i][j] = i + (j * g->width);
              g->rank[i][j] = 0;
            }

            if(g->board[i][j] == robbed_player) {
              g->parent[i][j] = i + (j * g->width);
              g->rank[i][j] = 0;
            }
          }
        }

        for (uint32_t i = 0; i < g->width; i++) {
          for(uint32_t j = 0; j < g->height; j++) {
            if(g->board[i][j] == player) {
              Union_helper(g, player, i, j);
            }

            if(g->board[i][j] == robbed_player) {
              Union_helper(g, robbed_player, i, j);
            }
          }
        }

        if ((g->areas_taken[player - 1] > g->areas) ||
          (g->areas_taken[robbed_player - 1] > g->areas)) {

          for (uint32_t i = 0; i < g->width; i++) {
            for (uint32_t j = 0; j < g->height; j++) {
              g->rank[i][j] = g->copy_rank[i][j];
              g->parent[i][j] = g->copy_parent[i][j];
            }
          }

          g->areas_taken[player - 1] = copy_areas_taken_player;
          g->areas_taken[robbed_player - 1] = copy_areas_taken_robbed_player;
          g->board[x][y] = robbed_player;
          return false;
        }
        else {
          g->golden[player - 1] = 1;
          g->fields_taken[player - 1]++;
          g->fields_taken[robbed_player - 1]--;
          g->free_fields_around[robbed_player - 1] =
            g->free_fields_around[robbed_player - 1] -
            delta_free_fields_around(g, x, y, robbed_player, 1);
          g->free_fields_around[player - 1] =
            g->free_fields_around[player - 1] +
            delta_free_fields_around(g, x, y, player, 1);
          return true;
        }
      }
    }
  }
}

uint64_t gamma_busy_fields(gamma_t *g, uint32_t player) {
  if (g != NULL) {
    if (player > 0 && player <= g->players) {  
      return g->fields_taken[player - 1];
    }
    else {
      return 0;
    }
  }
  else {
    return 0;
  }
}

uint64_t gamma_free_fields(gamma_t *g, uint32_t player) {
  if (g != NULL) {
    if (player > 0 && player <= g->players)
      if (g->areas_taken[player - 1] < g->areas) {
        return g->free_fields;
      }
      else {
        return g->free_fields_around[player - 1];
      }
    else {
      return 0;
    }
  }
  else {
    return 0;
  }
}

bool gamma_golden_possible(gamma_t *g, uint32_t player) {
  if ((g == NULL || player == 0) || (player > g->players)) {
    return false;
  }
  else {
    // sprawdzanie całej planszy
    for (uint32_t x = 0; x < g->width; x++) {
      for (uint32_t y = 0; y < g->height; y++) {
        if (g->board[x][y] != 0) {
          if ((g->golden[player - 1] != 1) && (g->board[x][y] != player)) {
            bool go_next = false;

            if (g->areas_taken[player - 1] == g->areas) {
              bool specific_case = 1;

              if (x != 0) {
                if (g->board[x - 1][y] == player) {
                  specific_case = 0;
                }
              }

              if (y != 0) {
                if (g->board[x][y - 1] == player) {
                  specific_case = 0;
                }
              }

              if (x != (g->width - 1)) {
                if (g->board[x + 1][y] == player) {
                  specific_case = 0;
                }
              }

              if (y != (g->height - 1)) {
                if (g->board[x][y + 1] == player) {
                  specific_case = 0;
                }
              }
  
              if (specific_case == 1) {
                go_next = true;
              }
            }

            if (go_next == false) {
              uint32_t robbed_player = g->board[x][y];
              uint64_t copy_areas_taken_player = g->areas_taken[player - 1];
              uint64_t copy_areas_taken_robbed_player =
                g->areas_taken[robbed_player - 1];
              g->board[x][y] = player;
              g->areas_taken[player - 1] = g->fields_taken[player - 1] + 1;
              g->areas_taken[robbed_player - 1] =
                g->fields_taken[robbed_player - 1] - 1;

              for (uint32_t i = 0; i < g->width; i++) {
                for (uint32_t j = 0; j < g->height; j++) {
                  g->copy_rank[i][j] = g->rank[i][j];
                  g->copy_parent[i][j] = g->parent[i][j];

                  if(g->board[i][j] == player) {
                    g->parent[i][j] = i + (j * g->width);
                    g->rank[i][j] = 0;
                  }

                  if(g->board[i][j] == robbed_player) {
                    g->parent[i][j] = i + (j * g->width);
                    g->rank[i][j] = 0;
                  }
                }
              }

              for (uint32_t i = 0; i < g->width; i++) {
                for(uint32_t j = 0; j < g->height; j++) {
                  if(g->board[i][j] == player) {
                    Union_helper(g, player, i, j);
                  }

                  if(g->board[i][j] == robbed_player) {
                    Union_helper(g, robbed_player, i, j);
                  }
                }
              }

              if ((g->areas_taken[player - 1] > g->areas) ||
                (g->areas_taken[robbed_player - 1] > g->areas)) {

                for (uint32_t i = 0; i < g->width; i++) {
                  for (uint32_t j = 0; j < g->height; j++) {
                    g->rank[i][j] = g->copy_rank[i][j];
                    g->parent[i][j] = g->copy_parent[i][j];
                  }
                }

                g->areas_taken[player - 1] = copy_areas_taken_player;
                g->areas_taken[robbed_player - 1] =
                  copy_areas_taken_robbed_player;
                g->board[x][y] = robbed_player;
              }
              else {
                for (uint32_t i = 0; i < g->width; i++) {
                  for (uint32_t j = 0; j < g->height; j++) {
                    g->rank[i][j] = g->copy_rank[i][j];
                    g->parent[i][j] = g->copy_parent[i][j];
                  }
                }

                g->areas_taken[player - 1] = copy_areas_taken_player;
                g->areas_taken[robbed_player - 1] =
                  copy_areas_taken_robbed_player;
                g->board[x][y] = robbed_player;
                return true;
              }
            }
          }
        }
      }
    }
    return false;
  }
}

uint32_t number_of_digits(uint32_t number) {
  if (number < 10) {
    return 1;
  }
  else {
    return (1 + number_of_digits(number/10));
  }
}

char* gamma_board(gamma_t *g) {
  if (g != NULL) {
    uint64_t width_tmp = g->width;
    uint64_t size;
    if (g->players < 10) {
      size = (width_tmp * g->height  + g->height + 1);
    }
    else {
      size = (width_tmp * g->height * (number_of_digits(g->players) + 1) +
      g->height + 1);
    }

    char* bufor = malloc(sizeof(char) * size);
    if (bufor == NULL) {
      return NULL;
    }
    else {
      uint64_t number_of_chars = 0;
      if (g->players < 10) {
        // w pętli j "powiększone" o 1 by się ona skończyła
        for (uint32_t j = g->height; j >= 1; j--) {
          for (uint32_t i = 0; i < g->width; i++) {
            if (g->board[i][j - 1] == 0) {
              bufor[number_of_chars] = '.';
              number_of_chars++;
            }
            else {
              bufor[number_of_chars] = g->board[i][j - 1] + '0';
              number_of_chars++;
            }
          }
          bufor[number_of_chars] = '\n';
          number_of_chars++;
        }
      }
      else {
        uint32_t length = number_of_digits(g->players);

        // w pętli j "powiększone" o 1 by się ona skończyła
        for (uint32_t j = g->height; j >= 1; j--) {
          for (uint32_t i = 0; i < g->width; i++) {
            if (g->board[i][j - 1] == 0) {
              for (uint32_t k = 0; k < (length - 1); k++) {
                bufor[number_of_chars] = ' ';
                number_of_chars++;
              }

              bufor[number_of_chars] = '.';
              number_of_chars++;
              bufor[number_of_chars] = ' ';
              number_of_chars++;
            }
            else {
              if (g->board[i][j - 1] < 10) {
                for (uint32_t k = 0; k < (length - 1); k++) {
                  bufor[number_of_chars] = ' ';
                  number_of_chars++;
                }

                bufor[number_of_chars] = g->board[i][j - 1] + '0';
                number_of_chars++;
                bufor[number_of_chars] = ' ';
                number_of_chars++;
              }
              else {
                uint32_t number_of_whitespaces =
                  length - number_of_digits(g->board[i][j - 1]) + 1;
                for (uint32_t k = 0; k < (number_of_whitespaces - 1); k++) {
                  bufor[number_of_chars] = ' ';
                  number_of_chars++;
                }

                uint32_t number = g->board[i][j - 1];
                // w pętli k "powiększone" o 1 by się ona skończyła
                for (uint64_t k = number_of_chars +
                  number_of_digits(g->board[i][j - 1]);
                  k > number_of_chars; k--) {

                  bufor[k - 1] = (number % 10) + '0';
                  number = (number - (number % 10))/10;
                }
                number_of_chars =
                  number_of_chars + number_of_digits(g->board[i][j - 1]);

                bufor[number_of_chars] = ' ';
                number_of_chars++;
              }
            }
          }
          bufor[number_of_chars] = '\n';
          number_of_chars++;
        }
      }
      bufor[number_of_chars] = '\0';
      number_of_chars++;

      return bufor;
    }
  }
  else {
    return NULL;
  }
}

uint32_t return_players(gamma_t *g) {
  return g->players;
}

uint32_t return_width(gamma_t *g) {
  return g->width;
}

uint32_t return_height(gamma_t *g) {
  return g->height;
}

uint64_t return_fields_taken(gamma_t *g, uint32_t player) {
  return g->fields_taken[player - 1];
}

uint64_t return_free_fields_around(gamma_t *g, uint32_t player) {
  return g->free_fields_around[player - 1];
}
