#include <iostream>
#include <cstdlib>
#include <ctime>
#include <conio.h>
#include <windows.h>
#include <cctype>
#include <cstring>

#define NOMINMAX

using namespace std;

#define ANCHO_PISTA 200
#define ALTO_PISTA 10
#define META 195
#define NUM_OBSTACULOS 60
#define NUM_ITEMS 30
#define MAX_BOTS 3
#define INMUNIDAD_DURACION 900
#define MENSAJES_Y_OFFSET (ALTO_PISTA + 8)
#define LINEA_ITEM_JUGADOR (ALTO_PISTA + 8 + 2)
#define LINEA_ITEM_BOT (ALTO_PISTA + 8 + 3)

enum ItemType {
    ITEM_INMUNIDAD,
    ITEM_TELETRANSPORTE,
    ITEM_TRAMPA,
    NUM_ITEM_TIPOS
};

const char* nombre_item(ItemType tipo) {
    switch (tipo) {
        case ITEM_INMUNIDAD: return "INMUNIDAD";
        case ITEM_TELETRANSPORTE: return "TELETRANSPORTE";
        case ITEM_TRAMPA: return "TRAMPA";
        default: return "DESCONOCIDO";
    }
}

int victorias_j1 = 0, derrotas_j1 = 0, empates = 0;
int victorias_j2 = 0, derrotas_j2 = 0;
int victorias_jugador_un_jugador = 0, derrotas_jugador_un_jugador = 0;

int g_pos_x_player1, g_pos_y_player1;
int g_pos_x_player2, g_pos_y_player2;
int g_pos_x_bots[MAX_BOTS], g_pos_y_bots[MAX_BOTS];
int g_bot_estado[MAX_BOTS];
int g_num_active_bots;

int g_obstaculo_x[NUM_OBSTACULOS], g_obstaculo_y[NUM_OBSTACULOS];
int g_item_x[NUM_ITEMS], g_item_y[NUM_ITEMS];
int g_item_recogido[NUM_ITEMS];

int g_inmunidad_player1 = 0;
int g_inmunidad_player2 = 0;
int g_inmunidad_bots[MAX_BOTS];

void gotoxy(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void limpiarPantalla() {
    system("cls");
}

int min2(int a, int b) { return (a < b) ? a : b; }
int max2(int a, int b) { return (a > b) ? a : b; }

void dibujarCaracter(int x, int y, const char* str, int color, int old_x, int old_y, const char* old_str) {
    if (old_x != -1 && old_y != -1) {
        gotoxy(old_x + 1, old_y + 2);
        setColor(10);
        for (size_t i = 0; i < strlen(old_str); i++) printf(" ");
    }
    gotoxy(x + 1, y + 2);
    setColor(color);
    printf("%s", str);
}

void dibujarPistaBase() {
    setColor(2);
    gotoxy(0, 1);
    printf("+");
    for (int i = 0; i < ANCHO_PISTA; i++) printf("-");
    printf("+\n");
    for (int j = 0; j < ALTO_PISTA; j++) {
        gotoxy(0, j + 2);
        printf("|");
        for (int i = 0; i < ANCHO_PISTA; i++) {
            int dibujado = 0;
            if (i == META) {
                setColor(6);
                printf("|");
                dibujado = 1;
            }
            if (!dibujado) {
                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                    if (g_obstaculo_x[k] == i && g_obstaculo_y[k] == j) {
                        setColor(12);
                        printf("X");
                        dibujado = 1;
                        break;
                    }
                }
            }
            if (!dibujado) {
                for (int k = 0; k < NUM_ITEMS; k++) {
                    if (g_item_recogido[k] == 0 && g_item_x[k] == i && g_item_y[k] == j) {
                        setColor(11);
                        printf("O");
                        dibujado = 1;
                        break;
                    }
                }
            }
            if (!dibujado) {
                setColor(10);
                printf(" ");
            }
        }
        printf("|\n");
    }
    setColor(2);
    gotoxy(0, ALTO_PISTA + 2);
    printf("+");
    for (int i = 0; i < ANCHO_PISTA; i++) printf("-");
    printf("+\n");
    setColor(6);
    for (int j = 0; j < ALTO_PISTA; j++) {
        gotoxy(META + 1, j + 2);
        printf("|");
    }
    gotoxy(META - 2, 1);
    setColor(6);
    printf("=====");
    gotoxy(META - 2, ALTO_PISTA + 2);
    printf("=====");
    setColor(15);
    gotoxy(META - 3, ALTO_PISTA + 3);
    printf("META");
}

void organizarObstaculosYItems() {
    memset(g_obstaculo_x, 0, sizeof(g_obstaculo_x));
    memset(g_obstaculo_y, 0, sizeof(g_obstaculo_y));
    memset(g_item_x, 0, sizeof(g_item_x));
    memset(g_item_y, 0, sizeof(g_item_y));
    memset(g_item_recogido, 0, sizeof(g_item_recogido));
    int idx = 0;
    while (idx < NUM_OBSTACULOS) {
        int x = rand() % (META - 5) + 5;
        int y = rand() % ALTO_PISTA;
        int solapa = 0;
        for (int k = 0; k < idx; k++) {
            if (g_obstaculo_x[k] == x && g_obstaculo_y[k] == y) {
                solapa = 1;
                break;
            }
        }
        if (!solapa) {
            g_obstaculo_x[idx] = x;
            g_obstaculo_y[idx] = y;
            idx++;
        }
    }
    idx = 0;
    while (idx < NUM_ITEMS) {
        int x = rand() % (META - 5) + 5;
        int y = rand() % ALTO_PISTA;
        int solapa = 0;
        for (int k = 0; k < NUM_OBSTACULOS; k++) {
            if (g_obstaculo_x[k] == x && g_obstaculo_y[k] == y) {
                solapa = 1;
                break;
            }
        }
        for (int k = 0; k < idx; k++) {
            if (g_item_x[k] == x && g_item_y[k] == y) {
                solapa = 1;
                break;
            }
        }
        if (!solapa) {
            g_item_x[idx] = x;
            g_item_y[idx] = y;
            g_item_recogido[idx] = 0;
            idx++;
        }
    }
}

