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

/* ===== Prototypes analyseur lexical ===== */
void clear_buffer(void);
void buffer_char(int c);
token check_reserved(void);
void lexical_error(int c);
token scanner(void);
void print_token(token t);
void print_token_to_output(token t);

/* ===== Prototypes analyseur syntaxique ===== */
void system_goal(void);
void program(void);
void inst_list(void);
void inst(void);
void id_list(void);
void expression(void);
void expr_list(void);
void add_op(void);
void prim(void);
void match(token expected);
void syntax_error(token tok);

/* ===== Variables globales pour l'analyseur syntaxique ===== */
token next_token;
token current_token;
int error_count = 0;  /* Pour éviter les boucles infinies */

/* ===== Analyseur lexical (inchangé) ===== */
void clear_buffer(void) {
    token_buffer[0] = '\0';
}

void buffer_char(int c) {
    int len = strlen(token_buffer);
    token_buffer[len] = (char)c;
    token_buffer[len + 1] = '\0';
}

token check_reserved(void) {
    if(strcmp(token_buffer, "begin") == 0) return begin;
    if(strcmp(token_buffer,"end") == 0) return end;
    if(strcmp(token_buffer,"read") == 0) return read;
    if(strcmp(token_buffer,"write") == 0) return write;
    return id;
}

void lexical_error(int c) {
    printf("Erreur lexicale (ligne %d) : caractere inattendu '%c'\n",
           line_number, c);
}

token scanner(void) {
    int in_char, c;

    clear_buffer();

    while ((in_char = fgetc(source)) != EOF) {

        if (in_char == '\n') {
            line_number++;
            continue;
        }

        if (isspace(in_char))
            continue;

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
                do {
                    in_char = fgetc(source);
                } while (in_char != '\n' && in_char != EOF);
                if (in_char == '\n') line_number++;
            } else {
                ungetc(c, source);
                return minusop;
            }
        }

        else if (in_char == ':') {
            c = fgetc(source);
            if (c == '=') return assignop;
            else {
                ungetc(c, source);
                lexical_error(in_char);
            }
        }

        else {
            lexical_error(in_char);
        }
    }

    return scan_eof;
}

void print_token_to_output(token t) {
    fprintf(output, "[ligne %d] ", line_number);
    switch (t) {
        case id:         fprintf(output, "ID(%s)\n", token_buffer); break;
        case intliteral: fprintf(output, "INT(%s)\n", token_buffer); break;
        case lparen:     fprintf(output, "LPAREN\n"); break;
        case rparen:     fprintf(output, "RPAREN\n"); break;
        case semicolon:  fprintf(output, "SEMICOLON\n"); break;
        case comma:      fprintf(output, "COMMA\n"); break;
        case plusop:     fprintf(output, "PLUSOP\n"); break;
        case minusop:    fprintf(output, "MINUSOP\n"); break;
        case assignop:   fprintf(output, "ASSIGNOP\n"); break;
        case scan_eof:   fprintf(output, "SCANEOF\n"); break;
        default:         fprintf(output, "ERREUR\n"); break;
    }
}

/* ===== Analyseur syntaxique ===== */

/*
 * Fonction syntax_error
 * Signale une erreur syntaxique avec le token fautif.
 */
void syntax_error(token tok) {
    printf("Erreur syntaxique (ligne %d) : token inattendu %d\n",
           line_number, tok);
    error_count++;
    if (error_count > 50) {
        printf("Trop d'erreurs, arret de l'analyse.\n");
        exit(1);
    }
}

/*
 * Fonction match
 * Vérifie que le token courant correspond au token attendu.
 * Si oui, avance au token suivant.
 * Sinon, signale une erreur mais continue (skip).
 */
void match(token expected) {
    if (current_token == expected) {
        current_token = next_token;
        next_token = scanner();
    } else {
        printf("Erreur syntaxique (ligne %d) : token attendu %d, token trouve %d\n",
               line_number, expected, current_token);
        error_count++;
        if (error_count > 50) {
            printf("Trop d'erreurs, arret de l'analyse.\n");
            exit(1);
        }
        /* Ne pas consommer le token fautif pour éviter de sauter des tokens valides */
        /* On avance quand même pour éviter les boucles infinies */
        current_token = next_token;
        next_token = scanner();
    }
}

/*
 * <system_goal> ::= <program> SCANEOF
 */
void system_goal(void) {
    program();
    match(scan_eof);
}

/*
 * <program> ::= begin <inst_list> end
 */
void program(void) {
    match(begin);
    inst_list();
    match(end);
}

/*
 * <inst_list> ::= <inst> {<inst>}
 */
void inst_list(void) {
    inst();
    while (1) {
        switch (next_token) {
            case id:
            case read:
            case write:
                inst();
                break;
            default:
                return;
        }
    }
}

/*
 * <inst> ::= ID := <expr> ;
 *          | read ( <id_list> ) ;
 *          | write ( <expr_list> ) ;
 */
void inst(void) {
    token tok = next_token;
    switch (tok) {
        case id:
            match(id);
            match(assignop);
            expression();
            match(semicolon);
            break;
        case read:
            match(read);
            match(lparen);
            id_list();
            match(rparen);
            match(semicolon);
            break;
        case write:
            match(write);
            match(lparen);
            expr_list();
            match(rparen);
            match(semicolon);
            break;
        default:
            syntax_error(tok);
            /* Avancer pour éviter boucle infinie */
            current_token = next_token;
            next_token = scanner();
            break;
    }
}

/*
 * <id_list> ::= ID { , ID }
 */
void id_list(void) {
    match(id);
    while (next_token == comma) {
        match(comma);
        match(id);
    }
}

/*
 * <expr> ::= <prim> { <addop> <prim> }
 */
void expression(void) {
    prim();
    while (next_token == plusop || next_token == minusop) {
        add_op();
        prim();
    }
}

/*
 * <expr_list> ::= <expr> { , <expr> }
 */
void expr_list(void) {
    expression();
    while (next_token == comma) {
        match(comma);
        expression();
    }
}

/*
 * <addop> ::= PLUSOP | MINUSOP
 */
void add_op(void) {
    token tok = next_token;
    if (tok == plusop || tok == minusop) {
        match(tok);
    } else {
        syntax_error(tok);
        current_token = next_token;
        next_token = scanner();
    }
}

/*
 * <prim> ::= ( <expr> )
 *          | ID
 *          | INTLITERAL
 */
void prim(void) {
    token tok = next_token;
    switch (tok) {
        case lparen:
            match(lparen);
            expression();
            match(rparen);
            break;
        case id:
            match(id);
            break;
        case intliteral:
            match(intliteral);
            break;
        default:
            syntax_error(tok);
            current_token = next_token;
            next_token = scanner();
            break;
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
    if (source == NULL) {
        printf("Impossible d'ouvrir le fichier %s\n", argv[1]);
        return 1;
    }
    if (output == NULL) {
        printf("Impossible d'ouvrir le fichier %s\n", argv[2]);
        return 1;
    }

    /* Initialisation de l'analyseur syntaxique */
    next_token = scanner();
    current_token = next_token;
    next_token = scanner();

    /* Lancement de l'analyse syntaxique */
    system_goal();

    fclose(source);
    fclose(output);
    return 0;
}