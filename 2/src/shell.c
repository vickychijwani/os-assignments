#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <error.h>
#include <libgen.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define COMMAND_MAX 10000
#define ARGS_MAX 20
#define TIME_MAX 17
#define MODE_MAX 11
#define DEFAULT_DIR_PERM 0755
#define BUFSIZE 4096
#define ALLOC(type,n) (type *) malloc((n)*sizeof(type))

/* errno holds error codes from library function calls */
extern int errno;
extern char *program_invocation_name;

/* function prototypes */
void my_cp(char *[], int);
int my_copy_check_for_src_errors(const char *);
int my_copy_to_dir_safe(char *, const char *);
int my_copy_to_dir(char *, const char *);
int my_copy(const char *, const char *);
void my_ls(char *[], int);
void my_ls_long(char *);
void my_ls_compact(char *);
void my_cd(char *[], int);
void my_rmdir(char *[], int);
void my_mkdir(char *[], int);
void my_pwd(char *[], int);

int execute(char *[], int);
void get_mode_str(char *, mode_t);
void rwx(unsigned short, char *);
off_t get_stats(struct stat ***, struct dirent **, char *path, int);
int tokenize_command(char *[], char *);
char *parse_escapes_and_quotes(char *);
int dirent_filter(const struct dirent *);
void read_command(char *);
void cleanup_args(char *[], int);

void trim(char *);
void combine(char*, const char*, const char*);

int main(void) {
    int i, num_args, status;
    char *command = ALLOC(char, COMMAND_MAX+1);

    char *args[ARGS_MAX];

    while (1) {
        i = 0;

        /* accept command input */
        read_command(command);

        num_args = tokenize_command(args, (char *) strdup(command));

        /* execute the appropriate function or command */
        program_invocation_name = args[0];
        if (strcmp(args[0], "exit") == 0)
            exit(0);
        else if (strcmp(args[0], "pwd") == 0)
            my_pwd(args, num_args);
        else if (strcmp(args[0], "mkdir") == 0)
            my_mkdir(args, num_args);
        else if (strcmp(args[0], "rmdir") == 0)
            my_rmdir(args, num_args);
        else if (strcmp(args[0], "cd") == 0)
            my_cd(args, num_args);
        else if (strcmp(args[0], "ls") == 0)
            my_ls(args, num_args);
        else if (strcmp(args[0], "cp") == 0)
            my_cp(args, num_args);
        else
            status = execute(args, num_args);
    }

    /* free heap-allocated memory */
    cleanup_args(args, num_args);
    free(command);

    return 0;
}

/* functions for the various builtin commands */
/* cp */
void my_cp(char *args[], int num_args) {
    int status_dest, error_flag = 0;
    char *src, *dest = args[num_args-1];
    struct stat *st_buf_dest = ALLOC(struct stat, 1);

    status_dest = stat(dest, st_buf_dest);

    if (num_args < 3)
        error(0, 0, "missing destination file operand after `%s'", args[1]);

    else if (num_args == 3) {
        src = args[1];
        error_flag = my_copy_check_for_src_errors(src);

        if (!error_flag) {
            if (my_copy_to_dir_safe(src, dest))
                ;
            else if (dest[strlen(dest)-1] != '/' &&
                     (status_dest != 0 || S_ISREG(st_buf_dest->st_mode)))
                my_copy(src, dest);
        }
    }

    else {
        int i;
        for (i = 1; i <= num_args-2; i++) {
            src = args[i];
            error_flag = my_copy_check_for_src_errors(src);

            if (!error_flag) {
                my_copy_to_dir_safe(src, dest);
            }
        }
    }
}

/* check for errors related to the source operand of cp */
int my_copy_check_for_src_errors(const char *src) {
    int status_src;
    struct stat *st_buf_src = ALLOC(struct stat, 1);
    status_src = stat(src, st_buf_src);

    if (status_src != 0) {
        error(0, errno, "cannot stat `%s'", src);
        return 1;
    }

    else if (S_ISDIR(st_buf_src->st_mode)) {
        error(0, 0, "omitting directory `%s'", src);
        return 1;
    }
    return 0;
}

/* copy src file to dest directory (with the same name, of course),
 * also check for errors
 */