void mostrarMarcadores(bool dos_jugadores) {
    setColor(14);
    gotoxy(0, ALTO_PISTA + 3);
    if (dos_jugadores) {
        printf("Marcadores (2 Jugadores):         \n");
        printf("Jugador 1 -> Victorias: %d | Derrotas: %d         \n", victorias_j1, derrotas_j1);
        printf("Jugador 2 -> Victorias: %d | Derrotas: %d         \n", victorias_j2, derrotas_j2);
        printf("Empates: %d         \n", empates);
        printf("Jugador 1: [W] Arriba, [S] Abajo, [D] Derecha, [A] Retroceder, [Q] Usar item        \n");
        printf("Jugador 2: [I] Arriba, [K] Abajo, [L] Derecha, [J] Retroceder, [O] Usar item        \n");
        printf("Presiona 'P' para salir o 'R' para reiniciar        \n");
    } else {
        printf("Marcadores (1 Jugador):         \n");
        printf("Jugador -> Victorias: %d | Derrotas: %d         \n", victorias_jugador_un_jugador, derrotas_jugador_un_jugador);
        printf("Jugador: [W] Arriba, [S] Abajo, [D] Derecha, [A] Retroceder, [Q] Usar item        \n");
        printf("Presiona 'P' para salir o 'R' para reiniciar        \n");
    }
}

void mostrarControlesJugador2() {
    setColor(14);
    gotoxy(0, ALTO_PISTA + 10);
    printf("Controles Jugador 2:\n");
    printf("[I] Arriba\n");
    printf("[K] Abajo\n");
    printf("[L] Derecha\n");
    printf("[J] Retroceder\n");
    printf("[O] Usar item\n");
}

void mostrarItemPortado(int jugador, int tipo_item, bool en_uso) {
    gotoxy(0, LINEA_ITEM_JUGADOR + jugador);
    setColor(15);
    if (tipo_item == -1) {
        printf("Jugador %d - Item portado: Ninguno.                                   ", jugador + 1);
    } else {
        if (en_uso)
            printf("Jugador %d - Item portado: %s (EN USO)                                ", jugador + 1, nombre_item((ItemType)tipo_item));
        else
            printf("Jugador %d - Item portado: %s                                         ", jugador + 1, nombre_item((ItemType)tipo_item));
    }
}

void mostrarItemPortadoBot(int bot, int tipo_item, bool en_uso) {
    gotoxy(0, LINEA_ITEM_BOT + bot);
    setColor(15);
    if (tipo_item == -1) {
        printf("Bot %d - Item portado: Ninguno.                                      ", bot + 1);
    } else {
        if (en_uso)
            printf("Bot %d - Item portado: %s (EN USO)                                   ", bot + 1, nombre_item((ItemType)tipo_item));
        else
            printf("Bot %d - Item portado: %s                                            ", bot + 1, nombre_item((ItemType)tipo_item));
    }
}

struct Llegada {
    char nombre[32];
    double tiempo;
};
void mostrarOrdenLlegada(Llegada* llegadas, int total) {
    printf("\n\nORDEN DE LLEGADA:\n");
    for (int i = 0; i < total; ++i) {
        printf("%d. %s (%.2f seg)\n", i + 1, llegadas[i].nombre, llegadas[i].tiempo);
    }
    printf("\n");
}

// --- FUNCIONES DE ITEMS (RECOGER Y USAR) ---

void recogerItem(int jugador, int* item_portado, bool* en_uso, ItemType efecto, char tecla_uso) {
    gotoxy(0, MENSAJES_Y_OFFSET);
    setColor(15);
    if (*item_portado == -1) {
        *item_portado = efecto;
        *en_uso = false;
        printf("Has recogido un item: %s. Ahora puedes usarlo con '%c'.               \n", nombre_item(efecto), tecla_uso);
    } else {
        printf("Ya tienes un item. Usa '%c' para gastarlo antes de recoger otro.      \n", tecla_uso);
    }
    mostrarItemPortado(jugador, *item_portado, *en_uso);
    Sleep(700);
    gotoxy(0, MENSAJES_Y_OFFSET);
    printf("                                                                      ");
}

void recogerItemBot(int bot, int* item_portado, bool* en_uso, ItemType efecto) {
    gotoxy(0, MENSAJES_Y_OFFSET + 1);
    setColor(15);
    if (*item_portado == -1) {
        *item_portado = efecto;
        *en_uso = false;
        printf("Bot %d ha recogido un item: %s.                                      \n", bot + 1, nombre_item(efecto));
    } else {
        printf("Bot %d ya tiene un item.                                             \n", bot + 1);
    }
    mostrarItemPortadoBot(bot, *item_portado, *en_uso);
    Sleep(500);
    gotoxy(0, MENSAJES_Y_OFFSET + 1);
    printf("                                                                      ");
}

void usarItemPortado(int* pos_x, int* pos_y, int* inmunidad, int* item_portado, bool* en_uso, int jugador, int num_players_bots, int* pos_x_otro = NULL, int* pos_y_otro = NULL) {
    if (*item_portado == -1 || *en_uso) return;
    *en_uso = true;
    mostrarItemPortado(jugador, *item_portado, true);

    switch (*item_portado) {
        case ITEM_INMUNIDAD:
            *inmunidad = INMUNIDAD_DURACION;
            gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
            setColor(14);
            printf("INMUNIDAD activada! (30 segundos)                                  ");
            Sleep(1000);
            gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
            printf("                                                                      ");
            break;
        case ITEM_TELETRANSPORTE: {
            int avance = rand() % 20 + 10;
            *pos_x = min2(*pos_x + avance, META);
            gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
            setColor(14);
            printf("TELETRANSPORTE usado! Avanzas %d posiciones.                         ", avance);
            Sleep(1000);
            gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
            printf("                                                                      ");
            break;
        }
        case ITEM_TRAMPA: {
            int retroceso = rand() % 10 + 5;
            if (num_players_bots == 2 && pos_x_otro != NULL) {
                *pos_x_otro = max2(*pos_x_otro - retroceso, 0);
                gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
                setColor(12);
                printf("TRAMPA usada! El oponente retrocede %d posiciones.                   ", retroceso);
                Sleep(1000);
                gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
                printf("                                                                      ");
            }
            break;
        }
    }
    *item_portado = -1;
    *en_uso = false;
    mostrarItemPortado(jugador, -1, false);
}

