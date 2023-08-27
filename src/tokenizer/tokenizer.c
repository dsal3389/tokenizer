#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>


#define MAX_FILES_COUNT 256

struct __config{
    int raw;
    int suppress;
    int group_spaces;
};


char *progname;
struct __config config = { .raw=0, .suppress=0, .group_spaces=0 };


// c is char and literal is string
// print the given char if configured --raw or -r, else
// print the given literal
#define PRINT_RAW_OR_LITERAL(c, literal) do {\
        if(config.raw){\
            printf("%c\n", c);\
        } else {\
            printf(literal "\n");\
        }\
    }while(0)


void die(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}


void print_help()
{
    printf(
        "%s [...flags] [...files]\n"
        "flags:\n"
        "    -h,  --help             print this message to the screen\n"
        "    -r,  --raw              print the character instead of (SPACE), (TAB), (NEWLINE)\n"
        "    -sb, --supress-blank    supress blank lines and invisable chars, (SPACE), (TAB) and (NEWLINE) wont be printed\n"
        "    -gs, --group-spaces     group multiple space in a row, into a single word (SPACEx{spacenumber}), (SPACEx8) means 8 spaces\n"
        "examples:\n"
        "    %s ./myfile.py ./myfile.c\n"
        "    %s -sb ./code.rst\n"
    , progname, progname, progname);
    exit(EXIT_SUCCESS);
}


int file_path_valid(const char *path)
{
    struct stat fstat;

    if(stat(path, &fstat) == -1){
        die("failed get stat for path %s\n", path);
    }

    switch(fstat.st_mode & S_IFMT){
        case S_IFBLK:
        case S_IFREG:
            return 1;
        default:
            return 0;
    }
}


void tokenize(char *str)
{
    int escape = 0;
    char tmp = 0;
    char *c = str, *start = NULL;

    while(*c){
        switch(*c){
            case 0:
                return;
            case '\n':
                if(config.suppress){
                    break;
                }
                PRINT_RAW_OR_LITERAL(*c, "(NEWLINE)");
                break;
            case '\t':
                if(config.suppress){
                    break;
                }
                PRINT_RAW_OR_LITERAL(*c, "(TAB)");
                break;
            case ' ':
                if(config.suppress){
                    break;
                }
                
                if(config.group_spaces && *(c+1) == ' '){
                    start = c;
                    while(*c && *c == ' '){
                        c++;
                    }
                    printf("(SPACEx%ld)\n", c - start);
                    c--;
                } else {
                    PRINT_RAW_OR_LITERAL(*c, "(SPACE)");
                }
                break;
            case '0'...'9':
                start = c;

                while(*c && (isalnum(*c) || *c == '.')){
                    c++;
                }

                tmp = *c;
                *c = 0;

                printf("%s\n", start);
                *c = tmp;
                c--;
                break;
            case '\"':
            case '\'':
                escape = 0;
                start = c;
                c++;

                while(*c && (*c != *start || escape)){
                    escape = 0;
                    
                    if(*c == '\\'){
                        while(*c && *c == '\\'){
                            escape = !escape;
                            c++;
                        }
                    } else {
                        c++;
                    }
                }

                c++;
                tmp = *c;
                *c = 0;

                printf("%s\n", start);
                *c = tmp;
                c--;
                break;
            case 'a'...'z':
            case 'A'...'Z':
                start = c;

                while(*c && (isalpha(*c) || *c == '_')){
                    c++;
                }

                tmp = *c;
                *c = 0;

                printf("%s\n", start);
                *c = tmp;
                c--;
                break;
            default:
                printf("%c\n", *c);
                break;
        }
        c++;
    }
}


void tokenize_file(const char *path)
{
    char line_buffer[1024], *c = line_buffer;
    int fd = open(path, O_RDONLY);

    if(fd == -1){
        die("failed open file %s for read\n", path);
    }

    while (1){
        c = line_buffer;
        memset(line_buffer, 0, sizeof(line_buffer));

        while(read(fd, c, 1) > 0){
            if(*c == '\n' || (line_buffer - c) >= 1023){
                c++;
                break;
            }
            c++;
        }

        if(line_buffer == c){
            break;
        }

        tokenize(line_buffer);
    }
}


void parse_argv(int *argc, char ***argv, char **buffer, size_t bsize)
{
    size_t files_count = 0;

    while(*argc){
        if(***argv == '-'){
            if(!strcmp(**argv, "-h") || !strcmp(**argv, "--help")){
                print_help();
            } else if(!strcmp(**argv, "-r") || !strcmp(**argv, "--raw")){
                if(config.group_spaces || config.suppress){
                    die("flag %s is not compatible with --supress-blank or --group-spaces\n");
                }

                config.raw = 1;
                (*argc)--;
                (*argv)++;
            } else if(!strcmp(**argv, "-sb") || !strcmp(**argv, "--suppress-blank")){
                if(config.raw || config.group_spaces){
                    die("flag %s is not compatible with --raw and --group-spaces\n", **argv);
                }

                config.suppress = 1;
                (*argc)--;
                (*argv)++;
            } else if (!strcmp(**argv, "-gs") || !strcmp(**argv, "--group-spaces")){
                if(config.raw || config.suppress){
                    die("flag %s is not compatible with --raw and --supress-blank\n", **argv);
                }

                config.group_spaces = 1;
                (*argc)--;
                (*argv)++;
            } else {
                die("uknown flag passed %s\n", **argv);
            }
        } else {
            if(files_count == bsize){
                die("too many files were given, max support is %d", bsize);
            }

            if(!file_path_valid(**argv)){
                die("given file path (%s) is invalid, not exists or not a file\n", **argv);
            }

            buffer[files_count++] = **argv;
            (*argc)--;
            (*argv)++;
        }
    }
}


int main(int argc, char **argv)
{
    char *files[MAX_FILES_COUNT];
    char **file = files;

    progname = *argv;
    argc--;
    argv++;

    memset(files, 0, sizeof(files));
    parse_argv(&argc, &argv, files, sizeof(files));

    while(*file){
        tokenize_file(*file);
        file++;
    }
}