int my_copy_to_dir_safe(char *src, const char *dest_dir) {
    int error_flag = 0, status_dest, ret;
    struct stat *st_buf_dest = ALLOC(struct stat, 1);

    status_dest = stat(dest_dir, st_buf_dest);
    error_flag = my_copy_check_for_src_errors(src);

    if (status_dest == 0 && S_ISDIR(st_buf_dest->st_mode)) {
        ret = my_copy_to_dir(src, dest_dir);
    }
    else if (dest_dir[strlen(dest_dir)-1] == '/') {
        error(0, 0, "target `%s' is not a directory", dest_dir);
        ret = 0;
    }
    else
        ret = 0;

    free(st_buf_dest);
    return ret;
}

/* copy src file to dest directory (with the same name, of course) */
int my_copy_to_dir(char *src, const char *dest_dir) {
    int status_dest, ret = 1;
    struct stat *st_buf_dest = ALLOC(struct stat, 1);
    char *dest = ALLOC(char, PATH_MAX+1);
    combine(dest, dest_dir, basename(src));
    status_dest = stat(dest, st_buf_dest);
    if (status_dest == 0 && S_ISDIR(st_buf_dest->st_mode)) {
        error(0, 0, "cannot overwrite directory `%s' with non-directory", dest);
        ret = 0;
    }
    else
        ret = my_copy(src, dest);
    free(dest);
    return ret;
}

/* copy src file to dest file */
/* return value 1 indicates success, 0 indicates failure */
int my_copy(const char* src, const char *dest) {
    int items_read, ret = 1;
    FILE *ifp, *ofp;
    void *buf = malloc(BUFSIZE*sizeof(char));
    ifp = fopen(src, "r");
    ofp = fopen(dest, "w");

    if (access(src, R_OK) && (!access(dest, F_OK) || access(dest, W_OK)))
        ret = 0;
    else if (ifp == NULL) {
        error(0, errno, "cannot open `%s' for reading", src);
        ret = 0;
    }
    else if (ofp == NULL) {
        error(0, errno, "cannot create regular file `%s'", dest);
        ret = 0;
    }
    else {
        while (!feof(ifp)) {
            items_read = fread(buf, sizeof(char), BUFSIZE, ifp);
            if (ferror(ifp)) {
                error(0, 0, "input file `%s' could not be read", src);
                ret = 0;
                break;
            }
            fwrite(buf, sizeof(char), items_read, ofp);
            if (ferror(ofp)) {
                error(0, 0, "output file `%s' could not be read", dest);
                ret = 0;
                break;
            }
        }
        fclose(ifp);
        fclose(ofp);
    }

    return ret;
}

/* ls */
void my_ls(char *args[], int num_args) {
    if (num_args >= 2 && strcmp(args[1], "-l") == 0)
        my_ls_long((num_args == 2) ? "." : args[2]);
    else
        my_ls_compact((num_args == 1) ? "." : args[1]);
}

/* ls WITH -l */
void my_ls_long(char *path) {
    int i, n;
    /* sum of the 1024-byte block sizes (to the next higher multiple of 4) of
     * all directory entries, displayed as "total <total_blk_size>" in ls -l
     */
    off_t total_blk_size;
    /* array of struct dirent */
    struct dirent **d_ents;
    /* array of struct stat */
    struct stat **stats;
    /* nane     = file / directory owner's name
     * group    = file / directory group's name
     * time     = time of last modification of the file (formatted string)
     * mode_str = string representing file type and mode, e.g., -rwxr-xr-x
     */
    char *name, *group, time[TIME_MAX], mode_str[MODE_MAX];

    /* n = number of entries scanned and filtered through by scandir() */
    n = scandir(path, &d_ents, dirent_filter, alphasort);

    if (n == -1) {
        perror("ls");
    }

    total_blk_size = get_stats(&stats, d_ents, path, n);
    printf("total %d\n", (int) total_blk_size);
    for (i = 0; i < n; i++) {
        name = getpwuid(stats[i]->st_uid)->pw_name;
        group = getgrgid(stats[i]->st_gid)->gr_name;
        strftime(time, TIME_MAX, "%Y-%m-%d %H:%M", localtime(&stats[i]->st_mtime));
        get_mode_str(mode_str, stats[i]->st_mode);
        printf("%s %3u %s %s %9u %s %s\n", mode_str,
               stats[i]->st_nlink, name, group,
               (unsigned) stats[i]->st_size,
               time, d_ents[i]->d_name);
        free(stats[i]);
    }
    free(stats);
}

