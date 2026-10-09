/* ============================================================
   Analyseur 1 PASSE - Version A
   - Une seule lecture de input.txt
   - Le parser lit les tokens DEPUIS LA MEMOIRE (pas de fichier)
   - A la fin, on ecrit TOUT dans outputFinal.txt :
       1) la liste des tokens (resultat du lexical)
       2) le verdict du syntaxique (resultat final)
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX 50
#define MAX_RESULT 10000   /* taille max du buffer de tokens */
#define MAX_MSG    5000    /* taille max du buffer de messages */

/* ---------- Les tokens (unites lexicales) ---------- */
typedef enum {
    BEGIN, END, READ, WRITE, ID, INTLITERAL, LPAREN, RPAREN,
    SEMICOLON, COMMA, ASSIGNOP, PLUSOP, MINUSOP, SCANEOF, FLOATLITERAL
} token;

/* Noms des tokens : MAJUSCULES pour l'affichage */
char tokens[15][15] = {
    "BEGIN", "END", "READ", "WRITE", "ID", "INTLITERAL",
    "LPAREN", "RPAREN", "SEMICOLON", "COMMA", "ASSIGNOP",
    "PLUSOP", "MINUSOP", "SCANEOF", "FLOATLITERAL"
};

/* ---------- Variables globales ---------- */
int ligneid = 1;            /* numero de ligne dans input.txt */
FILE *myFile = NULL;        /* input.txt */
token courant;              /* token courant du parser */
int nouvelle_ligne = 0;     /* 1 si on vient de passer une ligne */
int premier_token_ligne = 1;

/* ★ Buffer en MEMOIRE pour les tokens (resultat du lexical) ★ */
char resultat[MAX_RESULT];
int  resultat_len = 0;

/* ★ Buffer en MEMOIRE pour les messages du syntaxique ★ */
char messages[MAX_MSG];
int  messages_len = 0;

/* Nombre d'erreurs syntaxiques */
int nb_erreurs = 0;

/* Buffer pour construire un mot */
char buffer[MAX];
int buffer_indice = 0;

/* ---------- Fonctions utilitaires pour les buffers memoire ---------- */
void ajouter_resultat(const char *s) {
    int n = strlen(s);
    if (resultat_len + n < MAX_RESULT - 1) {
        strcpy(resultat + resultat_len, s);
        resultat_len += n;
    }
}

void ajouter_message(const char *s) {
    int n = strlen(s);
    if (messages_len + n < MAX_MSG - 1) {
        strcpy(messages + messages_len, s);
        messages_len += n;
    }
}

/* ---------- Fonctions du buffer de mot ---------- */
void clear_buffer() {
    buffer_indice = 0;
    buffer[0] = '\0';
}

void buffer_char(int car) {
    if (buffer_indice < MAX - 1) {
        buffer[buffer_indice] = (char)car;
        buffer_indice++;
        buffer[buffer_indice] = '\0';
    }
}

void to_maj() {
    int i;
    for (i = 0; i < buffer_indice; i++) {
        if (buffer[i] >= 'a' && buffer[i] <= 'z') {
            buffer[i] = buffer[i] - 'a' + 'A';
        }
    }
}

token check_reserved() {
    to_maj();
    if (strcmp(buffer, "BEGIN") == 0) return BEGIN;
    if (strcmp(buffer, "END")   == 0) return END;
    if (strcmp(buffer, "READ")  == 0) return READ;
    if (strcmp(buffer, "WRITE") == 0) return WRITE;
    return ID;
}

/* Erreur lexicale : on l'ajoute aux messages */
void lexical_error(int sym) {
    char tmp[100];
    sprintf(tmp, "Erreur lexicale ligne %d : caractere inconnu '%c'\n", ligneid, sym);
    ajouter_message(tmp);
    nb_erreurs++;
}

void read_number(FILE *f, int *c) {
    while (isdigit(*c)) {
        buffer_char(*c);
        *c = fgetc(f);
    }
    ungetc(*c, f);
}

void exposant(FILE *f, int *c) {
    buffer_char(*c);
    *c = fgetc(f);
    if (*c == '+' || *c == '-') {
        buffer_char(*c);
        *c = fgetc(f);
    }
    if (isdigit(*c)) {
        read_number(f, c);
    } else {
        ungetc(*c, f);
    }
}

