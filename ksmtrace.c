// https://stackoverflow.com/questions/12411942/pread-and-pwrite-not-defined
#define _XOPEN_SOURCE 500
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
 


#ifdef __APPLE__
static const char PKSM_PATH[] = "./";
#else
static const char PKSM_PATH[] = "/sys/kernel/mm/pksm/";
#endif

struct file_t {
    char *filename;
    unsigned long value;
    int fd; 
    bool open; // if the @fd is open
};


// code example
int list_dir(void) {
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
    return 0;
}

// get all regular flie name.
// return the number of files
int get_files(const char *path, struct file_t **data_files) {
    int num_files = 0;
    int array_size = 10;
    struct file_t *files = (struct file_t *)malloc(sizeof(struct file_t) * array_size);
    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    if (!d) {
        fprintf(stderr, "error open dirrectory %s: %s (%d)\n", path,
                strerror(errno), errno);
        return 0;
    }
    while ((dir = readdir(d)) != NULL) {
#define DT_REG           8
        if (dir->d_type != DT_REG)
            continue;
        int len = strlen(dir->d_name);
        int path_len = strlen(PKSM_PATH);
        /* printf("len = %d\n", len); */
        files[num_files].filename = (char *)malloc(sizeof(char) * (len + path_len + 2));
        strcpy(files[num_files].filename, PKSM_PATH);
        strcpy(files[num_files].filename + path_len, dir->d_name);
        files[num_files].open = false;
        num_files++;
        if (num_files == array_size) {
            array_size += 10;
            files = realloc(files, array_size * sizeof(struct file_t));
        }
    }
    closedir(d);
    *data_files = files;
    return num_files;
}

void print_files(const struct file_t *files, int len) {
    printf("total %d files\n", len);
    while (len--) {
        printf("%s\n", files->filename);
        files++;
    }
}

void print_file_values(const struct file_t *files, int len) {
    while (len--) {
        printf("%s = %lu\n", files->filename, files->value);
        files++;
    }
}



static char *g_buf;                 // buffer for reading files
static int g_pageSize;
static bool g_keep_running;
static const char *g_log_file_name = "pksmlog.csv";


bool read_file(struct file_t * const f)
{
    if (!f->open) {
        f->fd = open(f->filename, O_RDONLY);
        if (f->fd == -1) {
            fprintf(stderr, "error opening %s: %s (%d)\n", f->filename,
                    strerror(errno), errno);
            close(f->fd);
            return false;
        }
        f->open = true;
    }

    int result;
    memset(g_buf, '\0', g_pageSize);

    result = pread(f->fd, g_buf, g_pageSize, 0);
    if (result < 0) {
        fprintf(stderr, "error reading file %s: %s (%d)\n", f->filename,
                strerror(errno), errno);
        close(f->fd);
        f->open = false;
        return false;
    }
    char *end;
    f->value = strtoul(g_buf, &end, 10);
    return true;
}


bool read_files(struct file_t *files, int len)
{
    bool ok = true;
    while (len--) {
        ok &= read_file(files++);
    }
    return ok;
}

FILE* open_out_file(const char* filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "error opening %s: %s (%d)\n", filename,
                strerror(errno), errno);
        return NULL;
    }
    return file;
}


#define clear() printf("\033[H\033[J")

void usage(const char *prog_name)
{
    printf("%s: read pksm's sysfiles.\n", prog_name);
    printf("   default behavir: moniter pksm sysfile data\n");
    printf("   press <ctrl>-C to stop \n");
    printf("Usage: %s [-lqh]\n", prog_name);
    printf("    -l : log mode, write to file \"%s\"\n", g_log_file_name);
    printf("    -q : quiet mode\n");
    printf("    -h : print this help\n");
}

void intHandler(int n __attribute__((__unused__))) {
    g_keep_running = false;
}

int main(int argc __attribute__((unused)),
        char* argv[] __attribute__((unused)))
{

    signal(SIGINT, intHandler);
    bool is_log = false;
    bool is_quiet = false;
    /* int opt; */
    /* while ((opt = getopt(argc, argv, "lhq")) != -1) { */
    /*     switch(opt) { */
    /*         case 'l' : is_log = true; break; */
    /*         case 'q' : is_quiet = true; break; */
    /*         case 'h' : */
    /*         default: usage(argv[0]); */
    /*                  exit(0); */
    /*     } */
    /* } */


    struct file_t *files;
    int num_files = get_files(PKSM_PATH, &files);
    
    /* print_files(files, num_files); */

    /* g_pageSize = getpagesize(); */
    g_pageSize = 4096;
    printf("page size is %d bytes\n", g_pageSize);

    g_buf = malloc(g_pageSize); // seems to be unnaccesary, don't need such a huge buffer

    FILE *file;
    if (is_log) {
        file = open_out_file(g_log_file_name);
        /* write_status_header(file); */
    }

    g_keep_running = true;
    while (g_keep_running) {
        if(read_files(files, num_files)) {
            if (!is_quiet) {
                clear();
                print_file_values(files, num_files);
            }
            /* if (is_log) */
                /* write_status(file); */
        } else {
            fprintf(stderr, "read status failed\n") ;
        }

        sleep(1);
        /* struct timespec rqtp; */
        /* rqtp.tv_sec = 0; */
        /* rqtp.tv_nsec = 500 * 1000* 1000; // 500 ms */
        /* while (nanosleep(&rqtp, &rqtp) == -1 && errno == EINTR) */
        /*     ; */
    }

    free(g_buf);
    if (is_log)
        fclose(file);
    return 0;
}


