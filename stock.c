#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define MAX_ALERTS 3
#define MIN_STOCK 2
#define BUFF_SIZE 256


typedef struct Product {
    char type; int volume;
    struct Product* next;
} Product;


typedef struct {
    char message[20];
    int active;
} AlertLog;


/*Variables globales*/
Product* stock_head = NULL;
AlertLog alert_system[MAX_ALERTS];
int alert_index = 0;


Product* create_node(char type, int volume) {
    Product* new_p = (Product*)malloc(sizeof(Product));
    new_p->type = type; new_p->volume = volume; new_p->next = NULL;
    return new_p;
}


void unlink_node(Product* prev, Product* curr) {
    if (prev) prev->next = curr->next;
    else stock_head = curr->next;
    curr->next = NULL;
}


/*
SERVICE : GESTION DES ALERTES (MONITORING)
Valeur ajoutée: Ce service va garantir la continuité de l'exploitation en signalant les niveaux de stocks critiques sans jamais bloquer le système (Stratégie Buffer Circulaire).
*/


void Enregistrer_Alerte(char type, int volume) {
    snprintf(alert_system[alert_index].message, 20, "%c%d", type, volume);
    alert_system[alert_index].active = 1;
    printf(">> ALARME : Stock faible %s\n", alert_system[alert_index].message);
    alert_index = (alert_index + 1) % MAX_ALERTS;
}


int count_stock(char type, int volume); // Proto
void Verifier_Seuil(char type, int volume) {
    if (count_stock(type, volume) < MIN_STOCK) Enregistrer_Alerte(type, volume);
}


void Afficher_Une_Alerte(int i) {
    int idx = (alert_index + i) % MAX_ALERTS;
    if (alert_system[idx].active)
        printf("[Alert] Rupture : %s\n", alert_system[idx].message);
}


void Afficher_Log_Alertes() {
    printf("\n--- LOG ALERTES ---\n");
    for (int i = 0; i < MAX_ALERTS; i++) Afficher_Une_Alerte(i);
    printf("-------------------\n");
}


/*
SERVICE : GESTION DES ENTRÉES (APPROVISIONNEMENT)
 Valeur ajoutée: Ce service va assurer une mise en stock rapide et fiable, et respecter l'ordre d'arrivée pour préparer la gestion FIFO.
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


/*
SERVICE : GESTION DES SORTIES (ASSEMBLAGE COLIS)
Valeur ajoutée: Ce service va optimiser le transport et minimiser les pertes financières grâce à une rotation correcte des produits périssables (FIFO).
*/


Product* find_fifo(char t, int v, Product** prev_out) {
    Product *cur = stock_head, *prev = NULL;
    while (cur) {
        if (cur->type == t && cur->volume == v) {
            *prev_out = prev; return cur;
        }
        prev = cur; cur = cur->next;
    }
    return NULL;
}


Product* Extraire_Du_Stock(char t, int v) {
    Product *prev = NULL, *target = find_fifo(t, v, &prev);
    if (target) {
        unlink_node(prev, target);
        Verifier_Seuil(t, v);
        return target;
    }
    return NULL;
}


Product* Gerer_Rupture(char t, int v) {
    Product* p = Extraire_Du_Stock(t, v);
    if (!p) {
        printf("!! RUPTURE !! %c%d -> Backorder.\n", t, v);
        Enregistrer_Alerte(t, v);
    }
    return p;
}


void Swap_Ptr(Product** a, Product** b) {
    Product* temp = *a; *a = *b; *b = temp;
}


void Trier_Buffer_Decroissant(Product** items, int count) {
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - i - 1; j++)
            if (items[j]->volume < items[j+1]->volume)
                Swap_Ptr(&items[j], &items[j+1]);
}


void Empiler_Et_Afficher(Product** items, int count) {
    printf("\n--- COLIS (Haut -> Fond) ---\n");
    for (int i = count - 1; i >= 0; i--) {
        printf("| Haut : Type %c | Vol %d |\n", items[i]->type, items[i]->volume);
        free(items[i]);
    }
    printf("| Fond --------------------|\n");
}


void Assembler_Colis(Product** items, int count) {
    Trier_Buffer_Decroissant(items, count);
    Empiler_Et_Afficher(items, count);
}


void Traiter_Commande(char* cmd) {
    Product* tmp[50]; int cnt = 0;
    char* token = strtok(cmd, ", ");
    while (token) {
        if (strlen(token) >= 2) {
            Product* p = Gerer_Rupture(toupper(token[0]), atoi(&token[1]));
            if (p) tmp[cnt++] = p;
        }
        token = strtok(NULL, ", ");
    }
    if (cnt > 0) Assembler_Colis(tmp, cnt);
}


/*MAIN*/
void Demo_Init() {
    Saisir_Produit_Unitaire('A', 1); Saisir_Produit_Unitaire('B', 2);
    Saisir_Produit_Unitaire('C', 10); Saisir_Produit_Unitaire('A', 3);
}


void Menu_Action(int choice, char* buf) {
    if (choice == 3) { Afficher_Log_Alertes(); return; }
   
    printf("Saisie (ex: A1, B2): ");
    fgets(buf, BUFF_SIZE, stdin);
    buf[strcspn(buf, "\n")] = 0;


    if (choice == 1) Importer_Paquet_Rapide(buf);
    else if (choice == 2) Traiter_Commande(buf);
}


int main() {
    int c = 0; char buf[BUFF_SIZE];
    Demo_Init();
    printf("=== GESTION DE STOCK 2025 ===\n");
    do {
        printf("\n1.Ajout 2.Colis 3.Alertes 0.Fin\nChoix: ");
        if(scanf("%d", &c) && c != 0) {
            getchar();
            Menu_Action(c, buf);
        }
    } while (c != 0);
    return 0;
}