void usarItemPortadoBot(int* pos_x, int* pos_y, int* inmunidad, int* item_portado, bool* en_uso, int bot, int num_players_bots, int* pos_x_otro = NULL, int* pos_y_otro = NULL) {
    if (*item_portado == -1 || *en_uso) return;
    *en_uso = true;
    mostrarItemPortadoBot(bot, *item_portado, true);

    switch (*item_portado) {
        case ITEM_INMUNIDAD:
            *inmunidad = INMUNIDAD_DURACION;
            break;
        case ITEM_TELETRANSPORTE: {
            int avance = rand() % 20 + 10;
            *pos_x = min2(*pos_x + avance, META);
            break;
        }
        case ITEM_TRAMPA: {
            int retroceso = rand() % 10 + 5;
            if (num_players_bots == 1 && pos_x_otro != NULL) {
                *pos_x_otro = max2(*pos_x_otro - retroceso, 0);
            }
            break;
        }
    }
    *item_portado = -1;
    *en_uso = false;
    mostrarItemPortadoBot(bot, -1, false);
}

void aplicarEfectoItem(int item_idx, int player_id, int bot_id, int num_players_bots) {
    if (g_item_recogido[item_idx] == 0) {
        g_item_recogido[item_idx] = 1;
        dibujarCaracter(-1, -1, " ", 0, g_item_x[item_idx], g_item_y[item_idx], "O");
        ItemType efecto = (ItemType)(rand() % NUM_ITEM_TIPOS);
        if (player_id == 1) {
            recogerItem(0, &g_inmunidad_player1, (bool*)&g_inmunidad_player1, efecto, 'Q');
        } else if (player_id == 2) {
            recogerItem(1, &g_inmunidad_player2, (bool*)&g_inmunidad_player2, efecto, 'O');
        } else if (bot_id != -1) {
            recogerItemBot(bot_id, &g_inmunidad_bots[bot_id], (bool*)&g_inmunidad_bots[bot_id], efecto);
        }
    }
}

// --- JUEGO DOS JUGADORES ---

