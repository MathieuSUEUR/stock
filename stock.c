#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* --- CONSTANTES (Pour éviter les "Magic Numbers") --- */
#define MAX_ALERTS 3
#define MIN_STOCK 2
#define BUFF_SIZE 256
#define MAX_CMD_ITEMS 50
#define MSG_SIZE 20
#define BASE_DEC 10  /* Base décimale pour strtol */

/* --- STRUCTURES --- */
typedef struct Product {
    char type; 
    int volume;
    struct Product* next;
} Product;

typedef struct {
    char message[MSG_SIZE];
    int active;
} AlertLog;

/* --- VARIABLES GLOBALES (Static pour l'encapsulation) --- */
static Product* stock_head = NULL;
static AlertLog alert_system[MAX_ALERTS];
static int alert_index = 0;

/* --- GESTION MEMOIRE --- */

static void Nettoyer_Memoire(void) {
    Product* current = stock_head;
    Product* next_node = NULL;
    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
    stock_head = NULL;
}

static Product* create_node(char type, int volume) {
    /* CORRECTION VULN 3 : calloc met la mémoire à zéro (plus sûr que malloc) */
    Product* new_p = (Product*)calloc(1, sizeof(Product));
    
    if (new_p == NULL) {
        (void)fprintf(stderr, "Erreur critique : Memoire insuffisante.\n");
        exit(EXIT_FAILURE);
    }
    new_p->type = type; 
    new_p->volume = volume; 
    new_p->next = NULL;
    return new_p;
}

static void unlink_node(Product* prev, Product* curr) {
    if (prev) {
        prev->next = curr->next;
    } else {
        stock_head = curr->next;
    }
    curr->next = NULL;
}

/* --- SERVICE : ALERTES --- */

static void Enregistrer_Alerte(char type, int volume) {
    /* Sécurisation de la chaîne formatée */
    (void)snprintf(alert_system[alert_index].message, MSG_SIZE, "%c%d", type, volume);
    
    /* Garantie de la terminaison nulle (Paranoïa pour analyseurs statiques) */
    alert_system[alert_index].message[MSG_SIZE - 1] = '\0';
    
    alert_system[alert_index].active = 1;
    (void)printf(">> ALARME : Stock faible %s\n", alert_system[alert_index].message);
    
    alert_index = (alert_index + 1) % MAX_ALERTS; 
}

/* Prototype */
static int count_stock(char type, int volume); 

static void Verifier_Seuil(char type, int volume) {
    if (count_stock(type, volume) < MIN_STOCK) {
        Enregistrer_Alerte(type, volume);
    }
}

static void Afficher_Log_Alertes(void) {
    int i;
    (void)printf("\n--- LOG ALERTES ---\n");
    for (i = 0; i < MAX_ALERTS; i++) {
        int idx = (alert_index + i) % MAX_ALERTS;
        if (alert_system[idx].active) {
            (void)printf("[Alert] Rupture : %s\n", alert_system[idx].message);
        }
    }
    (void)printf("-------------------\n");
}

/* --- SERVICE : ENTREES --- */

/* CORRECTION CODE ISSUE : 'const' ajouté car on ne modifie pas les données */
static int count_stock(char type, int volume) {
    int count = 0;
    const Product* cur = stock_head;
    while (cur) {
        if (cur->type == type && cur->volume == volume) {
            count++;
        }
        cur = cur->next;
    }
    return count;
}

static void Saisir_Produit_Unitaire(char type, int volume) {
    Product* new_p = create_node(type, volume);
    
    if (!stock_head) {
        stock_head = new_p;
    } else {
        Product* cur = stock_head;
        while (cur->next) {
            cur = cur->next;
        }
        cur->next = new_p;
    }
}

/* Fonction auxiliaire pour parser un token (ex: "A1") */
static void Traiter_Token_Brut(const char* token_start, size_t length) {
    if (length >= 2) {
        char type_char = token_start[0];
        char* endptr = NULL;
        /* On copie la partie nombre dans un buffer temporaire pour strtol */
        char vol_buffer[16] = {0}; 
        int volume = 0;

        if (length - 1 < sizeof(vol_buffer)) {
            memcpy(vol_buffer, &token_start[1], length - 1);
            vol_buffer[length - 1] = '\0';
            
            volume = (int)strtol(vol_buffer, &endptr, BASE_DEC);

            if (volume > 0) {
                /* CORRECTION VULN 2 : Cast (unsigned char) pour toupper */
                Saisir_Produit_Unitaire((char)toupper((unsigned char)type_char), volume);
            }
        }
    }
}

/* CORRECTION VULN 1 : Remplacement de strtok par une boucle manuelle (Thread-safe) */
static void Importer_Paquet_Rapide(const char* input_str) {
    const char* cursor = input_str;
    const char* delims = ", ";
    
    while (*cursor != '\0') {
        /* Sauter les délimiteurs au début */
        size_t span = strspn(cursor, delims);
        cursor += span;

        if (*cursor == '\0') break;

        /* Trouver la fin du token */
        size_t token_len = strcspn(cursor, delims);
        
        /* Traiter le token */
        Traiter_Token_Brut(cursor, token_len);

        cursor += token_len;
    }
    (void)printf("Succes : Ajout termine.\n");
}

