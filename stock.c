#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_ALERTS 3
#define MIN_STOCK 2
#define BUFF_SIZE 256
#define MAX_CMD_ITEMS 50 // Sécurité pour le tableau temporaire

typedef struct Product {
    char type; 
    int volume;
    struct Product* next;
} Product;

typedef struct {
    char message[20];
    int active;
} AlertLog;

/* --- Variables globales --- */
Product* stock_head = NULL;
AlertLog alert_system[MAX_ALERTS];
int alert_index = 0;

/* --- Fonctions Utilitaires --- */

Product* create_node(char type, int volume) {
    Product* new_p = (Product*)malloc(sizeof(Product));
    // SÉCURITÉ : Vérification de l'allocation mémoire
    if (new_p == NULL) {
        fprintf(stderr, "Erreur critique : Memoire insuffisante.\n");
        exit(EXIT_FAILURE);
    }
    new_p->type = type; 
    new_p->volume = volume; 
    new_p->next = NULL;
    return new_p;
}

void unlink_node(Product* prev, Product* curr) {
    if (prev) prev->next = curr->next;
    else stock_head = curr->next;
    curr->next = NULL;
}

/* SERVICE : GESTION DES ALERTES (MONITORING)
   Stratégie : Buffer Circulaire (On écrase les vieilles alertes si plein)
   [cite_start][cite: 50, 134]
*/

void Enregistrer_Alerte(char type, int volume) {
    snprintf(alert_system[alert_index].message, 20, "%c%d", type, volume);
    alert_system[alert_index].active = 1;
    printf(">> ALARME : Stock faible %s\n", alert_system[alert_index].message);
    
    // Rotation du buffer circulaire
    alert_index = (alert_index + 1) % MAX_ALERTS; 
}

int count_stock(char type, int volume); // Prototype

void Verifier_Seuil(char type, int volume) {
    [cite_start]// [cite: 77, 145]
    if (count_stock(type, volume) < MIN_STOCK) 
        Enregistrer_Alerte(type, volume);
}

void Afficher_Une_Alerte(int i) {
    int idx = (alert_index + i) % MAX_ALERTS;
    if (alert_system[idx].active)
        printf("[Alert] Rupture : %s\n", alert_system[idx].message);
}

void Afficher_Log_Alertes() {
    printf("\n--- LOG ALERTES ---\n");
    [cite_start]// [cite: 79]
    for (int i = 0; i < MAX_ALERTS; i++) Afficher_Une_Alerte(i);
    printf("-------------------\n");
}

/* SERVICE : GESTION DES ENTRÉES (APPROVISIONNEMENT)
   [cite_start][cite: 61]
*/

int count_stock(char type, int volume) {
    int count = 0;
    Product* cur = stock_head;
    while (cur) {
        if (cur->type == type && cur->volume == volume) count++;
        cur = cur->next;
    }
    return count;
}

void Saisir_Produit_Unitaire(char type, int volume) {
    Product* new_p = create_node(type, volume);
    // Insertion en queue pour respecter l'ordre d'arrivée (FIFO)
    if (!stock_head) stock_head = new_p;
    else {
        Product* cur = stock_head;
        while (cur->next) cur = cur->next;
        cur->next = new_p;
    }
}

void Traiter_Token_Ajout(char* token) {
    if (strlen(token) >= 2)
        Saisir_Produit_Unitaire(toupper(token[0]), atoi(&token[1]));
}

void Importer_Paquet_Rapide(char* str) {
    char* token = strtok(str, ", ");
    while (token) {
        Traiter_Token_Ajout(token);
        token = strtok(NULL, ", ");
    }
    printf("Succes : Ajout termine.\n");
}

/* SERVICE : GESTION DES SORTIES (ASSEMBLAGE COLIS)
   Logique : FIFO pour le stock, LIFO trié pour le colis
   [cite_start][cite: 88, 92]
*/