/* ls WITHOUT -l */
void my_ls_compact(char *path) {
    int i, n;
    struct dirent **d_ents;

    /* n = number of entries scanned and filtered through by scandir() */
    n = scandir(path, &d_ents, dirent_filter, alphasort);

    if (n == -1)
        perror("ls");

    if (n >= 0) {
        for (i = 0; i < n; i++) {
            printf("%s\n", d_ents[i]->d_name);
            free(d_ents[i]);
        }
        free(d_ents);
    }
}

/* cd */
void my_cd(char *args[], int num_args) {
    /* cd to args[1] if it exists, else cd to $HOME if num_args == 1 */
    char *target = (num_args >= 2) ? args[1] : getenv("HOME");
    if (chdir(target) != 0)
        perror(args[0]);
}

/* rmdir */
void my_rmdir(char *args[], int num_args) {
    int i;
    if (num_args < 2)
        error(0, 0, "missing operand");
    else {
        for (i = 1; i < num_args; i++)
            if (rmdir(args[i]) != 0)
                perror(args[0]);
    }
}

/* mkdir */
void my_mkdir(char *args[], int num_args) {
    int i;
    if (num_args < 2)
        error(0, 0, "missing operand");
    else {
        for (i = 1; i < num_args; i++)
            if (mkdir(args[i], DEFAULT_DIR_PERM) != 0)
                perror(args[0]);
    }
}

/* pwd */
void my_pwd(char *args[], int num_args) {
    /* PATH_MAX is defined in /usr/include/limits.h */
    char *str = ALLOC(char, PATH_MAX+1);
    (void) num_args; /* suppress compiler "unused parameter" warnings */
    if (getcwd(str, PATH_MAX))
        printf("%s\n", str);
    else
        perror(args[0]);
    free(str);
}

/* if args[0] doesn't match any of the builtin commands, treat it as the name of
 * an executable, search for it in the current directory and $PATH, and run it.
 */
int execute(char *args[], int n) {
    int status, exec_ret, fork_ret;
    char *old_path = getenv("PATH");
    char *new_path = ALLOC(char, strlen(old_path)+3);

    /* add the current directory to $PATH */
    strcpy(new_path, ".:");
    strcat(new_path, old_path);
    /* the last argument being non-zero signifies that over-writing is allowed */
    setenv("PATH", new_path, 1);

    /* execvp() requires the command line arguments array to be terminated by a
     * NULL (see execvp(3))
     */
    args[n] = NULL;

    fork_ret = fork();
    if (!fork_ret) {
        exec_ret = execvp(args[0], args);
        if (exec_ret == -1) {
            if (errno == ENOENT)
                printf("%s: command not found\n", args[0]);
            else
                perror("execvp");
        }
    }
    if (fork_ret == -1)
        perror("fork");
    wait(&status);

    /* restore the old $PATH */
    setenv("PATH", old_path, 1);

    return status;
}



/* functions for converting the mode integer in struct stat to a human readable
 * string of access modes and the like (call it the "rwx form").
 */
void get_mode_str(char *s, mode_t mode) {
    char type;

    if (S_ISREG(mode))       type = '-';
    else if (S_ISDIR(mode))  type = 'd';
    else if (S_ISCHR(mode))  type = 'c';
    else if (S_ISBLK(mode))  type = 'b';
    else if (S_ISFIFO(mode)) type = 'p';
    else if (S_ISLNK(mode))  type = 'l';
    else if (S_ISSOCK(mode)) type = 's';
    else                     type = '?';
    s[0] = type;

    rwx((mode & 0700) << 0, &s[1]);
    rwx((mode & 0070) << 3, &s[4]);
    rwx((mode & 0007) << 6, &s[7]);

    s[10] = '\0';
}

/* convert a 3-digit octal number to its "rwx form" */
void rwx(unsigned short mode, char *s) {
    s[0] = (mode & S_IRUSR) ? 'r' : '-';
    s[1] = (mode & S_IWUSR) ? 'w' : '-';
    s[2] = (mode & S_IXUSR) ? 'x' : '-';
}



/* functions for gathering and structuring information for ls -l
 */

/* fetch struct stat structures for certain given entries (in d_ents, which is
 * an array of struct dirent pointers)
 */
