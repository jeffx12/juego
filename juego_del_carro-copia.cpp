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
#define LINEA_ITEM_BOT (ALTO_PISTA + 8 + 5)
#define BOOST_DURACION 300
#define SLOWDOWN_DURACION 600

enum ItemType {
    ITEM_INMUNIDAD,
    ITEM_TELETRANSPORTE,
    ITEM_TRAMPA,
    ITEM_BOOST,
    ITEM_SLOWDOWN,
    NUM_ITEM_TIPOS
};

const char* nombre_item(ItemType tipo) {
    switch (tipo) {
        case ITEM_INMUNIDAD: return "INMUNIDAD";
        case ITEM_TELETRANSPORTE: return "TELETRANSPORTE";
        case ITEM_TRAMPA: return "TRAMPA";
        case ITEM_BOOST: return "BOOST";
        case ITEM_SLOWDOWN: return "SLOWDOWN";
        default: return "DESCONOCIDO";
    }
}

// Estadisticas globales
int victorias_j1 = 0, derrotas_j1 = 0, empates = 0;
int victorias_j2 = 0, derrotas_j2 = 0;
int victorias_jugador_un_jugador = 0, derrotas_jugador_un_jugador = 0;

// Estado global del juego
int g_pos_x_player1, g_pos_y_player1;
int g_pos_x_player2, g_pos_y_player2;
int g_pos_x_bots[MAX_BOTS], g_pos_y_bots[MAX_BOTS];
int g_bot_estado[MAX_BOTS];
int g_num_active_bots;

int g_obstaculo_x[NUM_OBSTACULOS], g_obstaculo_y[NUM_OBSTACULOS];
int g_item_x[NUM_ITEMS], g_item_y[NUM_ITEMS];
int g_item_recogido[NUM_ITEMS];

// Sistema de items
int g_inmunidad_player1 = 0;
int g_inmunidad_player2 = 0;
int g_inmunidad_bots[MAX_BOTS];

int g_item_portado_player1 = -1;
int g_item_portado_player2 = -1;
int g_item_portado_bots[MAX_BOTS];

bool g_item_en_uso_player1 = false;
bool g_item_en_uso_player2 = false;
bool g_item_en_uso_bots[MAX_BOTS];

// Sistema de velocidad
int g_boost_player1 = 0;
int g_boost_player2 = 0;
int g_boost_bots[MAX_BOTS];

int g_slowdown_player1 = 0;
int g_slowdown_player2 = 0;
int g_slowdown_bots[MAX_BOTS];

// Puntuacion
int g_puntuacion_player1 = 0;
int g_puntuacion_player2 = 0;
int g_puntuacion_bots[MAX_BOTS];

// (Eliminado sistema de vidas múltiples)

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

void mostrarEstadisticasJuego(bool dos_jugadores) {
    setColor(11);
    if (dos_jugadores) {
        gotoxy(0, ALTO_PISTA + 12);
        printf("=== ESTADISTICAS ===\n");
        printf("J1 - Puntos: %d | Boost: %d | Slow: %d\n", 
               g_puntuacion_player1, g_boost_player1, g_slowdown_player1);
        printf("J2 - Puntos: %d | Boost: %d | Slow: %d\n", 
               g_puntuacion_player2, g_boost_player2, g_slowdown_player2);
        for (int i = 0; i < g_num_active_bots; i++) {
            printf("Bot%d - Puntos: %d | Boost: %d | Slow: %d\n", 
                   i+1, g_puntuacion_bots[i], g_boost_bots[i], g_slowdown_bots[i]);
        }
    } else {
        gotoxy(0, ALTO_PISTA + 10);
        printf("=== ESTADISTICAS ===\n");
        printf("Jugador - Puntos: %d | Boost: %d | Slow: %d\n", 
               g_puntuacion_player1, g_boost_player1, g_slowdown_player1);
        for (int i = 0; i < g_num_active_bots; i++) {
            printf("Bot%d - Puntos: %d | Boost: %d | Slow: %d\n", 
                   i+1, g_puntuacion_bots[i], g_boost_bots[i], g_slowdown_bots[i]);
        }
    }
}

void inicializarEstadisticas(int num_bots) {
    g_puntuacion_player1 = 0;
    g_puntuacion_player2 = 0;
    g_boost_player1 = 0;
    g_boost_player2 = 0;
    g_slowdown_player1 = 0;
    g_slowdown_player2 = 0;
    
    for (int i = 0; i < MAX_BOTS; i++) {
        g_puntuacion_bots[i] = 0;
        g_boost_bots[i] = 0;
        g_slowdown_bots[i] = 0;
    }
    g_num_active_bots = num_bots;
}

