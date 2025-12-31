#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* CONSTANTES ET MACROS */
#define MAX_ALERTS 3
#define MIN_STOCK 2
#define BUFF_SIZE 256
#define MAX_CMD_ITEMS 50
#define MSG_SIZE 20  /* Correction "Magic Number" */

/* STRUCTURES */
typedef struct Product {
    char type; 
    int volume;
    struct Product* next;
} Product;

typedef struct {
    char message[MSG_SIZE];
    int active;
} AlertLog;

/* VARIABLES GLOBALES (STATIC pour limiter la portée - Correction Embold) */
static Product* stock_head = NULL;
static AlertLog alert_system[MAX_ALERTS];
static int alert_index = 0;

/* --- GESTION MEMOIRE & UTILITAIRES --- */

/* Correction : Fonction pour libérer toute la mémoire à la fin */
static void Nettoyer_Memoire() {
    Product* current = stock_head;
    Product* next_node;
    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
    stock_head = NULL;
}

static Product* create_node(char type, int volume) {
    Product* new_p = (Product*)malloc(sizeof(Product));
    if (new_p == NULL) {
        fprintf(stderr, "Erreur critique : Memoire insuffisante.\n");
        /* En cas de crash, on essaie de nettoyer si possible, ou on quitte */
        exit(EXIT_FAILURE);
    }
    new_p->type = type; 
    new_p->volume = volume; 
    new_p->next = NULL;
    return new_p;
}

static void unlink_node(Product* prev, Product* curr) {
    if (prev) prev->next = curr->next;
    else stock_head = curr->next;
    curr->next = NULL;
}

/* --- SERVICE : ALERTES --- */

static void Enregistrer_Alerte(char type, int volume) {
    /* Utilisation de MSG_SIZE au lieu de 20 */
    snprintf(alert_system[alert_index].message, MSG_SIZE, "%c%d", type, volume);
    alert_system[alert_index].active = 1;
    printf(">> ALARME : Stock faible %s\n", alert_system[alert_index].message);
    
    alert_index = (alert_index + 1) % MAX_ALERTS; 
}

/* Prototype nécessaire pour la récursion croisée */
static int count_stock(char type, int volume); 

static void Verifier_Seuil(char type, int volume) {
    if (count_stock(type, volume) < MIN_STOCK) 
        Enregistrer_Alerte(type, volume);
}

static void Afficher_Log_Alertes() {
    printf("\n--- LOG ALERTES ---\n");
    for (int i = 0; i < MAX_ALERTS; i++) {
        int idx = (alert_index + i) % MAX_ALERTS;
        if (alert_system[idx].active)
            printf("[Alert] Rupture : %s\n", alert_system[idx].message);
    }
    printf("-------------------\n");
}

/* --- SERVICE : ENTREES --- */

static int count_stock(char type, int volume) {
    int count = 0;
    Product* cur = stock_head;
    while (cur) {
        if (cur->type == type && cur->volume == volume) count++;
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
        while (cur->next) cur = cur->next;
        cur->next = new_p;
    }
}

static void Traiter_Token_Ajout(char* token) {
    if (strlen(token) >= 2) {
        /* Correction : strtol est plus sûr que atoi */
        char* endptr;
        int vol = (int)strtol(&token[1], &endptr, 10);
        
        /* Vérification basique que le volume est valide */
        if (vol > 0) {
            Saisir_Produit_Unitaire(toupper((unsigned char)token[0]), vol);
        }
    }
}

static void Importer_Paquet_Rapide(char* str) {
    /* Note: strtok modifie la chaîne d'origine, c'est acceptable ici */
    char* token = strtok(str, ", ");
    while (token) {
        Traiter_Token_Ajout(token);
        token = strtok(NULL, ", ");
    }
    printf("Succes : Ajout termine.\n");
}

/* --- SERVICE : SORTIES --- */

static Product* find_fifo(char t, int v, Product** prev_out) {
    Product *cur = stock_head, *prev = NULL;
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
        printf("!! RUPTURE !! %c%d -> Backorder.\n", t, v);
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
    /* Tri à bulles : Plus petits volumes vers la fin du tableau */
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (items[j]->volume < items[j+1]->volume) {
                Swap_Ptr(&items[j], &items[j+1]);
            }
        }
    }

    /* Affichage LIFO (Haut = Fin du tableau) */
    printf("\n--- COLIS (Haut -> Fond) ---\n");
    for (int i = count - 1; i >= 0; i--) {
        printf("| Haut : Type %c | Vol %d |\n", items[i]->type, items[i]->volume);
        free(items[i]); /* Libération impérative ici */
    }
    printf("| Fond --------------------|\n");
}

static void Traiter_Commande(char* cmd) {
    Product* tmp[MAX_CMD_ITEMS]; 
    int cnt = 0;
    char* token = strtok(cmd, ", ");
    
    while (token) {
        if (strlen(token) >= 2) {
            if (cnt >= MAX_CMD_ITEMS) {
                printf("! Commande tronquee (max %d)\n", MAX_CMD_ITEMS);
                break; 
            }
            
            char* endptr;
            int vol = (int)strtol(&token[1], &endptr, 10);
            
            if (vol > 0) {
                /* Cast unsigned char pour toupper pour éviter bugs d'index négatifs */
                Product* p = Gerer_Rupture(toupper((unsigned char)token[0]), vol);
                if (p) tmp[cnt++] = p;
            }
        }
        token = strtok(NULL, ", ");
    }
    if (cnt > 0) Assembler_Colis(tmp, cnt);
}

/* --- MAIN --- */

static void Demo_Init() {
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
    
    printf("Saisie (ex: A1, B2): ");
    if (fgets(buf, BUFF_SIZE, stdin) != NULL) {
        buf[strcspn(buf, "\n")] = 0;

        if (choice == 1) Importer_Paquet_Rapide(buf);
        else if (choice == 2) Traiter_Commande(buf);
    }
}

int main() {
    int c = 0; 
    char buf[BUFF_SIZE];
    
    /* Initialisation sécurisée du buffer */
    memset(buf, 0, BUFF_SIZE);

    Demo_Init();
    printf("=== GESTION DE STOCK 2025 (Clean) ===\n");
    
    do {
        printf("\n1.Ajout 2.Colis 3.Alertes 0.Fin\nChoix: ");
        if(scanf("%d", &c) == 1 && c != 0) {
            /* Vider le buffer clavier proprement */
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
            
            Menu_Action(c, buf);
        } else {
             /* Si scanf échoue (ex: lettre tapée), on vide le buffer pour éviter boucle infinie */
             if (c != 0) {
                 int ch;
                 while ((ch = getchar()) != '\n' && ch != EOF);
             }
        }
    } while (c != 0);
    
    /* NETTOYAGE FINAL (Important pour Embold) */
    printf("Arrêt du système. Nettoyage mémoire...\n");
    Nettoyer_Memoire();
    
    return 0;
}