off_t get_stats(struct stat ***stats, struct dirent **d_ents, char *basepath, int n) {
    int i, ret;
    off_t total_blk_size = 0;
    char *path = ALLOC(char, PATH_MAX+1);
    struct stat **s = ALLOC(struct stat *, n);
    for (i = 0; i < n; i++) {
        s[i] = ALLOC(struct stat, 1);
        strcpy(path, basepath);
        strcat(path, "/");
        strcat(path, d_ents[i]->d_name);
        ret = stat(path, s[i]);
        if (ret != 0)
            perror("stat");
        total_blk_size += (s[i]->st_size % 4096 == 0) ?
            (s[i]->st_size / 4096) * 4 :
            (s[i]->st_size / 4096 + 1) * 4;
    }
    *stats = s;
    free(path);
    return total_blk_size;
}

/* filter to NOT display dotfiles in 'ls' and 'ls -l' */
int dirent_filter(const struct dirent *d) {
    return ((d->d_name)[0] != '.');
}



/* functions to help tokenize the input command, with escapes and quoted strings */
int tokenize_command(char *args[], char *command) {
    static int i = 0;
    unsigned int j, k, len = 0, in_single_quotes = 0, in_double_quotes = 0, in_quotes = 0;
    trim(command);

    /* base case: command = "" */
    if (*command == '\0') {
        j = i;
        i = 0;
        return j;
    }

    /* recursive case: tokenize_command(args, "a b") <=> args[i] = "a", tokenize_command(args, "b") */
    /* also handles arguments with spaces (e.g. foo\ bar, "foo bar") */
    for (j = 0; j <= strlen(command); j++, len++) {
        if (command[j] == '\'') in_single_quotes = !in_single_quotes;
        else if (command[j] == '\"') in_double_quotes = !in_double_quotes;
        in_quotes = in_single_quotes || in_double_quotes;
        if ((!in_quotes && command[j] == ' ' && command[j-1] != '\\') ||
            command[j] == '\0') {
            args[i] = ALLOC(char, len+1);
            for (k = 0; k < len; k++) {
                args[i][k] = command[k];
            }
            args[i][len] = '\0';
            args[i] = parse_escapes_and_quotes(args[i]);
            i++;
            return tokenize_command(args, &command[len+1]);
        }
    }
    return 0;
}

/* parse escaped spaces and remove literal quotes from strings
 * e.g. foo\ bar => foo bar
 *      "foo bar" => foo bar
 *      'foo bar' => foo bar
 */
char *parse_escapes_and_quotes(char *s) {
    int i, j = 0, len = strlen(s);
    char *t = ALLOC(char, len+1), *temp = s;

    if (*s == '\'' || *s == '\"') {
        strncpy(t, s, len);
        *(t+len-1) = '\0';
        t++;
        trim(t);
    }
    else {
        for (i = 0; i < len; i++) {
            if (*s != '\\') {
                *t++ = *s;
                j++;
            }
            s++;
        }
        *t = '\0';
        t -= j;
    }

    /* free memory allocated to the un-parsed s since it's no longer needed */
    free(temp);

    return t;
}



/* utility functions */
/* trims leading and trailing whitespace from a string */
void trim(char *str) {
    char *end;

    /* Trim leading space */
    while (isspace(*str)) str++;

    /* All spaces? */
    if (*str == '\0')
        return;

    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        end--;

    /* Write new null terminator */
    *(end+1) = '\0';
}

void combine(char* dest, const char* path1, const char* path2) {
    char separator[] = "/";
    if(path1 == NULL && path2 == NULL)
        strcpy(dest, "");
    else if(path2 == NULL || strlen(path2) == 0)
        strcpy(dest, path1);
    else if(path1 == NULL || strlen(path1) == 0)
        strcpy(dest, path2);
    else {
        const char last_char = path1[strlen(path1) - 1];
        strcpy(dest, path1);
        if(last_char != separator[0])
            strcat(dest, separator);
        strcat(dest, path2);
    }
}


/* read a command at the prompt */
void read_command(char *command) {
    char *prompt = "myshell> ";
    printf("%s", prompt);
    fgets(command, COMMAND_MAX, stdin);
}

/* de-allocate memory from arguments from the executed command */
void cleanup_args(char *args[], int n) {
    int i;
    for (i = 0; i < n; i++) {
        free(args[i]);
        args[i] = NULL;
    }
}