void iniciarJuegoDosJugadores() {
    int k;
    char opcion = 'r';
    clock_t start_time;
    int old_pos_x1, old_pos_y1, old_pos_x2, old_pos_y2;
    Llegada llegadas[2 + MAX_BOTS];
    int total_llegadas = 0;
    int incluir_bots = -1, num_bots = 0;
    do {
        limpiarPantalla();
        printf("--- Modo Dos Jugadores ---\n");
        printf("¿Quieres incluir bots en la carrera? (s/n): ");
        char resp[8];
        fgets(resp, sizeof(resp), stdin);
        if (tolower((unsigned char)resp[0]) == 's') {
            incluir_bots = 1;
            printf("¿Cuántos bots quieres? (1-%d): ", MAX_BOTS);
            scanf("%d", &num_bots);
            while (num_bots < 1 || num_bots > MAX_BOTS) {
                printf("Número de bots inválido. Por favor, introduce un valor entre 1 y %d: ", MAX_BOTS);
                while (getchar() != '\n');
                scanf("%d", &num_bots);
            }
            while (getchar() != '\n');
        } else if (tolower((unsigned char)resp[0]) == 'n') {
            incluir_bots = 0;
            num_bots = 0;
        } else {
            printf("Respuesta inválida. Por favor, responde 's' o 'n'.\n");
            Sleep(1000);
        }
    } while (incluir_bots == -1);

    while (opcion == 'r') {
        limpiarPantalla();
        g_pos_x_player1 = 0;
        g_pos_y_player1 = ALTO_PISTA / 2;
        g_pos_x_player2 = 0;
        g_pos_y_player2 = ALTO_PISTA / 2 + 1;
        g_inmunidad_player1 = 0;
        g_inmunidad_player2 = 0;
        total_llegadas = 0;
        for (int l = 0; l < 2 + MAX_BOTS; ++l) llegadas[l].nombre[0] = '\0';
        srand((unsigned int)time(NULL));
        start_time = clock();
        organizarObstaculosYItems();
        dibujarPistaBase();
        mostrarMarcadores(true);
        mostrarControlesJugador2();
        old_pos_x1 = g_pos_x_player1; old_pos_y1 = g_pos_y_player1;
        old_pos_x2 = g_pos_x_player2; old_pos_y2 = g_pos_y_player2;
        int g_pos_x_bots_local[MAX_BOTS], g_pos_y_bots_local[MAX_BOTS];
        int g_inmunidad_bots_local[MAX_BOTS];
        int g_bot_estado_local[MAX_BOTS];
        for (int b = 0; b < MAX_BOTS; b++) {
            g_pos_x_bots_local[b] = 0;
            g_pos_y_bots_local[b] = (ALTO_PISTA / ((num_bots > 0 ? num_bots : 1) + 2)) * (b + 2);
            g_inmunidad_bots_local[b] = 0;
            g_bot_estado_local[b] = 0;
        }
        bool player1_llego_meta = false, player2_llego_meta = false, bot_llego_meta[MAX_BOTS] = {false};
        while (1) {
            gotoxy(0, 0);
            setColor(10);
            printf("Tiempo jugado: %.2f segundos", (double)(clock() - start_time) / CLOCKS_PER_SEC);
            if (g_inmunidad_player1 > 0) g_inmunidad_player1--;
            if (g_inmunidad_player2 > 0) g_inmunidad_player2--;
            for (int b = 0; b < MAX_BOTS; b++) if (g_inmunidad_bots_local[b] > 0) g_inmunidad_bots_local[b]--;
            old_pos_x1 = g_pos_x_player1; old_pos_y1 = g_pos_y_player1;
            old_pos_x2 = g_pos_x_player2; old_pos_y2 = g_pos_y_player2;
            if (!player1_llego_meta && !player2_llego_meta && g_pos_x_player1 == g_pos_x_player2 && g_pos_y_player1 == g_pos_y_player2) {
                limpiarPantalla();
                printf("Choque entre jugadores. Empate, fin del juego.\n");
                empates++;
                goto fin_juego_dos_jugadores;
            }
            if (!player1_llego_meta) {
                if (GetAsyncKeyState('D') & 0x8000) g_pos_x_player1++;
                if (GetAsyncKeyState('A') & 0x8000 && g_pos_x_player1 > 0) g_pos_x_player1--;
                if (GetAsyncKeyState('W') & 0x8000 && g_pos_y_player1 > 0) g_pos_y_player1--;
                if (GetAsyncKeyState('S') & 0x8000 && g_pos_y_player1 < ALTO_PISTA - 1) g_pos_y_player1++;
            }
            if (!player2_llego_meta) {
                if (GetAsyncKeyState('L') & 0x8000) g_pos_x_player2++;
                if (GetAsyncKeyState('J') & 0x8000 && g_pos_x_player2 > 0) g_pos_x_player2--;
                if (GetAsyncKeyState('I') & 0x8000 && g_pos_y_player2 > 0) g_pos_y_player2--;
                if (GetAsyncKeyState('K') & 0x8000 && g_pos_y_player2 < ALTO_PISTA - 1) g_pos_y_player2++;
            }
            if (!player1_llego_meta && !player2_llego_meta && g_pos_x_player1 == g_pos_x_player2 && g_pos_y_player1 == g_pos_y_player2) {
                limpiarPantalla();
                printf("Choque entre jugadores. Empate, fin del juego.\n");
                empates++;
                goto fin_juego_dos_jugadores;
            }
            if (!player1_llego_meta)
                dibujarCaracter(g_pos_x_player1, g_pos_y_player1, "[o1]", (g_inmunidad_player1 > 0 ? 14 : 9), old_pos_x1, old_pos_y1, "[o1]");
            else
                dibujarCaracter(old_pos_x1, old_pos_y1, "    ", 0, old_pos_x1, old_pos_y1, "[o1]");
            if (!player2_llego_meta)
                dibujarCaracter(g_pos_x_player2, g_pos_y_player2, "[o2]", (g_inmunidad_player2 > 0 ? 14 : 13), old_pos_x2, old_pos_y2, "[o2]");
            else
                dibujarCaracter(old_pos_x2, old_pos_y2, "    ", 0, old_pos_x2, old_pos_y2, "[o2]");
            int old_pos_x_bots[MAX_BOTS], old_pos_y_bots[MAX_BOTS];
            for (int b = 0; b < MAX_BOTS; b++) {
                old_pos_x_bots[b] = g_pos_x_bots_local[b];
                old_pos_y_bots[b] = g_pos_y_bots_local[b];
            }
            if (incluir_bots && num_bots > 0) {
                for (int b = 0; b < num_bots; b++) {
                    if (g_bot_estado_local[b] == 0 && !bot_llego_meta[b]) {
                        int next_x = g_pos_x_bots_local[b] + 1;
                        int next_y = g_pos_y_bots_local[b];
                        bool hay_obstaculo = false;
                        for (int k = 0; k < NUM_OBSTACULOS; k++) {
                            if (g_obstaculo_x[k] == next_x && g_obstaculo_y[k] == next_y) {
                                hay_obstaculo = true;
                                break;
                            }
                        }
                        if (!hay_obstaculo) {
                            g_pos_x_bots_local[b]++;
                        } else {
                            if (g_pos_y_bots_local[b] > 0) {
                                bool libre_arriba = true;
                                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                    if (g_obstaculo_x[k] == g_pos_x_bots_local[b] && g_obstaculo_y[k] == g_pos_y_bots_local[b] - 1) {
                                        libre_arriba = false;
                                        break;
                                    }
                                }
                                if (libre_arriba) g_pos_y_bots_local[b]--;
                                else if (g_pos_y_bots_local[b] < ALTO_PISTA - 1) {
                                    bool libre_abajo = true;
                                    for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                        if (g_obstaculo_x[k] == g_pos_x_bots_local[b] && g_obstaculo_y[k] == g_pos_y_bots_local[b] + 1) {
                                            libre_abajo = false;
                                            break;
                                        }
                                    }
                                    if (libre_abajo) g_pos_y_bots_local[b]++;
                                }
                            } else if (g_pos_y_bots_local[b] < ALTO_PISTA - 1) {
                                bool libre_abajo = true;
                                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                    if (g_obstaculo_x[k] == g_pos_x_bots_local[b] && g_obstaculo_y[k] == g_pos_y_bots_local[b] + 1) {
                                        libre_abajo = false;
                                        break;
                                    }
                                }
                                if (libre_abajo) g_pos_y_bots_local[b]++;
                            }
                        }
                        if (rand() % 100 < 20) {
                            if (rand() % 2 == 0 && g_pos_y_bots_local[b] > 0) g_pos_y_bots_local[b]--;
                            else if (g_pos_y_bots_local[b] < ALTO_PISTA - 1) g_pos_y_bots_local[b]++;
                        }
                        char bot_str[8];
                        sprintf(bot_str, "[B%d]", b + 1);
                        dibujarCaracter(g_pos_x_bots_local[b], g_pos_y_bots_local[b], bot_str, (g_inmunidad_bots_local[b] > 0 ? 14 : 11 + b), old_pos_x_bots[b], old_pos_y_bots[b], bot_str);
                        for (k = 0; k < NUM_OBSTACULOS; k++) {
                            if (g_pos_x_bots_local[b] == g_obstaculo_x[k] && g_pos_y_bots_local[b] == g_obstaculo_y[k]) {
                                if (g_inmunidad_bots_local[b] > 0) {
                                    gotoxy(0, MENSAJES_Y_OFFSET); setColor(15);
                                    printf("Bot %d es INMUNE al obstaculo!            \n", b + 1);
                                    Sleep(300);
                                    gotoxy(0, MENSAJES_Y_OFFSET); printf("                                             \n");
                                } else {
                                    g_bot_estado_local[b] = 1;
                                    g_obstaculo_x[k] = -1;
                                    g_obstaculo_y[k] = -1;
                                    gotoxy(g_pos_x_bots_local[b] + 1, g_pos_y_bots_local[b] + 2);
                                    setColor(10);
                                    printf(" ");
                                    char bot_str2[8];
                                    sprintf(bot_str2, "[B%d]", b + 1);
                                    dibujarCaracter(-1, -1, "", 0, old_pos_x_bots[b], old_pos_y_bots[b], bot_str2);
                                    gotoxy(0, MENSAJES_Y_OFFSET + 1);
                                    printf("Bot %d ha chocado con 'X' y ha sido eliminado de la carrera!             \n", b + 1);
                                    Sleep(500);
                                    gotoxy(0, MENSAJES_Y_OFFSET + 1);
                                    printf("                                                              \n");
                                }
                                break;
                            }
                        }
                    } else if (bot_llego_meta[b]) {
                        char bot_str[8];
                        sprintf(bot_str, "[B%d]", b + 1);
                        dibujarCaracter(old_pos_x_bots[b], old_pos_y_bots[b], "    ", 0, old_pos_x_bots[b], old_pos_y_bots[b], bot_str);
                    }
                }
            }
            for (k = 0; k < NUM_ITEMS; k++) {
                if (g_item_recogido[k] == 0) {
                    if (!player1_llego_meta && g_pos_x_player1 == g_item_x[k] && g_pos_y_player1 == g_item_y[k]) {
                        aplicarEfectoItem(k, 1, -1, 2);
                    }
                    if (!player2_llego_meta && g_pos_x_player2 == g_item_x[k] && g_pos_y_player2 == g_item_y[k]) {
                        aplicarEfectoItem(k, 2, -1, 2);
                    }
                    if (incluir_bots && num_bots > 0) {
                        for (int b = 0; b < num_bots; b++) {
                            if (g_bot_estado_local[b] == 0 && !bot_llego_meta[b] && g_pos_x_bots_local[b] == g_item_x[k] && g_pos_y_bots_local[b] == g_item_y[k]) {
                                aplicarEfectoItem(k, -1, b, 2);
                            }
                        }
                    }
                }
            }
            for (k = 0; k < NUM_OBSTACULOS; k++) {
                if (!player1_llego_meta && g_pos_x_player1 == g_obstaculo_x[k] && g_pos_y_player1 == g_obstaculo_y[k]) {
                    if (g_inmunidad_player1 > 0) {
                        gotoxy(0, MENSAJES_Y_OFFSET); setColor(15);
                        printf("Jugador 1 es INMUNE al obstaculo!            \n");
                        Sleep(300);
                        gotoxy(0, MENSAJES_Y_OFFSET); printf("                                             \n");
                    } else {
                        limpiarPantalla();
                        printf("Jugador 1 choco con un obstaculo 'X' y ha sido eliminado. Jugador 2 es el ganador.\n");
                        derrotas_j1++;
                        victorias_j2++;
                        goto fin_juego_dos_jugadores;
                    }
                }
                if (!player2_llego_meta && g_pos_x_player2 == g_obstaculo_x[k] && g_pos_y_player2 == g_obstaculo_y[k]) {
                    if (g_inmunidad_player2 > 0) {
                        gotoxy(0, MENSAJES_Y_OFFSET); setColor(15);
                        printf("Jugador 2 es INMUNE al obstaculo!            \n");
                        Sleep(300);
                        gotoxy(0, MENSAJES_Y_OFFSET); printf("                                             \n");
                    } else {
                        limpiarPantalla();
                        printf("Jugador 2 choco con un obstaculo 'X' y ha sido eliminado. Jugador 1 es el ganador.\n");
                        derrotas_j2++;
                        victorias_j1++;
                        goto fin_juego_dos_jugadores;
                    }
                }
            }
            if (!player1_llego_meta && g_pos_x_player1 >= META) {
                player1_llego_meta = true;
                dibujarCaracter(g_pos_x_player1, g_pos_y_player1, "    ", 0, g_pos_x_player1, g_pos_y_player1, "[o1]");
                gotoxy(0, MENSAJES_Y_OFFSET + 6);
                setColor(10);
                printf("Jugador 1 ha llegado a la meta!                                   ");
                double tiempo = (double)(clock() - start_time) / CLOCKS_PER_SEC;
                strcpy(llegadas[total_llegadas].nombre, "Jugador 1");
                llegadas[total_llegadas].tiempo = tiempo;
                total_llegadas++;
            }
            if (!player2_llego_meta && g_pos_x_player2 >= META) {
                player2_llego_meta = true;
                dibujarCaracter(g_pos_x_player2, g_pos_y_player2, "    ", 0, g_pos_x_player2, g_pos_y_player2, "[o2]");
                gotoxy(0, MENSAJES_Y_OFFSET + 7);
                setColor(13);
                printf("Jugador 2 ha llegado a la meta!                                   ");
                double tiempo = (double)(clock() - start_time) / CLOCKS_PER_SEC;
                strcpy(llegadas[total_llegadas].nombre, "Jugador 2");
                llegadas[total_llegadas].tiempo = tiempo;
                total_llegadas++;
            }
            if (incluir_bots && num_bots > 0) {
                for (int b = 0; b < num_bots; b++) {
                    if (!bot_llego_meta[b] && g_bot_estado_local[b] == 0 && g_pos_x_bots_local[b] >= META) {
                        bot_llego_meta[b] = true;
                        char bot_str[8];
                        sprintf(bot_str, "[B%d]", b + 1);
                        dibujarCaracter(g_pos_x_bots_local[b], g_pos_y_bots_local[b], "    ", 0, g_pos_x_bots_local[b], g_pos_y_bots_local[b], bot_str);
                        gotoxy(0, MENSAJES_Y_OFFSET + 8 + b);
                        setColor(11 + b);
                        printf("Bot %d ha llegado a la meta!                                       ", b + 1);
                        double tiempo = (double)(clock() - start_time) / CLOCKS_PER_SEC;
                        sprintf(llegadas[total_llegadas].nombre, "Bot %d", b + 1);
                        llegadas[total_llegadas].tiempo = tiempo;
                        total_llegadas++;
                    }
                }
            }
            int en_carrera = 0;
            if (!player1_llego_meta) en_carrera++;
            if (!player2_llego_meta) en_carrera++;
            if (incluir_bots && num_bots > 0) {
                for (int b = 0; b < num_bots; b++) {
                    if (!bot_llego_meta[b] && g_bot_estado_local[b] == 0) en_carrera++;
                }
            }
            if (en_carrera == 0) {
                limpiarPantalla();
                printf("Todos han llegado a la meta o han sido eliminados.\n");
                printf("Fin de la carrera.\n");
                mostrarOrdenLlegada(llegadas, total_llegadas);
                empates++;
                goto fin_juego_dos_jugadores;
            }
            if (GetAsyncKeyState('P') & 0x8000) {
                limpiarPantalla();
                printf("Juego terminado.\n");
                return;
            }
            Sleep(30);
        }
    fin_juego_dos_jugadores:
        mostrarMarcadores(true);
        mostrarControlesJugador2();
        printf("\nPresiona 'R' para reiniciar o 'P' para volver al menu: ");
        fflush(stdin);
        do {
            opcion = tolower(_getch());
        } while (opcion != 'r' && opcion != 'p');
    }
}

