#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    begin,
    end,
    read,
    write,
    id,
    intliteral,
    lparen,
    rparen,
    semicolon,
    comma,
    plusop,
    minusop,
    assignop,
    scan_eof
} token;

char token_buffer[256];

FILE *source;
FILE* output;

int line_number = 1;

/* ===== Prototypes ===== */
void clear_buffer(void);
void buffer_char(int c);
token check_reserved(void);
void lexical_error(int c);
token scanner(void);
const char* token_name(token t);

/* ===== Analyseur lexical ===== */
void clear_buffer(void) { token_buffer[0] = '\0'; }

void buffer_char(int c) {
    int len = strlen(token_buffer);
    token_buffer[len] = (char)c;
    token_buffer[len + 1] = '\0';
}

token check_reserved(void) {
    if (strcmp(token_buffer, "begin") == 0) return begin;
    if (strcmp(token_buffer, "end")   == 0) return end;
    if (strcmp(token_buffer, "read")  == 0) return read;
    if (strcmp(token_buffer, "write") == 0) return write;
    return id;
}

void lexical_error(int c) {
    /* On signale l'erreur mais on continue l'analyse */
    printf("Erreur lexicale (ligne %d) : caractere inattendu '%c'\n",
           line_number, c);
    fprintf(output, " *** ERREUR LEXICALE '%c' ***", c);
}

token scanner(void) {
    int in_char, c;
    clear_buffer();
    while ((in_char = fgetc(source)) != EOF) {
        if (in_char == '\n') { line_number++; continue; }
        if (isspace(in_char)) continue;

        else if (isalpha(in_char)) {
            buffer_char(in_char);
            for (c = fgetc(source); isalnum(c) || c == '_'; c = fgetc(source))
                buffer_char(c);
            ungetc(c, source);
            return check_reserved();
        }
        else if (isdigit(in_char)) {
            buffer_char(in_char);
            for (c = fgetc(source); isdigit(c); c = fgetc(source))
                buffer_char(c);
            ungetc(c, source);
            return intliteral;
        }
        else if (in_char == '(') return lparen;
        else if (in_char == ')') return rparen;
        else if (in_char == ';') return semicolon;
        else if (in_char == ',') return comma;
        else if (in_char == '+') return plusop;
        else if (in_char == '-') {
            c = fgetc(source);
            if (c == '-') {
                do { in_char = fgetc(source); } while (in_char != '\n' && in_char != EOF);
                if (in_char == '\n') line_number++;
            } else {
                ungetc(c, source);
                return minusop;
            }
        }
        else if (in_char == ':') {
            c = fgetc(source);
            if (c == '=') return assignop;
            else { ungetc(c, source); lexical_error(in_char); }
        }
        else {
            lexical_error(in_char);
        }
    }
    return scan_eof;
}

/* ===== Nom affiché pour chaque token ===== */
const char* token_name(token t) {
    switch (t) {
        case begin:      return "BEGIN";
        case end:        return "END";
        case read:       return "READ";
        case write:      return "WRITE";
        case id:         return "ID";
        case intliteral: return "INTLITERAL";
        case lparen:     return "LPAREN";
        case rparen:     return "RPAREN";
        case semicolon:  return "SEMICOLON";
        case comma:      return "COMMA";
        case plusop:     return "PLUSOP";
        case minusop:    return "MINUSOP";
        case assignop:   return "ASSIGNOP";
        case scan_eof:   return "SCANEOF";
        default:         return "ERREUR";
    }
}

/* ===== Main ===== */
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage : %s fichier_source fichier_output\n", argv[0]);
        return 1;
    }

    source = fopen(argv[1], "r");
    output = fopen(argv[2], "w");
    if (source == NULL) { printf("Impossible d'ouvrir %s\n", argv[1]); return 1; }
    if (output == NULL) { printf("Impossible d'ouvrir %s\n", argv[2]); return 1; }

    /* Pas de buffer : écriture immédiate dans output */
   // setvbuf(output, NULL, _IONBF, 0);

    token t;
    int first_on_line = 1;
    int last_line = 1;

    do {
        t = scanner();

        if (t == scan_eof) {
            /* Écrire SCANEOF à la fin */
            if (!first_on_line) fprintf(output, "\n");
            fprintf(output, "SCANEOF\n");
            break;
        }

        /* Détection de changement de ligne : si on a sauté une ligne pendant le scan,
           on doit fermer la ligne précédente avant d'écrire le nouveau token */
        if (line_number != last_line) {
            if (!first_on_line) {
                fprintf(output, "\n");
            }
            last_line = line_number;
            first_on_line = 1;
        }

        if (!first_on_line) fprintf(output, " ");
        fprintf(output, "%s", token_name(t));
        first_on_line = 0;

        /* Regarder si le prochain caractère est un '\n' pour fermer la ligne */
        int c = fgetc(source);
        if (c == '\n') {
            fprintf(output, "\n");
            first_on_line = 1;
            line_number++;
            last_line = line_number;
        } else {
            ungetc(c, source);
        }

    } while (t != scan_eof);

    fclose(source);
    fclose(output);
    return 0;
}