/* --- SERVICE : SORTIES --- */

static Product* find_fifo(char t, int v, Product** prev_out) {
    Product *cur = stock_head;
    Product *prev = NULL;
    
    while (cur) {
        if (cur->type == t && cur->volume == v) {
            *prev_out = prev; 
            return cur;
        }
        prev = cur; 
        cur = cur->next;
    }
    return NULL;
}

static Product* Extraire_Du_Stock(char t, int v) {
    Product *prev = NULL;
    Product *target = find_fifo(t, v, &prev);
    
    if (target) {
        unlink_node(prev, target);
        Verifier_Seuil(t, v);
        return target;
    }
    return NULL;
}

static Product* Gerer_Rupture(char t, int v) {
    Product* p = Extraire_Du_Stock(t, v);
    if (!p) {
        (void)printf("!! RUPTURE !! %c%d -> Backorder.\n", t, v);
        Enregistrer_Alerte(t, v);
    }
    return p;
}

static void Swap_Ptr(Product** a, Product** b) {
    Product* temp = *a; 
    *a = *b; 
    *b = temp;
}

static void Assembler_Colis(Product** items, int count) {
    int i, j;
    
    /* Tri à bulles décroissant */
    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - i - 1; j++) {
            if (items[j]->volume < items[j+1]->volume) {
                Swap_Ptr(&items[j], &items[j+1]);
            }
        }
    }

    (void)printf("\n--- COLIS (Haut -> Fond) ---\n");
    for (i = count - 1; i >= 0; i--) {
        (void)printf("| Haut : Type %c | Vol %d |\n", items[i]->type, items[i]->volume);
        free(items[i]); 
    }
    (void)printf("| Fond --------------------|\n");
}

/* Même logique de parsing manuel pour éviter strtok */
static void Traiter_Commande(const char* cmd) {
    Product* tmp[MAX_CMD_ITEMS]; 
    int cnt = 0;
    const char* cursor = cmd;
    const char* delims = ", ";

    while (*cursor != '\0') {
        size_t span = strspn(cursor, delims);
        cursor += span;
        if (*cursor == '\0') break;

        size_t token_len = strcspn(cursor, delims);
        
        /* Extraction locale pour commande */
        if (token_len >= 2 && cnt < MAX_CMD_ITEMS) {
             char type_char = cursor[0];
             char vol_buffer[16] = {0};
             if (token_len - 1 < sizeof(vol_buffer)) {
                 memcpy(vol_buffer, &cursor[1], token_len - 1);
                 vol_buffer[token_len - 1] = '\0';
                 
                 int vol = (int)strtol(vol_buffer, NULL, BASE_DEC);
                 if (vol > 0) {
                     Product* p = Gerer_Rupture((char)toupper((unsigned char)type_char), vol);
                     if (p) tmp[cnt++] = p;
                 }
             }
        }
        cursor += token_len;
    }
    
    if (cnt > 0) {
        Assembler_Colis(tmp, cnt);
    }
}

/* --- MAIN --- */

static void Demo_Init(void) {
    Saisir_Produit_Unitaire('A', 1); 
    Saisir_Produit_Unitaire('B', 2);
    Saisir_Produit_Unitaire('C', 10); 
    Saisir_Produit_Unitaire('A', 3);
}

static void Menu_Action(int choice, char* buf) {
    if (choice == 3) { 
        Afficher_Log_Alertes(); 
        return; 
    }
    
    (void)printf("Saisie (ex: A1, B2): ");
    /* Vérification stricte du retour de fgets */
    if (fgets(buf, BUFF_SIZE, stdin) != NULL) {
        buf[strcspn(buf, "\n")] = 0;

        if (choice == 1) {
            Importer_Paquet_Rapide(buf);
        } else if (choice == 2) {
            Traiter_Commande(buf);
        }
    }
}

static void Vider_Buffer_Clavier(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

int main(void) {
    int c = 0; 
    char buf[BUFF_SIZE] = {0}; /* Initialisation à zéro */
    
    Demo_Init();
    (void)printf("=== GESTION DE STOCK 2025 (Secure) ===\n");
    
    do {
        (void)printf("\n1.Ajout 2.Colis 3.Alertes 0.Fin\nChoix: ");
        
        if(scanf("%d", &c) == 1 && c != 0) {
            Vider_Buffer_Clavier();
            Menu_Action(c, buf);
        } else {
             if (c != 0) Vider_Buffer_Clavier();
        }
        
    } while (c != 0);
    
    (void)printf("Arrêt du système. Nettoyage mémoire...\n");
    Nettoyer_Memoire();
    
    return 0;
}