void dibujarCaracter(int x, int y, const char* str, int color, int old_x, int old_y, const char* old_str) {
    // Borra la posicion anterior
    if (old_x != -1 && old_y != -1) {
        gotoxy(old_x + 1, old_y + 2);
        setColor(10);
        for (size_t i = 0; i < strlen(old_str); i++) printf(" ");
    }
    // Dibuja la nueva posicion
    if (x != -1 && y != -1) {
        gotoxy(x + 1, y + 2);
        setColor(color);
        printf("%s", str);
    }
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
            bool dibujado = false;
            // Linea de meta
            if (i == META) {
                setColor(6);
                printf("|");
                dibujado = true;
            }
            // Obstaculos
            if (!dibujado) {
                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                    if (g_obstaculo_x[k] == i && g_obstaculo_y[k] == j) {
                        setColor(12);
                        printf("X");
                        dibujado = true;
                        break;
                    }
                }
            }
            // Items
            if (!dibujado) {
                for (int k = 0; k < NUM_ITEMS; k++) {
                    if (g_item_recogido[k] == 0 && g_item_x[k] == i && g_item_y[k] == j) {
                        setColor(11);
                        printf("O");
                        dibujado = true;
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
    // Borde inferior
    setColor(2);
    gotoxy(0, ALTO_PISTA + 2);
    printf("+");
    for (int i = 0; i < ANCHO_PISTA; i++) printf("-");
    printf("+\n");
    // Decoracion meta
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
    memset(g_obstaculo_x, -1, sizeof(g_obstaculo_x));
    memset(g_obstaculo_y, -1, sizeof(g_obstaculo_y));
    memset(g_item_x, -1, sizeof(g_item_x));
    memset(g_item_y, -1, sizeof(g_item_y));
    memset(g_item_recogido, 0, sizeof(g_item_recogido));
    // Obstaculos
    int idx = 0;
    while (idx < NUM_OBSTACULOS) {
        int x = rand() % (META - 10) + 10;
        int y = rand() % ALTO_PISTA;
        bool solapa = false;
        for (int k = 0; k < idx; k++) {
            if (g_obstaculo_x[k] == x && g_obstaculo_y[k] == y) {
                solapa = true;
                break;
            }
        }
        if (!solapa) {
            g_obstaculo_x[idx] = x;
            g_obstaculo_y[idx] = y;
            idx++;
        }
    }
    // Items
    idx = 0;
    while (idx < NUM_ITEMS) {
        int x = rand() % (META - 10) + 10;
        int y = rand() % ALTO_PISTA;
        bool solapa = false;
        for (int k = 0; k < NUM_OBSTACULOS; k++) {
            if (g_obstaculo_x[k] == x && g_obstaculo_y[k] == y) {
                solapa = true;
                break;
            }
        }
        for (int k = 0; k < idx; k++) {
            if (g_item_x[k] == x && g_item_y[k] == y) {
                solapa = true;
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
            if (jugador == 0) g_puntuacion_player1 += 10;
            else if (jugador == 1) g_puntuacion_player2 += 10;
            Sleep(1000);
            gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
            printf("                                                                      ");
            break;
            
        case ITEM_TELETRANSPORTE: {
            int avance = rand() % 20 + 10;
            *pos_x = min2(*pos_x + avance, META - 1);
            gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
            setColor(14);
            printf("TELETRANSPORTE usado! Avanzas %d posiciones.                         ", avance);
            if (jugador == 0) g_puntuacion_player1 += 15;
            else if (jugador == 1) g_puntuacion_player2 += 15;
            Sleep(1000);
            gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
            printf("                                                                      ");
            break;
        }
        
        case ITEM_TRAMPA: {
            int retroceso = rand() % 10 + 5;
            if (pos_x_otro != NULL) {
                *pos_x_otro = max2(*pos_x_otro - retroceso, 0);
                gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
                setColor(12);
                printf("TRAMPA usada! El oponente retrocede %d posiciones.                   ", retroceso);
                if (jugador == 0) g_puntuacion_player1 += 20;
                else if (jugador == 1) g_puntuacion_player2 += 20;
                Sleep(1000);
                gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
                printf("                                                                      ");
            }
            break;
        }
        
        case ITEM_BOOST:
            if (jugador == 0) g_boost_player1 = BOOST_DURACION;
            else if (jugador == 1) g_boost_player2 = BOOST_DURACION;
            gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
            setColor(10);
            printf("BOOST activado! Velocidad aumentada por 10 segundos.                 ");
            if (jugador == 0) g_puntuacion_player1 += 10;
            else if (jugador == 1) g_puntuacion_player2 += 10;
            Sleep(1000);
            gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
            printf("                                                                      ");
            break;
            
        case ITEM_SLOWDOWN:
            if (pos_x_otro != NULL && jugador == 0) g_slowdown_player2 = SLOWDOWN_DURACION;
            else if (pos_x_otro != NULL && jugador == 1) g_slowdown_player1 = SLOWDOWN_DURACION;
            gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
            setColor(4);
            printf("SLOWDOWN usado! El oponente se ralentiza por 20 segundos.             ");
            if (jugador == 0) g_puntuacion_player1 += 15;
            else if (jugador == 1) g_puntuacion_player2 += 15;
            Sleep(1000);
            gotoxy(0, MENSAJES_Y_OFFSET + 4 + jugador);
            printf("                                                                      ");
            break;
    }
    
    *item_portado = -1;
    *en_uso = false;
    mostrarItemPortado(jugador, -1, false);
}

void usarItemPortadoBot(int* pos_x, int* pos_y, int* inmunidad, int* item_portado, bool* en_uso, int bot) {
    if (*item_portado == -1 || *en_uso) return;
    *en_uso = true;
    mostrarItemPortadoBot(bot, *item_portado, true);
    
    switch (*item_portado) {
        case ITEM_INMUNIDAD:
            *inmunidad = INMUNIDAD_DURACION;
            g_puntuacion_bots[bot] += 10;
            break;
            
        case ITEM_TELETRANSPORTE: {
            int avance = rand() % 20 + 10;
            *pos_x = min2(*pos_x + avance, META - 1);
            g_puntuacion_bots[bot] += 15;
            break;
        }
        
        case ITEM_TRAMPA: {
            int retroceso = rand() % 10 + 5;
            if (g_pos_x_player1 > *pos_x - 20) {
                g_pos_x_player1 = max2(g_pos_x_player1 - retroceso, 0);
                g_puntuacion_bots[bot] += 20;
            }
            break;
        }
        
        case ITEM_BOOST:
            g_boost_bots[bot] = BOOST_DURACION;
            g_puntuacion_bots[bot] += 10;
            break;
            
        case ITEM_SLOWDOWN:
            g_slowdown_player1 = SLOWDOWN_DURACION;
            g_puntuacion_bots[bot] += 15;
            break;
    }
    
    *item_portado = -1;
    *en_uso = false;
    mostrarItemPortadoBot(bot, -1, false);
}

void aplicarEfectoItem(int item_idx, int player_id, int bot_id) {
    if (g_item_recogido[item_idx] == 0) {
        g_item_recogido[item_idx] = 1;
        gotoxy(g_item_x[item_idx] + 1, g_item_y[item_idx] + 2);
        setColor(10);
        printf(" ");
        ItemType efecto = (ItemType)(rand() % NUM_ITEM_TIPOS);
        if (player_id == 1) {
            recogerItem(0, &g_item_portado_player1, &g_item_en_uso_player1, efecto, 'Q');
        } else if (player_id == 2) {
            recogerItem(1, &g_item_portado_player2, &g_item_en_uso_player2, efecto, 'O');
        } else if (bot_id != -1) {
            recogerItemBot(bot_id, &g_item_portado_bots[bot_id], &g_item_en_uso_bots[bot_id], efecto);
        }
    }
}

void iniciarJuegoDosJugadores() {
    char opcion = 'r';
    clock_t start_time;
    int old_pos_x1, old_pos_y1, old_pos_x2, old_pos_y2;
    Llegada llegadas[2 + MAX_BOTS];
    int total_llegadas = 0;
    int incluir_bots = -1, num_bots = 0;
    bool player1_eliminado = false, player2_eliminado = false;
    do {
        limpiarPantalla();
        printf("--- Modo Dos Jugadores ---\n");
        printf("Quieres incluir bots en la carrera? (s/n): ");
        char resp = _getch();
        if (tolower(resp) == 's') {
            incluir_bots = 1;
            printf("s\n");
            printf("Cuantos bots quieres? (1-%d): ", MAX_BOTS);
            scanf("%d", &num_bots);
            while (num_bots < 1 || num_bots > MAX_BOTS) {
                printf("Numero de bots invalido. Por favor, introduce un valor entre 1 y %d: ", MAX_BOTS);
                scanf("%d", &num_bots);
            }
            fflush(stdin);
        } else if (tolower(resp) == 'n') {
            printf("n\n");
            incluir_bots = 0;
            num_bots = 0;
        } else {
            printf("\nRespuesta invalida. Por favor, responde 's' o 'n'.\n");
            Sleep(1000);
        }
    } while (incluir_bots == -1);

    while (opcion == 'r') {
        inicializarEstadisticas(num_bots);
        limpiarPantalla();
        g_pos_x_player1 = 0;
        g_pos_y_player1 = ALTO_PISTA / 2;
        g_pos_x_player2 = 0;
        g_pos_y_player2 = ALTO_PISTA / 2 + 1;
        g_inmunidad_player1 = 0;
        g_inmunidad_player2 = 0;
        g_item_portado_player1 = -1;
        g_item_portado_player2 = -1;
        g_item_en_uso_player1 = false;
        g_item_en_uso_player2 = false;
        total_llegadas = 0;
        player1_eliminado = false;
        player2_eliminado = false;
        for (int l = 0; l < 2 + MAX_BOTS; ++l) llegadas[l].nombre[0] = '\0';
        srand((unsigned int)time(NULL));
        start_time = clock();
        organizarObstaculosYItems();
        dibujarPistaBase();
        mostrarMarcadores(true);
        old_pos_x1 = g_pos_x_player1; old_pos_y1 = g_pos_y_player1;
        old_pos_x2 = g_pos_x_player2; old_pos_y2 = g_pos_y_player2;
        for (int b = 0; b < MAX_BOTS; b++) {
            g_pos_x_bots[b] = 0;
            g_pos_y_bots[b] = (ALTO_PISTA / (max2(num_bots, 1) + 2)) * (b + 1);
            if (g_pos_y_bots[b] >= ALTO_PISTA) g_pos_y_bots[b] = ALTO_PISTA - 1;
            g_inmunidad_bots[b] = 0;
            g_bot_estado[b] = 0;
            g_item_portado_bots[b] = -1;
            g_item_en_uso_bots[b] = false;
        }
        bool player1_llego_meta = false, player2_llego_meta = false;
        bool bot_llego_meta[MAX_BOTS] = {false};
        bool bot_eliminado[MAX_BOTS] = {false}; // <--- AGREGADO SEGUN INSTRUCCION 1
        mostrarItemPortado(0, g_item_portado_player1, g_item_en_uso_player1);
        mostrarItemPortado(1, g_item_portado_player2, g_item_en_uso_player2);
        for (int b = 0; b < num_bots; b++) {
            mostrarItemPortadoBot(b, g_item_portado_bots[b], g_item_en_uso_bots[b]);
        }
        while (1) {
            gotoxy(0, 0);
            setColor(10);
            printf("Tiempo jugado: %.2f segundos", (double)(clock() - start_time) / CLOCKS_PER_SEC);
            if (g_inmunidad_player1 > 0) g_inmunidad_player1--;
            if (g_inmunidad_player2 > 0) g_inmunidad_player2--;
            for (int b = 0; b < MAX_BOTS; b++) if (g_inmunidad_bots[b] > 0) g_inmunidad_bots[b]--;

            // Actualizar efectos de velocidad
            if (g_boost_player1 > 0) g_boost_player1--;
            if (g_boost_player2 > 0) g_boost_player2--;
            if (g_slowdown_player1 > 0) g_slowdown_player1--;
            if (g_slowdown_player2 > 0) g_slowdown_player2--;
            for (int b = 0; b < MAX_BOTS; b++) {
                if (g_boost_bots[b] > 0) g_boost_bots[b]--;
                if (g_slowdown_bots[b] > 0) g_slowdown_bots[b]--;
            }

            // Mostrar estadísticas
            mostrarEstadisticasJuego(true);

            old_pos_x1 = g_pos_x_player1; old_pos_y1 = g_pos_y_player1;
            old_pos_x2 = g_pos_x_player2; old_pos_y2 = g_pos_y_player2;
            // Jugador 1
            if (!player1_llego_meta && !player1_eliminado) {
                int velocidad1 = (g_boost_player1 > 0) ? 2 : 1;
                if (g_slowdown_player1 > 0) velocidad1 = (velocidad1 > 1) ? 1 : 0;
                
                if (GetAsyncKeyState('D') & 0x8000) {
                    for (int v = 0; v < velocidad1 && g_pos_x_player1 < META; v++) {
                        g_pos_x_player1++;
                    }
                }
                if (GetAsyncKeyState('A') & 0x8000 && g_pos_x_player1 > 0) g_pos_x_player1--;
                if (GetAsyncKeyState('W') & 0x8000 && g_pos_y_player1 > 0) g_pos_y_player1--;
                if (GetAsyncKeyState('S') & 0x8000 && g_pos_y_player1 < ALTO_PISTA - 1) g_pos_y_player1++;
                if (GetAsyncKeyState('Q') & 0x8000) {
                    usarItemPortado(&g_pos_x_player1, &g_pos_y_player1, &g_inmunidad_player1, 
                                  &g_item_portado_player1, &g_item_en_uso_player1, 0, 2, 
                                  &g_pos_x_player2, &g_pos_y_player2);
                    Sleep(100);
                }
            }
            // Jugador 2
            if (!player2_llego_meta && !player2_eliminado) {
                int velocidad2 = (g_boost_player2 > 0) ? 2 : 1;
                if (g_slowdown_player2 > 0) velocidad2 = (velocidad2 > 1) ? 1 : 0;
                
                if (GetAsyncKeyState('L') & 0x8000) {
                    for (int v = 0; v < velocidad2 && g_pos_x_player2 < META; v++) {
                        g_pos_x_player2++;
                    }
                }
                if (GetAsyncKeyState('J') & 0x8000 && g_pos_x_player2 > 0) g_pos_x_player2--;
                if (GetAsyncKeyState('I') & 0x8000 && g_pos_y_player2 > 0) g_pos_y_player2--;
                if (GetAsyncKeyState('K') & 0x8000 && g_pos_y_player2 < ALTO_PISTA - 1) g_pos_y_player2++;
                if (GetAsyncKeyState('O') & 0x8000) {
                    usarItemPortado(&g_pos_x_player2, &g_pos_y_player2, &g_inmunidad_player2, 
                                  &g_item_portado_player2, &g_item_en_uso_player2, 1, 2, 
                                  &g_pos_x_player1, &g_pos_y_player1);
                    Sleep(100);
                }
            }
            // Colision entre jugadores
            if (!player1_llego_meta && !player2_llego_meta && 
                g_pos_x_player1 == g_pos_x_player2 && g_pos_y_player1 == g_pos_y_player2) {
                limpiarPantalla();
                printf("Choque entre jugadores. Empate, fin del juego.\n");
                empates++;
                break;
            }
            // Dibuja jugadores
            if (!player1_llego_meta && !player1_eliminado)
                dibujarCaracter(g_pos_x_player1, g_pos_y_player1, "[o1]", (g_inmunidad_player1 > 0 ? 14 : 9), old_pos_x1, old_pos_y1, "[o1]");
            else
                dibujarCaracter(-1, -1, "", 0, old_pos_x1, old_pos_y1, "[o1]");
            if (!player2_llego_meta && !player2_eliminado)
                dibujarCaracter(g_pos_x_player2, g_pos_y_player2, "[o2]", (g_inmunidad_player2 > 0 ? 14 : 13), old_pos_x2, old_pos_y2, "[o2]");
            else
                dibujarCaracter(-1, -1, "", 0, old_pos_x2, old_pos_y2, "[o2]");
            // IA de bots y dibujo
            int old_pos_x_bots[MAX_BOTS], old_pos_y_bots[MAX_BOTS];
            for (int b = 0; b < MAX_BOTS; b++) {
                old_pos_x_bots[b] = g_pos_x_bots[b];
                old_pos_y_bots[b] = g_pos_y_bots[b];
            }
            if (incluir_bots && num_bots > 0) {
                for (int b = 0; b < num_bots; b++) {
                    if (g_bot_estado[b] == 0 && !bot_llego_meta[b] && !bot_eliminado[b]) { // <--- INSTRUCCION 3
                        // IA simple de bot
                        int next_x = g_pos_x_bots[b] + 1;
                        bool hay_obstaculo = false;
                        for (int k = 0; k < NUM_OBSTACULOS; k++) {
                            if (g_obstaculo_x[k] == next_x && g_obstaculo_y[k] == g_pos_y_bots[b]) {
                                hay_obstaculo = true;
                                break;
                            }
                        }
                        if (!hay_obstaculo) {
                            g_pos_x_bots[b] = min2(g_pos_x_bots[b] + 1, META);
                        } else {
                            // Intentar evitar obstaculo
                            bool movido = false;
                            if (g_pos_y_bots[b] > 0) {
                                bool libre_arriba = true;
                                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                    if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] - 1) {
                                        libre_arriba = false;
                                        break;
                                    }
                                }
                                if (libre_arriba) {
                                    g_pos_y_bots[b]--;
                                    movido = true;
                                }
                            }
                            if (!movido && g_pos_y_bots[b] < ALTO_PISTA - 1) {
                                bool libre_abajo = true;
                                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                    if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] + 1) {
                                        libre_abajo = false;
                                        break;
                                    }
                                }
                                if (libre_abajo) {
                                    g_pos_y_bots[b]++;
                                    movido = true;
                                }
                            }
                            // Si no puede moverse, se queda en su lugar
                        }
                        // Recoger item si hay
                        for (int k = 0; k < NUM_ITEMS; k++) {
                            if (g_item_recogido[k] == 0 && g_item_x[k] == g_pos_x_bots[b] && g_item_y[k] == g_pos_y_bots[b]) {
                                aplicarEfectoItem(k, 0, b);
                                break;
                            }
                        }
                        // Usar item aleatoriamente
                        if (g_item_portado_bots[b] != -1 && !g_item_en_uso_bots[b]) {
                            if (rand() % 30 == 0) { // Probabilidad de usar item
                                usarItemPortadoBot(&g_pos_x_bots[b], &g_pos_y_bots[b], &g_inmunidad_bots[b], &g_item_portado_bots[b], &g_item_en_uso_bots[b], b);
                            }
                        }
                    }
                    // Dibuja bot
                    if (!bot_llego_meta[b] && !bot_eliminado[b]) // <--- INSTRUCCION 4
                        dibujarCaracter(g_pos_x_bots[b], g_pos_y_bots[b], "[B]", (g_inmunidad_bots[b] > 0 ? 14 : 11), old_pos_x_bots[b], old_pos_y_bots[b], "[B]");
                    else
                        dibujarCaracter(-1, -1, "", 0, old_pos_x_bots[b], old_pos_y_bots[b], "[B]");
                }
            }
            // Recoger items jugadores
            for (int k = 0; k < NUM_ITEMS; k++) {
                if (g_item_recogido[k] == 0 && g_item_x[k] == g_pos_x_player1 && g_item_y[k] == g_pos_y_player1 && !player1_llego_meta && !player1_eliminado) {
                    aplicarEfectoItem(k, 1, -1);
                }
                if (g_item_recogido[k] == 0 && g_item_x[k] == g_pos_x_player2 && g_item_y[k] == g_pos_y_player2 && !player2_llego_meta && !player2_eliminado) {
                    aplicarEfectoItem(k, 2, -1);
                }
            }
            // Colision con obstaculos jugadores
            for (int k = 0; k < NUM_OBSTACULOS; k++) {
                if (g_obstaculo_x[k] == g_pos_x_player1 && g_obstaculo_y[k] == g_pos_y_player1 && g_inmunidad_player1 == 0 && !player1_llego_meta && !player1_eliminado) {
                    player1_llego_meta = true; // Eliminar jugador
                    player1_eliminado = true;
                    dibujarCaracter(-1, -1, "", 0, g_pos_x_player1, g_pos_y_player1, "[o1]");
                    gotoxy(0, MENSAJES_Y_OFFSET);
                    setColor(12);
                    printf("Jugador 1 eliminado por colision!                           ");
                    Sleep(2000);
                }
                if (g_obstaculo_x[k] == g_pos_x_player2 && g_obstaculo_y[k] == g_pos_y_player2 && g_inmunidad_player2 == 0 && !player2_llego_meta && !player2_eliminado) {
                    player2_llego_meta = true; // Eliminar jugador
                    player2_eliminado = true;
                    dibujarCaracter(-1, -1, "", 0, g_pos_x_player2, g_pos_y_player2, "[o2]");
                    gotoxy(0, MENSAJES_Y_OFFSET);
                    setColor(12);
                    printf("Jugador 2 eliminado por colision!                           ");
                    Sleep(2000);
                }
            }
            // Colision con obstaculos bots
            for (int b = 0; b < num_bots; b++) {
                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                    if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] && g_inmunidad_bots[b] == 0 && !bot_llego_meta[b] && !bot_eliminado[b]) {
                        bot_llego_meta[b] = true; // Eliminar bot
                        bot_eliminado[b] = true;
                        dibujarCaracter(-1, -1, "", 0, g_pos_x_bots[b], g_pos_y_bots[b], "[B]");
                        gotoxy(0, MENSAJES_Y_OFFSET);
                        setColor(12);
                        printf("Bot %d eliminado por colision!                           ", b + 1);
                        Sleep(1500);
                    }
                }
            }
            // Llegada a meta
            double tiempo_actual = (double)(clock() - start_time) / CLOCKS_PER_SEC;
            if (!player1_llego_meta && !player1_eliminado && g_pos_x_player1 >= META) {
                player1_llego_meta = true;
                strcpy(llegadas[total_llegadas].nombre, "Jugador 1");
                llegadas[total_llegadas].tiempo = tiempo_actual;
                total_llegadas++;
                victorias_j1++;
                derrotas_j2++;
            }
            if (!player2_llego_meta && !player2_eliminado && g_pos_x_player2 >= META) {
                player2_llego_meta = true;
                strcpy(llegadas[total_llegadas].nombre, "Jugador 2");
                llegadas[total_llegadas].tiempo = tiempo_actual;
                total_llegadas++;
                victorias_j2++;
                derrotas_j1++;
            }
            for (int b = 0; b < num_bots; b++) {
                if (!bot_llego_meta[b] && g_pos_x_bots[b] >= META) {
                    bot_llego_meta[b] = true;
                    char nombre_bot[32];
                    sprintf(nombre_bot, "Bot %d", b + 1);
                    strcpy(llegadas[total_llegadas].nombre, nombre_bot);
                    llegadas[total_llegadas].tiempo = tiempo_actual;
                    total_llegadas++;
                }
            }
            // Si todos han llegado a meta o sido eliminados, termina
            int total_participantes = 2 + num_bots;
            int terminados = 0; // Llegados + eliminados
            if (player1_llego_meta || player1_eliminado) terminados++;
            if (player2_llego_meta || player2_eliminado) terminados++;
            for (int b = 0; b < num_bots; b++) if (bot_llego_meta[b] || bot_eliminado[b]) terminados++; // <--- INSTRUCCION 5
            if (terminados == total_participantes) {
                limpiarPantalla();
                mostrarOrdenLlegada(llegadas, total_llegadas);
                printf("Fin de la carrera. Presiona 'R' para reiniciar o 'P' para salir.\n");
                while (1) {
                    if (_kbhit()) {
                        char c = _getch();
                        if (c == 'r' || c == 'R') {
                            opcion = 'r';
                            break;
                        } else if (c == 'p' || c == 'P') {
                            opcion = 'p';
                            break;
                        }
                    }
                    Sleep(100);
                }
                break;
            }
            // Permitir salir o reiniciar en cualquier momento
            if (_kbhit()) {
                char c = _getch();
                if (c == 'r' || c == 'R') {
                    opcion = 'r';
                    break;
                } else if (c == 'p' || c == 'P') {
                    opcion = 'p';
                    break;
                }
            }
            Sleep(30);
        }
    }
}