/* ---------- Le SCANNER (lit input.txt) ---------- */
token scanner(FILE *f) {
    int c;
    clear_buffer();
    if (feof(f)) return SCANEOF;

    while ((c = fgetc(f)) != EOF) {

        if (c == '\n') {
            ligneid++;
            nouvelle_ligne = 1;
            return (token)(-1);
        }

        if (isspace(c)) continue;

        if (isalpha(c)) {
            buffer_char(c);
            c = fgetc(f);
            while (isalnum(c) || c == '_') {
                buffer_char(c);
                c = fgetc(f);
            }
            ungetc(c, f);
            return check_reserved();
        }

        if (isdigit(c)) {
            buffer_char(c);
            c = fgetc(f);
            read_number(f, &c);

            if (c == '.') {
                c = fgetc(f);
                if (isdigit(c)) {
                    buffer_char('.');
                    read_number(f, &c);
                } else {
                    ungetc(c, f);
                    return INTLITERAL;
                }
            }
            if (c == 'e' || c == 'E') {
                c = fgetc(f);
                exposant(f, &c);
                return FLOATLITERAL;
            }
            if (strchr(buffer, '.') != NULL) return FLOATLITERAL;
            return INTLITERAL;
        }

        if (c == '.') {
            c = fgetc(f);
            if (isdigit(c)) {
                buffer_char('.');
                read_number(f, &c);
                if (c == 'e' || c == 'E') {
                    c = fgetc(f);
                    exposant(f, &c);
                }
                return FLOATLITERAL;
            } else {
                ungetc(c, f);
                lexical_error('.');
                continue;
            }
        }

        if (c == '(') return LPAREN;
        if (c == ')') return RPAREN;
        if (c == ';') return SEMICOLON;
        if (c == ',') return COMMA;
        if (c == '+') return PLUSOP;

        if (c == ':') {
            int c2 = fgetc(f);
            if (c2 == '=') return ASSIGNOP;
            ungetc(c2, f);
            lexical_error(':');
            continue;
        }

        if (c == '-') {
            int c2 = fgetc(f);
            if (c2 == '-') {
                int cc;
                do { cc = fgetc(f); } while (cc != '\n' && cc != EOF);
                ligneid++;
                nouvelle_ligne = 1;
                continue;
            } else {
                ungetc(c2, f);
                return MINUSOP;
            }
        }

        lexical_error(c);
    }
    return SCANEOF;
}

/* ---------- Le PARSER ---------- */
void syntax_error(token t) {
    char tmp[150];
    if ((int)t == -1)
        sprintf(tmp, "Erreur de syntaxe ligne %d : expression manquante\n", ligneid);
    else
        sprintf(tmp, "Erreur de syntaxe ligne %d : attendu %s\n", ligneid, tokens[t]);
    ajouter_message(tmp);
    nb_erreurs++;
}

/* ★ Le parser recoit le token EN MEMOIRE (valeur de retour) ★ */
token next_token() {
    token t = scanner(myFile);

    /* Sauter les retours de ligne : on ajoute \n dans le buffer resultat */
    while ((int)t == -1) {// -1 signifie retour a la ligne
        if (nouvelle_ligne) {
            ajouter_resultat("\n");
            nouvelle_ligne = 0;
            premier_token_ligne = 1;
        }
        t = scanner(myFile);
    }
    if (nouvelle_ligne) {
        ajouter_resultat("\n");
        nouvelle_ligne = 0;
        premier_token_ligne = 1;
    }

    /* Ajout du token dans le buffer resultat (en memoire) */
    if (t != SCANEOF) {
        if (premier_token_ligne) {
            ajouter_resultat(tokens[t]);
            premier_token_ligne = 0;
        } else {
            ajouter_resultat(" ");
            ajouter_resultat(tokens[t]);
        }
    } else {
        ajouter_resultat(" ");
        ajouter_resultat(tokens[t]);
    }

    return t;   /* ← retour EN MEMOIRE au parser */
}