Product* find_fifo(char t, int v, Product** prev_out) {
    Product *cur = stock_head, *prev = NULL;
    while (cur) {
        // On cherche le premier correspondant (le plus ancien)
        if (cur->type == t && cur->volume == v) {
            *prev_out = prev; 
            return cur;
        }
        prev = cur; 
        cur = cur->next;
    }
    return NULL;
}

Product* Extraire_Du_Stock(char t, int v) {
    Product *prev = NULL, *target = find_fifo(t, v, &prev);
    if (target) {
        unlink_node(prev, target);
        Verifier_Seuil(t, v); // Vérification après retrait
        return target;
    }
    return NULL;
}

Product* Gerer_Rupture(char t, int v) {
    Product* p = Extraire_Du_Stock(t, v);
    if (!p) {
        printf("!! RUPTURE !! %c%d -> Backorder.\n", t, v);
        [cite_start]// En cas de rupture, on génère une alerte [cite: 45]
        Enregistrer_Alerte(t, v);
    }
    return p;
}

void Swap_Ptr(Product** a, Product** b) {
    Product* temp = *a; *a = *b; *b = temp;
}

void Trier_Buffer_Decroissant(Product** items, int count) {
    // Tri pour mettre les petits volumes en fin de tableau (Haut du colis)
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - i - 1; j++)
            if (items[j]->volume < items[j+1]->volume)
                Swap_Ptr(&items[j], &items[j+1]);
}

void Empiler_Et_Afficher(Product** items, int count) {
    printf("\n--- COLIS (Haut -> Fond) ---\n");
    // BUG CORRIGÉ : boucle jusqu'à >= 0 pour inclure le dernier élément
    for (int i = count - 1; i >= 0; i--) {
        printf("| Haut : Type %c | Vol %d |\n", items[i]->type, items[i]->volume);
        free(items[i]); // Libération mémoire
    }
    printf("| Fond --------------------|\n");
}

void Assembler_Colis(Product** items, int count) {
    Trier_Buffer_Decroissant(items, count);
    Empiler_Et_Afficher(items, count);
}

void Traiter_Commande(char* cmd) {
    Product* tmp[MAX_CMD_ITEMS]; 
    int cnt = 0;
    char* token = strtok(cmd, ", ");
    
    while (token) {
        if (strlen(token) >= 2) {
            // SÉCURITÉ : Protection contre le Buffer Overflow
            if (cnt >= MAX_CMD_ITEMS) {
                printf("! Attention : Commande tronquée (max %d items)\n", MAX_CMD_ITEMS);
                break; 
            }
            
            Product* p = Gerer_Rupture(toupper(token[0]), atoi(&token[1]));
            if (p) tmp[cnt++] = p;
        }
        token = strtok(NULL, ", ");
    }
    if (cnt > 0) Assembler_Colis(tmp, cnt);
}

/* --- MAIN --- */

void Demo_Init() {
    Saisir_Produit_Unitaire('A', 1); Saisir_Produit_Unitaire('B', 2);
    Saisir_Produit_Unitaire('C', 10); Saisir_Produit_Unitaire('A', 3);
}

void Menu_Action(int choice, char* buf) {
    if (choice == 3) { Afficher_Log_Alertes(); return; }
    
    printf("Saisie (ex: A1, B2): ");
    fgets(buf, BUFF_SIZE, stdin);
    buf[strcspn(buf, "\n")] = 0; // Suppression du saut de ligne

    if (choice == 1) Importer_Paquet_Rapide(buf);
    else if (choice == 2) Traiter_Commande(buf);
}

int main() {
    int c = 0; 
    char buf[BUFF_SIZE];
    
    Demo_Init();
    printf("=== GESTION DE STOCK 2025 ===\n");
    
    do {
        printf("\n1.Ajout 2.Colis 3.Alertes 0.Fin\nChoix: ");
        if(scanf("%d", &c) && c != 0) {
            // SÉCURITÉ : Vider correctement le buffer clavier
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
            
            Menu_Action(c, buf);
        }
    } while (c != 0);
    
    return 0;
}