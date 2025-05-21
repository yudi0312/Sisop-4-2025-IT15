#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>

#define RELIC_PATH "/home/yudi0312/PRAKTIKUM_SISOP/Sisop-4-2025-IT15/soal_2/relics"
#define LOG_FILE "/home/yudi0312/PRAKTIKUM_SISOP/Sisop-4-2025-IT15/soal_2/activity.log"
#define CHUNK_SIZE 1024
#define MAX_CHUNKS 100

pid_t last_open_pid = 0;


void write_log(const char *format, ...) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(log, format, args);
    va_end(args);

    fprintf(log, "\n");
    fclose(log);
}

static char last_open_file[256] = {0};
static int last_open_mode = 0; 
static int copy_logged = 0;

static int baymax_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    const char *filename = path + 1; 

    char chunk_path[512];
    off_t total_size = 0;
    int found = 0;

    for (int i = 0; i < MAX_CHUNKS; i++) {
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_PATH, filename, i);
        FILE *fp = fopen(chunk_path, "rb");
        if (!fp) {
            if (i == 0 && !found) return -ENOENT;
            break;
        }
        found = 1;
        fseek(fp, 0, SEEK_END);
        total_size += ftell(fp);
        fclose(fp);
    }

    if (!found)
        return -ENOENT;

    stbuf->st_mode = S_IFREG | 0644;
    stbuf->st_nlink = 1;
    stbuf->st_size = total_size;

    return 0;
}

static int baymax_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    DIR *d = opendir(RELIC_PATH);
    if (d) {
        struct dirent *entry;
        char last_file[256] = {0};
        while ((entry = readdir(d)) != NULL) {
            char *dot = strrchr(entry->d_name, '.');
            if (!dot) continue;
            if (strlen(dot) != 4) continue;

            char base_name[256];
            size_t len = dot - entry->d_name;
            if (len >= sizeof(base_name)) len = sizeof(base_name) - 1;
            strncpy(base_name, entry->d_name, len);
            base_name[len] = '\0';

            if (strcmp(base_name, last_file) != 0) {
                filler(buf, base_name, NULL, 0, 0);
                strcpy(last_file, base_name);
            }
        }
        closedir(d);
    }

    return 0;
}

static int baymax_open(const char *path, struct fuse_file_info *fi) {
    const char *filename = path + 1;

    FILE *fp;
    char chunk_path[512];
    snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_PATH, filename, 0);

    fp = fopen(chunk_path, "rb");
    if (!fp) return -ENOENT;
    fclose(fp);

    strncpy(last_open_file, filename, sizeof(last_open_file));
    last_open_mode = (fi->flags & O_ACCMODE) == O_RDONLY ? 1 : 0;
    last_open_pid = fuse_get_context()->pid; 
    copy_logged = 0;

    return 0;
}


static int baymax_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    (void) fi;
    const char *filename = path + 1;
    char proc_path[64], proc_name[256] = "";
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/comm", last_open_pid);
    FILE *f = fopen(proc_path, "r");
    if (f) {
        fgets(proc_name, sizeof(proc_name), f);
        proc_name[strcspn(proc_name, "\n")] = 0; 
        fclose(f);
    }
    if (last_open_mode == 1 && strcmp(filename, last_open_file) == 0 && !copy_logged) {
        if (strcmp(proc_name, "cp") == 0) {
            write_log("COPY: %s -> /tmp/%s", filename, filename);
            copy_logged = 1;
        } else if (
            strcmp(proc_name, "ls") != 0 &&
            strcmp(proc_name, "fuse") != 0 &&
            strcmp(proc_name, "fusermount3") != 0 &&
            strcmp(proc_name, "bash") != 0 &&
            strcmp(proc_name, "gnome-terminal-") != 0 &&
            strcmp(proc_name, "code") != 0  
        ) {
            write_log("READ: %s", filename);
        }
    }

    size_t total_read = 0;
    off_t current_offset = 0;
    char chunk_path[512];

    for (int i = 0; i < MAX_CHUNKS && size > 0; i++) {
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_PATH, filename, i);
        FILE *fp = fopen(chunk_path, "rb");
        if (!fp) break;

        fseek(fp, 0, SEEK_END);
        size_t chunk_size = ftell(fp);
        rewind(fp);

        if (offset < current_offset + chunk_size) {
            size_t start = offset > current_offset ? (size_t)(offset - current_offset) : 0;
            size_t to_read = chunk_size - start;
            if (to_read > size) to_read = size;

            size_t read_bytes = fread(buf + total_read, 1, to_read, fp);
            total_read += read_bytes;
            size -= read_bytes;
        }
        current_offset += chunk_size;
        fclose(fp);
    }

    return total_read;
}
static int baymax_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) mode;
    (void) fi;
    const char *filename = path + 1;
    char chunk_path[512];

    snprintf(chunk_path, sizeof(chunk_path), "%s/%s.000", RELIC_PATH, filename);

    FILE *fp = fopen(chunk_path, "wb");
    if (!fp)
        return -EIO;

    fclose(fp);
    return 0;
}
static int baymax_write(const char *path, const char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    (void) fi;
    if (offset != 0)
        return 0;

    const char *filename = path + 1;
    char chunk_path[512];

    size_t written = 0;
    int chunk_index = 0;

    char chunk_list[1024] = {0};
    size_t chunk_list_len = 0;

    while (written < size) {
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_PATH, filename, chunk_index);
        FILE *fp = fopen(chunk_path, "wb");
        if (!fp)
            return -EIO;

        size_t to_write = CHUNK_SIZE;
        if (written + to_write > size)
            to_write = size - written;

        fwrite(buf + written, 1, to_write, fp);
        fclose(fp);
        written += to_write;
        
        char temp[64];
        snprintf(temp, sizeof(temp), "%s.%03d", filename, chunk_index);
        if (chunk_index > 0) {
            if (chunk_list_len + 2 < sizeof(chunk_list)) {
                strcat(chunk_list, ", ");
                chunk_list_len += 2;
            }
        }
        if (chunk_list_len + strlen(temp) < sizeof(chunk_list)) {
            strcat(chunk_list, temp);
            chunk_list_len += strlen(temp);
        }

        chunk_index++;
    }

    write_log("WRITE: %s -> %s", filename, chunk_list);

    return written;
}

static int baymax_unlink(const char *path) {
    const char *filename = path + 1;
    char chunk_path[512];
    int i;
    int removed = 0;

    for (i = 0; i < MAX_CHUNKS; i++) {
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_PATH, filename, i);
        if (access(chunk_path, F_OK) != 0)
            break;
        remove(chunk_path);
        removed = 1;
    }

    if (!removed)
        return -ENOENT;

    if (i > 1)
        write_log("DELETE: %s.%03d - %s.%03d", filename, 0, filename, i - 1);
    else
        write_log("DELETE: %s.%03d", filename, 0);

    return 0;
}

static struct fuse_operations baymax_oper = {
    .getattr = baymax_getattr,
    .readdir = baymax_readdir,
    .open = baymax_open,
    .read = baymax_read,
    .create = baymax_create,
    .write = baymax_write,
    .unlink = baymax_unlink,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &baymax_oper, NULL);
}