void match(token t) {
    if (courant == t) {
        courant = next_token();   /* ← courant recoit le token en RAM */
    } else {
        char tmp[100];
        sprintf(tmp, "Trouve %s au lieu de %s\n", tokens[courant], tokens[t]);
        ajouter_message(tmp);
        syntax_error(t);
    }
}

/* Prototypes */
void program();
void inst_list();
void inst();
void prim();
void id_list();
void expression();
void expr_list();
void add_op();

void system_goal() {
    myFile = fopen("input.txt", "r");
    if (myFile == NULL) {
        printf("Impossible d'ouvrir input.txt\n");
        exit(1);
    }

    /* ★ Le parser lit DEPUIS LA MEMOIRE, pas depuis un fichier ★ */
    program();
    match(SCANEOF);

    fclose(myFile);

    /* ★ Ecriture du resultat FINAL dans outputFinal.txt ★ */
    FILE *final = fopen("outputFinal.txt", "w");
    if (final == NULL) {
        printf("Impossible d'ouvrir outputFinal.txt\n");
        exit(1);
    }

    fprintf(final, "===== RESULTAT DE L'ANALYSE LEXICALE =====\n");
    fprintf(final, "%s\n", resultat);

    fprintf(final, "\n===== RESULTAT DE L'ANALYSE SYNTAXIQUE =====\n");
    if (nb_erreurs == 0) {
        fprintf(final, "Aucune erreur detectee. Programme correct.\n");
    } else {
        fprintf(final, "Nombre d'erreurs : %d\n", nb_erreurs);
        fprintf(final, "%s", messages);
    }

    fclose(final);
}

void program() {
    courant = next_token();
    match(BEGIN);
    inst_list();
    match(END);
}

void inst_list() {
    inst();
    while (1) {
        if (courant == ID || courant == READ || courant == WRITE) {
            inst();
        } else if (courant == END || courant == SCANEOF) {
            return;
        } else {
            syntax_error(-1);
            courant = next_token();
        }
    }
}

void inst() {
    if (courant == ID) {
        match(ID);
        match(ASSIGNOP);
        expression();
        match(SEMICOLON);
    }
    else if (courant == READ) {
        match(READ);
        match(LPAREN);
        id_list();
        match(RPAREN);
        match(SEMICOLON);
    }
    else if (courant == WRITE) {
        match(WRITE);
        match(LPAREN);
        expr_list();
        match(RPAREN);
        match(SEMICOLON);
    }
    else {
        char tmp[100];
        sprintf(tmp, "Instruction inattendue : %s\n", tokens[courant]);
        ajouter_message(tmp);
        syntax_error(-1);
    }
}

void prim() {
    if (courant == LPAREN) {
        match(LPAREN);
        expression();
        match(RPAREN);
    }
    else if (courant == ID) {
        match(ID);
    }
    else if (courant == INTLITERAL) {
        match(INTLITERAL);
    }
    else if (courant == FLOATLITERAL) {
        match(FLOATLITERAL);
    }
    else if (courant == END || courant == SCANEOF) {
        return;
    }
    else {
        while (courant != RPAREN && courant != COMMA &&
               courant != SEMICOLON && courant != END && courant != SCANEOF) {
            char tmp[100];
            sprintf(tmp, "Trouve %s (inattendu)\n", tokens[courant]);
            ajouter_message(tmp);
            syntax_error(-1);
            courant = next_token();
        }
    }
}

void id_list() {
    if (courant == ID) match(ID);
    else syntax_error(ID);
    while (courant == COMMA) {
        match(COMMA);
        if (courant == ID) match(ID);
        else syntax_error(ID);
    }
}

void expression() {
    prim();
    while (courant == PLUSOP || courant == MINUSOP) {
        add_op();
        prim();
    }
}

void expr_list() {
    expression();
    while (courant == COMMA) {
        match(COMMA);
        expression();
    }
}

void add_op() {
    if (courant == PLUSOP || courant == MINUSOP) match(courant);
    else syntax_error(courant);
}

/* ---------- MAIN ---------- */
int main() {
    printf("=== Analyseur 1 PASSE - Version A ===\n");
    resultat[0] = '\0';
    messages[0] = '\0';
    system_goal();
    printf("\nAnalyse terminee. Voir outputFinal.txt\n");
    return 0;
}