// --- FUNCIONES PARA EL MODO DE UN JUGADOR ---

void iniciarJuegoUnJugador() {
    char opcion = 'r';
    clock_t start_time;
    int old_pos_x, old_pos_y;
    Llegada llegadas[1 + MAX_BOTS];
    int total_llegadas = 0;
    int num_bots = 0;
    bool player_eliminado = false;
    do {
        limpiarPantalla();
        printf("--- Modo Un Jugador ---\n");
        printf("Cuantos bots quieres? (1-%d): ", MAX_BOTS);
        scanf("%d", &num_bots);
        while (num_bots < 1 || num_bots > MAX_BOTS) {
            printf("Numero de bots invalido. Por favor, introduce un valor entre 1 y %d: ", MAX_BOTS);
            scanf("%d", &num_bots);
        }
        fflush(stdin);

        while (opcion == 'r') {
            limpiarPantalla();
            g_pos_x_player1 = 0;
            g_pos_y_player1 = ALTO_PISTA / 2;
            g_inmunidad_player1 = 0;
            g_item_portado_player1 = -1;
            g_item_en_uso_player1 = false;
            total_llegadas = 0;
            player_eliminado = false;
            for (int l = 0; l < 1 + MAX_BOTS; ++l) llegadas[l].nombre[0] = '\0';
            srand((unsigned int)time(NULL));
            start_time = clock();
            organizarObstaculosYItems();
            dibujarPistaBase();
            mostrarMarcadores(false);
            old_pos_x = g_pos_x_player1; old_pos_y = g_pos_y_player1;
            for (int b = 0; b < MAX_BOTS; b++) {
                g_pos_x_bots[b] = 0;
                g_pos_y_bots[b] = (ALTO_PISTA / (max2(num_bots, 1) + 1)) * (b + 1);
                if (g_pos_y_bots[b] >= ALTO_PISTA) g_pos_y_bots[b] = ALTO_PISTA - 1;
                g_inmunidad_bots[b] = 0;
                g_bot_estado[b] = 0;
                g_item_portado_bots[b] = -1;
                g_item_en_uso_bots[b] = false;
            }
            bool player_llego_meta = false;
            bool bot_llego_meta[MAX_BOTS] = {false};
            bool bot_eliminado[MAX_BOTS] = {false}; // <--- AGREGADO SEGUN INSTRUCCION 6
            mostrarItemPortado(0, g_item_portado_player1, g_item_en_uso_player1);
            for (int b = 0; b < num_bots; b++) {
                mostrarItemPortadoBot(b, g_item_portado_bots[b], g_item_en_uso_bots[b]);
            }
            while (1) {
                gotoxy(0, 0);
                setColor(10);
                printf("Tiempo jugado: %.2f segundos", (double)(clock() - start_time) / CLOCKS_PER_SEC);
                if (g_inmunidad_player1 > 0) g_inmunidad_player1--;
                for (int b = 0; b < MAX_BOTS; b++) if (g_inmunidad_bots[b] > 0) g_inmunidad_bots[b]--;
                old_pos_x = g_pos_x_player1; old_pos_y = g_pos_y_player1;
                // Jugador
                if (!player_llego_meta && !player_eliminado) {
                    if (GetAsyncKeyState('D') & 0x8000) g_pos_x_player1 = min2(g_pos_x_player1 + 1, META);
                    if (GetAsyncKeyState('A') & 0x8000 && g_pos_x_player1 > 0) g_pos_x_player1--;
                    if (GetAsyncKeyState('W') & 0x8000 && g_pos_y_player1 > 0) g_pos_y_player1--;
                    if (GetAsyncKeyState('S') & 0x8000 && g_pos_y_player1 < ALTO_PISTA - 1) g_pos_y_player1++;
                    if (GetAsyncKeyState('Q') & 0x8000) {
                        usarItemPortado(&g_pos_x_player1, &g_pos_y_player1, &g_inmunidad_player1,
                                        &g_item_portado_player1, &g_item_en_uso_player1, 0, 1);
                        Sleep(100);
                    }
                }
                // Dibuja jugador
                if (!player_llego_meta && !player_eliminado)
                    dibujarCaracter(g_pos_x_player1, g_pos_y_player1, "[o1]", (g_inmunidad_player1 > 0 ? 14 : 9), old_pos_x, old_pos_y, "[o1]");
                else
                    dibujarCaracter(-1, -1, "", 0, old_pos_x, old_pos_y, "[o1]");
                // IA de bots y dibujo
                int old_pos_x_bots[MAX_BOTS], old_pos_y_bots[MAX_BOTS];
                for (int b = 0; b < MAX_BOTS; b++) {
                    old_pos_x_bots[b] = g_pos_x_bots[b];
                    old_pos_y_bots[b] = g_pos_y_bots[b];
                }
                for (int b = 0; b < num_bots; b++) {
                    if (g_bot_estado[b] == 0 && !bot_llego_meta[b] && !bot_eliminado[b]) { // <--- INSTRUCCION 6
                        // IA simple de bot
                        int next_x = g_pos_x_bots[b] + 1;
                        bool hay_obstaculo = false;
                        for (int k = 0; k < NUM_OBSTACULOS; k++) {
                            if (g_obstaculo_x[k] == next_x && g_obstaculo_y[k] == g_pos_y_bots[b]) {
                                hay_obstaculo = true;
                                break;
                            }
                        }
                        if (!hay_obstaculo) {
                            g_pos_x_bots[b] = min2(g_pos_x_bots[b] + 1, META);
                        } else {
                            // Intentar evitar obstaculo
                            bool movido = false;
                            if (g_pos_y_bots[b] > 0) {
                                bool libre_arriba = true;
                                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                    if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] - 1) {
                                        libre_arriba = false;
                                        break;
                                    }
                                }
                                if (libre_arriba) {
                                    g_pos_y_bots[b]--;
                                    movido = true;
                                }
                            }
                            if (!movido && g_pos_y_bots[b] < ALTO_PISTA - 1) {
                                bool libre_abajo = true;
                                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                                    if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] + 1) {
                                        libre_abajo = false;
                                        break;
                                    }
                                }
                                if (libre_abajo) {
                                    g_pos_y_bots[b]++;
                                    movido = true;
                                }
                            }
                            // Si no puede moverse, se queda en su lugar
                        }
                        // Recoger item si hay
                        for (int k = 0; k < NUM_ITEMS; k++) {
                            if (g_item_recogido[k] == 0 && g_item_x[k] == g_pos_x_bots[b] && g_item_y[k] == g_pos_y_bots[b]) {
                                aplicarEfectoItem(k, 0, b);
                                break;
                            }
                        }
                        // Usar item aleatoriamente
                        if (g_item_portado_bots[b] != -1 && !g_item_en_uso_bots[b]) {
                            if (rand() % 30 == 0) { // Probabilidad de usar item
                                usarItemPortadoBot(&g_pos_x_bots[b], &g_pos_y_bots[b], &g_inmunidad_bots[b], &g_item_portado_bots[b], &g_item_en_uso_bots[b], b);
                            }
                        }
                    }
                    // Dibuja bot
                    if (!bot_llego_meta[b] && !bot_eliminado[b]) // <--- INSTRUCCION 6
                        dibujarCaracter(g_pos_x_bots[b], g_pos_y_bots[b], "[B]", (g_inmunidad_bots[b] > 0 ? 14 : 11), old_pos_x_bots[b], old_pos_y_bots[b], "[B]");
                    else
                        dibujarCaracter(-1, -1, "", 0, old_pos_x_bots[b], old_pos_y_bots[b], "[B]");
                }
                // Recoger items jugador
                for (int k = 0; k < NUM_ITEMS; k++) {
                    if (g_item_recogido[k] == 0 && g_item_x[k] == g_pos_x_player1 && g_item_y[k] == g_pos_y_player1 && !player_llego_meta && !player_eliminado) {
                        aplicarEfectoItem(k, 1, -1);
                    }
                }
                // Colision con obstaculos jugador
                for (int k = 0; k < NUM_OBSTACULOS; k++) {
                    if (g_obstaculo_x[k] == g_pos_x_player1 && g_obstaculo_y[k] == g_pos_y_player1 && g_inmunidad_player1 == 0 && !player_llego_meta && !player_eliminado) {
                        player_llego_meta = true;
                        player_eliminado = true;
                        dibujarCaracter(-1, -1, "", 0, g_pos_x_player1, g_pos_y_player1, "[o1]");
                        gotoxy(0, MENSAJES_Y_OFFSET);
                        setColor(12);
                        printf("Jugador eliminado por colision!                           ");
                        Sleep(2000);
                    }
                }
                // Colision con obstaculos bots
                for (int b = 0; b < num_bots; b++) {
                    for (int k = 0; k < NUM_OBSTACULOS; k++) {
                        if (g_obstaculo_x[k] == g_pos_x_bots[b] && g_obstaculo_y[k] == g_pos_y_bots[b] && g_inmunidad_bots[b] == 0 && !bot_llego_meta[b] && !bot_eliminado[b]) {
                            bot_llego_meta[b] = true; // Eliminar bot
                            bot_eliminado[b] = true;
                            dibujarCaracter(-1, -1, "", 0, g_pos_x_bots[b], g_pos_y_bots[b], "[B]");
                            gotoxy(0, MENSAJES_Y_OFFSET);
                            setColor(12);
                            printf("Bot %d eliminado por colision!                           ", b + 1);
                            Sleep(1500);
                        }
                    }
                }
                // Llegada a meta
                double tiempo_actual = (double)(clock() - start_time) / CLOCKS_PER_SEC;
                if (!player_llego_meta && !player_eliminado && g_pos_x_player1 >= META) {
                    player_llego_meta = true;
                    strcpy(llegadas[total_llegadas].nombre, "Jugador");
                    llegadas[total_llegadas].tiempo = tiempo_actual;
                    total_llegadas++;
                    victorias_jugador_un_jugador++;
                }
                for (int b = 0; b < num_bots; b++) {
                    if (!bot_llego_meta[b] && g_pos_x_bots[b] >= META) {
                        bot_llego_meta[b] = true;
                        char nombre_bot[32];
                        sprintf(nombre_bot, "Bot %d", b + 1);
                        strcpy(llegadas[total_llegadas].nombre, nombre_bot);
                        llegadas[total_llegadas].tiempo = tiempo_actual;
                        total_llegadas++;
                    }
                }
                // Si todos han llegado a meta o sido eliminados, termina
                int total_participantes = 1 + num_bots;
                int terminados = 0;
                if (player_llego_meta || player_eliminado) terminados++;
                for (int b = 0; b < num_bots; b++) if (bot_llego_meta[b] || bot_eliminado[b]) terminados++; // <--- INSTRUCCION 6
                if (terminados == total_participantes) {
                    limpiarPantalla();
                    mostrarOrdenLlegada(llegadas, total_llegadas);
                    printf("Fin de la carrera. Presiona 'R' para reiniciar o 'P' para salir.\n");
                    while (1) {
                        if (_kbhit()) {
                            char c = _getch();
                            if (c == 'r' || c == 'R') {
                                opcion = 'r';
                                break;
                            } else if (c == 'p' || c == 'P') {
                                opcion = 'p';
                                break;
                            }
                        }
                        Sleep(100);
                    }
                    break;
                }
                // Permitir salir o reiniciar en cualquier momento
                if (_kbhit()) {
                    char c = _getch();
                    if (c == 'r' || c == 'R') {
                        opcion = 'r';
                        break;
                    } else if (c == 'p' || c == 'P') {
                        opcion = 'p';
                        break;
                    }
                }
                Sleep(30);
            }
        }
    } while (opcion == 'r');
}

// --- MENU PRINCIPAL ---

int main() {
    SetConsoleTitleA("Juego del Carro - Carrera");
    while (true) {
        limpiarPantalla();
        setColor(15);
        printf("========================================\n");
        printf("         JUEGO DEL CARRO - CARRERA      \n");
        printf("========================================\n");
        printf("1. Modo Un Jugador\n");
        printf("2. Modo Dos Jugadores\n");
        printf("3. Salir\n");
        printf("Selecciona una opcion: ");
        char opcion = _getch();
        if (opcion == '1') {
            iniciarJuegoUnJugador();
        } else if (opcion == '2') {
            iniciarJuegoDosJugadores();
        } else if (opcion == '3') {
            limpiarPantalla();
            printf("Gracias por jugar!\n");
            Sleep(1000);
            break;
        } else {
            printf("\nOpcion invalida. Presiona una tecla para continuar...\n");
            _getch();
        }
    }
    return 0;
}
