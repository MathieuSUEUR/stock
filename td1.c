#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXTOK 100
#define STACK_SIZE 100

/* Lit la première ligne du fichier donné, la stocke dans le buffer et retire le saut de ligne final. */
int lecture_expression(const char *filename, char *line, size_t size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Erreur : Impossible de lire le fichier '%s'.\n", filename);
        return 0;
    }
    if (!fgets(line, size, file)) {
        printf("Erreur : Le fichier est vide.\n");
        fclose(file);
        return 0;
    }
    line[strcspn(line, "\n")] = 0;
    fclose(file);
    return 1;
}

/* Parcourt tous les tokens pour vérifier s'ils sont soit des opérateurs valides (+, -, *, /), soit des nombres réels. */
int verifier_syntaxe(char *tokens[], int nt) {
    for (int i = 0; i < nt; i++) {
        char *tok = tokens[i];
        if (strcmp(tok, "+") != 0 && strcmp(tok, "-") != 0 &&
            strcmp(tok, "*") != 0 && strcmp(tok, "/") != 0) {
            char *endptr;
            strtod(tok, &endptr);
            if (*endptr != '\0') {
                printf("Erreur syntaxe : Token invalide '%s'\n", tok);
                return 0; 
            }
        }
    }
    return 1;
}

/* Exécute l'algorithme de la notation polonaise inverse (RPN) en utilisant une pile pour calculer le résultat final. */
int evaluer_rpn(char *tokens[], int nt, double *resultat_final) {
    double stack[STACK_SIZE];
    int top = 0;

    for (int i = 0; i < nt; i++) {
        char *tok = tokens[i];

        if (strcmp(tok, "+") == 0 || strcmp(tok, "-") == 0 ||
            strcmp(tok, "*") == 0 || strcmp(tok, "/") == 0) {
            if (top < 2) {
                printf("Erreur : L'opérateur '%s' nécessite deux opérandes.\n", tok);
                return 0;
            }

            double b = stack[--top];
            double a = stack[--top];
            double r = 0;

            if (strcmp(tok, "+") == 0){
                 r = a + b; 
            }else if (strcmp(tok, "-") == 0){
                r = a - b;
            } else if (strcmp(tok, "*") == 0){
                r = a * b;
            }else if (strcmp(tok, "/") == 0){
                if (b == 0) {
                    printf("Erreur : Division par zéro.\n");
                    return 0;
                }
                r = a / b;
            }
            stack[top++] = r;
        } else {
            if (top >= STACK_SIZE) {
                printf("Erreur : Pile pleine (Stack Overflow).\n");
                return 0;
            }
            stack[top++] = atof(tok);
        }
    }
    if (top != 1) {
        printf("Erreur : Expression RPN invalide (il reste %d valeurs dans la pile).\n", top);
        return 0;
    }

    *resultat_final = stack[0];
    return 1;
}

/* Découpe la chaîne de caractères initiale en un tableau de mots (tokens) en utilisant les espaces comme séparateurs. */
int tokenizer(char *line, char *tokens[], int max_tokens) {
    int nt = 0;
    char *t = strtok(line, " \t\n");
    while (t != NULL && nt < max_tokens) {
        tokens[nt++] = t;
        t = strtok(NULL, " \t\n");
    }
    return nt;
}

/* Affiche le résultat du calcul sur la sortie standard si le statut est valide, ou un message d'échec sinon. */
void afficher_resultat(int status, double resultat) {
    if (status) {
        printf("Résultat : %g\n", resultat);
    } else {
        printf("Le calcul a échoué.\n");
    }
}

int main(int argc, char *argv[]) {
    char line[1024];
    char *tokens[MAXTOK];
    int nt;
    double resultat;
    
    char *filename = (argc > 1) ? argv[1] : "calcul.txt";

    if (!lecture_expression(filename, line, sizeof(line))) {
        return 1;
    }

    printf("Expression lue : %s\n", line);

    nt = tokenizer(line, tokens, MAXTOK);

    if (!verifier_syntaxe(tokens, nt)) {
        return 1;
    }
    int status = evaluer_rpn(tokens, nt, &resultat);
    afficher_resultat(status, resultat);

    return 0;
}