// --- JUEGO UN JUGADOR ---

void iniciarJuegoUnJugador() {
    int num_bots, dificultad_bot, b, k;
    char opcion = 'r';
    clock_t start_time;
    Llegada llegadas[1 + MAX_BOTS];
    int total_llegadas = 0;
    limpiarPantalla();
    printf("--- Modo de Un Jugador ---\n");
    printf("Cuantos bots quieres (1-%d)? ", MAX_BOTS);
    scanf("%d", &num_bots);
    while (num_bots < 1 || num_bots > MAX_BOTS) {
        printf("Numero de bots invalido. Por favor, introduce un valor entre 1 y %d: ", MAX_BOTS);
        while (getchar() != '\n');
        scanf("%d", &num_bots);
    }
    fflush(stdin);
    printf("Selecciona la dificultad de los bots:\n");
    printf("1. Facil\n2. Normal\n3. Dificil\nTu opcion: ");
    scanf("%d", &dificultad_bot);
    while (dificultad_bot < 1 || dificultad_bot > 3) {
        printf("Dificultad invalida. Por favor, introduce 1, 2 o 3: ");
        while (getchar() != '\n');
        scanf("%d", &dificultad_bot);
    }
    fflush(stdin);
    while (opcion == 'r') {
        limpiarPantalla();
        g_pos_x_player1 = 0;
        g_pos_y_player1 = ALTO_PISTA / 2;
        g_num_active_bots = num_bots;
        g_inmunidad_player1 = 0;
        for (b = 0; b < MAX_BOTS; b++) {
            g_inmunidad_bots[b] = 0;
            g_bot_estado[b] = 0;
        }
        for (b = 0; b < num_bots; b++) {
            g_pos_x_bots[b] = 0;
            g_pos_y_bots[b] = (ALTO_PISTA / (num_bots + 1)) * (b + 1);
            if (g_pos_y_bots[b] == g_pos_y_player1) g_pos_y_bots[b] = (g_pos_y_bots[b] + 1) % ALTO_PISTA;
            g_bot_estado[b] = 0;
        }
        srand((unsigned int)time(NULL));
        start_time = clock();
        total_llegadas = 0;
        for (int l = 0; l < 1 + MAX_BOTS; ++l) llegadas[l].nombre[0] = '\0';
        organizarObstaculosYItems();
        dibujarPistaBase();
        mostrarMarcadores(false);
        int old_pos_x_jugador = g_pos_x_player1, old_pos_y_jugador = g_pos_y_player1;
        int old_pos_x_bots[MAX_BOTS], old_pos_y_bots[MAX_BOTS];
        for (b = 0; b < num_bots; b++) {
            old_pos_x_bots[b] = g_pos_x_bots[b];
            old_pos_y_bots[b] = g_pos_y_bots[b];
        }
        bool player_llego_meta = false, bot_llego_meta[MAX_BOTS] = {false};
        while (1) {
            gotoxy(0, 0);
            setColor(10);
            printf("Tiempo jugado: %.2f segundos", (double)(clock() - start_time) / CLOCKS_PER_SEC);
            if (g_inmunidad_player1 > 0) g_inmunidad_player1--;
            for (b = 0; b < num_bots; b++) if (g_inmunidad_bots[b] > 0) g_inmunidad_bots[b]--;
            old_pos_x_jugador = g_pos_x_player1; old_pos_y_jugador = g_pos_y_player1;
            for (b = 0; b < num_bots; b++) {
                old_pos_x_bots[b] = g_pos_x_bots[b];
                old_pos_y_bots[b] = g_pos_y_bots[b];
            }
            if (!player_llego_meta) {
                if (GetAsyncKeyState('D') & 0x8000) g_pos_x_player1++;
                if (GetAsyncKeyState('A') & 0x8000 && g_pos_x_player1 > 0) g_pos_x_player1--;
                if (GetAsyncKeyState('W') & 0x8000 && g_pos_y_player1 > 0) g_pos_y_player1--;
                if (GetAsyncKeyState('S') & 0x8000 && g_pos_y_player1 < ALTO_PISTA - 1) g_pos_y_player1++;
            }
            if (!player_llego_meta)
                dibujarCaracter(g_pos_x_player1, g_pos_y_player1, "[P]", (g_inmunidad_player1 > 0 ? 14 : 9), old_pos_x_jugador, old_pos_y_jugador, "[P]");
            else
                dibujarCaracter(old_pos_x_jugador, old_pos_y_jugador, "   ", 0, old_pos_x_jugador, old_pos_y_jugador, "[P]");
            for (k = 0; k < NUM_ITEMS; k++) {
                if (g_item_recogido[k] == 0) {
                    if (!player_llego_meta && g_pos_x_player1 == g_item_x[k] && g_pos_y_player1 == g_item_y[k]) {
                        aplicarEfectoItem(k, 0, -1, 1);
                    }
                    for (b = 0; b < num_bots; b++) {
                        if (g_bot_estado[b] == 0 && !bot_llego_meta[b] && g_pos_x_bots[b] == g_item_x[k] && g_pos_y_bots[b] == g_item_y[k]) {
                            aplicarEfectoItem(k, -1, b, 1);
                        }
                    }
                }
            }
            for (b = 0; b < num_bots; b++) {
                if (g_bot_estado[b] == 0 && !bot_llego_meta[b]) {
                    int next_x = g_pos_x_bots[b] + 1;
                    int next_y = g_pos_y_bots[b];
                    bool hay_obstaculo = false;
                    for (int k = 0; k < NUM_OBSTACULOS; k++) {
                        if (g_obstaculo_x[k] == next_x && g_obstaculo_y[k] == next_y) {
                            hay_obstaculo = true;
                            break;
                        }
                    }
                    if (dificultad_bot == 1) {
                        if (!hay_obstaculo) g_pos_x_bots[b]++;
                        else {
                            if (g_pos_y_bots[b] > 0) {
                                bool libre_arriba = true;
                                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                    if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] - 1) {
                                        libre_arriba = false;
                                        break;
                                    }
                                }
                                if (libre_arriba) g_pos_y_bots[b]--;
                                else if (g_pos_y_bots[b] < ALTO_PISTA - 1) {
                                    bool libre_abajo = true;
                                    for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                        if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] + 1) {
                                            libre_abajo = false;
                                            break;
                                        }
                                    }
                                    if (libre_abajo) g_pos_y_bots[b]++;
                                }
                            } else if (g_pos_y_bots[b] < ALTO_PISTA - 1) {
                                bool libre_abajo = true;
                                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                    if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] + 1) {
                                        libre_abajo = false;
                                        break;
                                    }
                                }
                                if (libre_abajo) g_pos_y_bots[b]++;
                            }
                        }
                        if (rand() % 100 < 20) {
                            if (rand() % 2 == 0 && g_pos_y_bots[b] > 0) g_pos_y_bots[b]--;
                            else if (g_pos_y_bots[b] < ALTO_PISTA - 1) g_pos_y_bots[b]++;
                        }
                    } else if (dificultad_bot == 2) {
                        if (!hay_obstaculo) g_pos_x_bots[b]++;
                        else {
                            if (g_pos_y_bots[b] > 0) {
                                bool libre_arriba = true;
                                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                    if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] - 1) {
                                        libre_arriba = false;
                                        break;
                                    }
                                }
                                if (libre_arriba) g_pos_y_bots[b]--;
                                else if (g_pos_y_bots[b] < ALTO_PISTA - 1) {
                                    bool libre_abajo = true;
                                    for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                        if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] + 1) {
                                            libre_abajo = false;
                                            break;
                                        }
                                    }
                                    if (libre_abajo) g_pos_y_bots[b]++;
                                }
                            } else if (g_pos_y_bots[b] < ALTO_PISTA - 1) {
                                bool libre_abajo = true;
                                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                    if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] + 1) {
                                        libre_abajo = false;
                                        break;
                                    }
                                }
                                if (libre_abajo) g_pos_y_bots[b]++;
                            }
                        }
                        if (rand() % 100 < 30) {
                            if (rand() % 2 == 0 && g_pos_y_bots[b] > 0) g_pos_y_bots[b]--;
                            else if (g_pos_y_bots[b] < ALTO_PISTA - 1) g_pos_y_bots[b]++;
                        }
                    } else {
                        if (!hay_obstaculo) g_pos_x_bots[b]++;
                        else {
                            bool esquivado = false;
                            for (int dy = 1; dy <= g_pos_y_bots[b]; dy++) {
                                bool libre = true;
                                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                    if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] - dy) {
                                        libre = false;
                                        break;
                                    }
                                }
                                if (libre) {
                                    g_pos_y_bots[b] -= dy;
                                    esquivado = true;
                                    break;
                                }
                            }
                            if (!esquivado) {
                                for (int dy = 1; dy < ALTO_PISTA - g_pos_y_bots[b]; dy++) {
                                    bool libre = true;
                                    for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                        if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] + dy) {
                                            libre = false;
                                            break;
                                        }
                                    }
                                    if (libre) {
                                        g_pos_y_bots[b] += dy;
                                        esquivado = true;
                                        break;
                                    }
                                }
                            }
                            if (!esquivado) g_pos_x_bots[b]++;
                        }
                        if (rand() % 100 < 40) {
                            if (rand() % 2 == 0 && g_pos_y_bots[b] > 0) g_pos_y_bots[b]--;
                            else if (g_pos_y_bots[b] < ALTO_PISTA - 1) g_pos_y_bots[b]++;
                        }
                    }
                    char bot_str[8];
                    sprintf(bot_str, "[B%d]", b + 1);
                    dibujarCaracter(g_pos_x_bots[b], g_pos_y_bots[b], bot_str, (g_inmunidad_bots[b] > 0 ? 14 : 11 + b), old_pos_x_bots[b], old_pos_y_bots[b], bot_str);
                    for (k = 0; k < NUM_OBSTACULOS; k++) {
                        if (g_pos_x_bots[b] == g_obstaculo_x[k] && g_pos_y_bots[b] == g_obstaculo_y[k]) {
                            if (g_inmunidad_bots[b] > 0) {
                                gotoxy(0, MENSAJES_Y_OFFSET); setColor(15);
                                printf("Bot %d es INMUNE al obstaculo!            \n", b + 1);
                                Sleep(300);
                                gotoxy(0, MENSAJES_Y_OFFSET); printf("                                             \n");
                            } else {
                                g_bot_estado[b] = 1;
                                g_num_active_bots--;
                                g_obstaculo_x[k] = -1;
                                g_obstaculo_y[k] = -1;
                                gotoxy(g_pos_x_bots[b] + 1, g_pos_y_bots[b] + 2);
                                setColor(10);
                                printf(" ");
                                char bot_str2[8];
                                sprintf(bot_str2, "[B%d]", b + 1);
                                dibujarCaracter(-1, -1, "", 0, old_pos_x_bots[b], old_pos_y_bots[b], bot_str2);
                                gotoxy(0, MENSAJES_Y_OFFSET + 1);
                                printf("Bot %d ha chocado con 'X' y ha sido eliminado de la carrera!             \n", b + 1);
                                Sleep(500);
                                gotoxy(0, MENSAJES_Y_OFFSET + 1);
                                printf("                                                              \n");
                            }
                            break;
                        }
                    }
                }
            }
            for (k = 0; k < NUM_OBSTACULOS; k++) {
                if (!player_llego_meta && g_pos_x_player1 == g_obstaculo_x[k] && g_pos_y_player1 == g_obstaculo_y[k]) {
                    if (g_inmunidad_player1 > 0) {
                        gotoxy(0, MENSAJES_Y_OFFSET); setColor(15);
                        printf("Jugador es INMUNE al obstaculo!            \n");
                        Sleep(300);
                        gotoxy(0, MENSAJES_Y_OFFSET); printf("                                             \n");
                    } else {
                        limpiarPantalla();
                        printf("Chocaste con un obstaculo 'X' y has sido eliminado!\n");
                        derrotas_jugador_un_jugador++;
                        goto fin_juego_un_jugador;
                    }
                }
            }
            if (!player_llego_meta && g_pos_x_player1 >= META) {
                player_llego_meta = true;
                dibujarCaracter(g_pos_x_player1, g_pos_y_player1, "   ", 0, g_pos_x_player1, g_pos_y_player1, "[P]");
                gotoxy(0, MENSAJES_Y_OFFSET + 6);
                setColor(10);
                printf("Jugador ha llegado a la meta!                                   ");
                double tiempo = (double)(clock() - start_time) / CLOCKS_PER_SEC;
                strcpy(llegadas[total_llegadas].nombre, "Jugador");
                llegadas[total_llegadas].tiempo = tiempo;
                total_llegadas++;
            }
            for (b = 0; b < num_bots; b++) {
                if (!bot_llego_meta[b] && g_bot_estado[b] == 0 && g_pos_x_bots[b] >= META) {
                    bot_llego_meta[b] = true;
                    g_num_active_bots--;
                    char bot_str[8];
                    sprintf(bot_str, "[B%d]", b + 1);
                    dibujarCaracter(g_pos_x_bots[b], g_pos_y_bots[b], "    ", 0, g_pos_x_bots[b], g_pos_y_bots[b], bot_str);
                    gotoxy(0, MENSAJES_Y_OFFSET + 8 + b);
                    setColor(11 + b);
                    printf("Bot %d ha llegado a la meta!                                       ", b + 1);
                    double tiempo = (double)(clock() - start_time) / CLOCKS_PER_SEC;
                    sprintf(llegadas[total_llegadas].nombre, "Bot %d", b + 1);
                    llegadas[total_llegadas].tiempo = tiempo;
                    total_llegadas++;
                }
            }
            int en_carrera = 0;
            if (!player_llego_meta) en_carrera++;
            for (b = 0; b < num_bots; b++) {
                if (!bot_llego_meta[b] && g_bot_estado[b] == 0) en_carrera++;
            }
            if (en_carrera == 0) {
                limpiarPantalla();
                printf("Todos han llegado a la meta o han sido eliminados.\n");
                printf("Fin de la carrera.\n");
                mostrarOrdenLlegada(llegadas, total_llegadas);
                if (player_llego_meta)
                    victorias_jugador_un_jugador++;
                else
                    derrotas_jugador_un_jugador++;
                goto fin_juego_un_jugador;
            }
            for (b = 0; b < num_bots; b++) {
                if (g_bot_estado[b] == 0 && !bot_llego_meta[b] && !player_llego_meta) {
                    if (g_pos_x_player1 == g_pos_x_bots[b] && g_pos_y_player1 == g_pos_y_bots[b]) {
                        g_bot_estado[b] = 1;
                        g_num_active_bots--;
                        char bot_str[8];
                        sprintf(bot_str, "[B%d]", b + 1);
                        dibujarCaracter(-1, -1, "", 0, old_pos_x_bots[b], old_pos_y_bots[b], bot_str);
                        gotoxy(0, MENSAJES_Y_OFFSET);
                        printf("Chocaste con el Bot %d! El bot ha sido eliminado de la carrera.               \n", b + 1);
                        Sleep(500);
                        gotoxy(0, MENSAJES_Y_OFFSET);
                        printf("                                                              \n");
                        break;
                    }
                }
            }
            if (GetAsyncKeyState('P') & 0x8000) {
                limpiarPantalla();
                printf("Juego terminado.\n");
                return;
            }
            Sleep(30);
        }
    fin_juego_un_jugador:
        mostrarMarcadores(false);
        if (total_llegadas > 0) mostrarOrdenLlegada(llegadas, total_llegadas);
        printf("\nPresiona 'R' para reiniciar o 'P' para volver al menu: ");
        fflush(stdin);
        do {
            opcion = tolower(_getch());
        } while (opcion != 'r' && opcion != 'p');
    }
}

int main() {
    int modo_juego;
    while (1) {
        limpiarPantalla();
        setColor(15);
        printf("==============================================\n");
        printf("========== Bienvenido a la Carrera! ==========\n");
        printf("==============================================\n");
        printf("\nSelecciona el modo de juego:\n");
        printf("1. Un Jugador (vs Bots)\n");
        printf("2. Dos Jugadores\n");
        printf("3. Salir\n");
        printf("Tu opcion: ");
        int res = scanf("%d", &modo_juego);
        if (res != 1) {
            fflush(stdin);
            modo_juego = 0;
        } else {
            fflush(stdin);
        }
        switch (modo_juego) {
            case 1:
                iniciarJuegoUnJugador();
                break;
            case 2:
                iniciarJuegoDosJugadores();
                break;
            case 3:
                limpiarPantalla();
                printf("Gracias por jugar. Hasta pronto!\n");
                return 0;
            default:
                printf("Opcion invalida. Por favor, introduce 1, 2 o 3.\n");
                Sleep(1500);
                break;
        }
    }
    return 0;
}
