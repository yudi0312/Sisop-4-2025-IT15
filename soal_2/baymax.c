
#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdbool.h>
#include <glib.h>
#include <pthread.h>

#define RELIC_DIR "/home/yudi0312/PRAKTIKUM_SISOP/Sisop-4-2025-IT15/soal_2/relics"
#define LOG_FILE "/home/yudi0312/PRAKTIKUM_SISOP/Sisop-4-2025-IT15/soal_2/activity.log"
#define MAX_PIECE_SIZE 1024
static const char *MOUNT_DIR = "/home/yudi0312/PRAKTIKUM_SISOP/Sisop-4-2025-IT15/soal_2/mount_dir";

void write_log(const char *message) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, message);
    fclose(log);
}

typedef struct {
    bool is_copy;
} file_status_t;

GHashTable *file_status_table;
pthread_mutex_t status_lock = PTHREAD_MUTEX_INITIALIZER;

static int fs_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    memset(st, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }

    char base[256];
    snprintf(base, sizeof(base), "%s", path + 1);

    char frag_path[512];
    snprintf(frag_path, sizeof(frag_path), "%s/%s.000", RELIC_DIR, base);

    FILE *f = fopen(frag_path, "rb");
    if (!f) return -ENOENT;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fclose(f);

    int i = 1;
    char tmp[512];
    while (1) {
        snprintf(tmp, sizeof(tmp), "%s/%s.%03d", RELIC_DIR, base, i);
        FILE *next = fopen(tmp, "rb");
        if (!next) break;
        fseek(next, 0, SEEK_END);
        size += ftell(next);
        fclose(next);
        i++;
    }

    st->st_mode = S_IFREG | 0644;
    st->st_nlink = 1;
    st->st_size = size;
    return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    DIR *dir = opendir(RELIC_DIR);
    if (!dir) return -ENOENT;

    struct dirent *dp;
    char seen[100][256];
    int seen_count = 0;

    while ((dp = readdir(dir)) != NULL) {
        char *dot = strrchr(dp->d_name, '.');
        if (!dot || strlen(dot) != 4) continue;
        char name[256];
        strncpy(name, dp->d_name, strlen(dp->d_name) - 4);
        name[strlen(dp->d_name) - 4] = '\0';

        int already_seen = 0;
        for (int i = 0; i < seen_count; i++) {
            if (strcmp(seen[i], name) == 0) {
                already_seen = 1;
                break;
            }
        }
        if (!already_seen) {
            filler(buf, name, NULL, 0, 0);
            strcpy(seen[seen_count++], name);
        }
    }
    closedir(dir);
    return 0;
}

static int fs_open(const char *path, struct fuse_file_info *fi) {
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    char frag[512];
    snprintf(frag, sizeof(frag), "%s/%s.000", RELIC_DIR, name);
    int fd = open(frag, O_RDONLY);
    if (fd == -1) return -ENOENT;

    fi->fh = fd;

    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    pthread_mutex_lock(&status_lock);
    file_status_t *status = g_hash_table_lookup(file_status_table, name);
  
    bool is_outside_mount = strncmp(cwd, MOUNT_DIR, strlen(MOUNT_DIR)) != 0;
    if (is_outside_mount) {

        if (!status) {
            status = malloc(sizeof(file_status_t));
            status->is_copy = true;
            g_hash_table_insert(file_status_table, strdup(name), status);

            char log_msg[1024];
            snprintf(log_msg, sizeof(log_msg), "COPY: %s -> /tmp/%s", name, name);
            write_log(log_msg);
        }

    } else {

        if (!status || !status->is_copy) {

            char log_msg[1024];
            snprintf(log_msg, sizeof(log_msg), "READ: %s", name);
            write_log(log_msg);
        }
    }

    pthread_mutex_unlock(&status_lock);
    return 0;
}


static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    size_t read_bytes = 0;
    size_t pos = 0;
    int i = 0;
    while (1) {
        char frag_path[512];
        snprintf(frag_path, sizeof(frag_path), "%s/%s.%03d", RELIC_DIR, name, i);
        FILE *f = fopen(frag_path, "rb");
        if (!f) break;

        fseek(f, 0, SEEK_END);
        size_t frag_size = ftell(f);
        rewind(f);

        if (offset < pos + frag_size) {
            fseek(f, offset > pos ? offset - pos : 0, SEEK_SET);
            size_t to_read = (size - read_bytes < frag_size) ? size - read_bytes : frag_size;
            to_read = to_read < MAX_PIECE_SIZE ? to_read : MAX_PIECE_SIZE;
            read_bytes += fread(buf + read_bytes, 1, to_read, f);
            fclose(f);
            i++;
            if (read_bytes >= size) break;
        } else {
            pos += frag_size;
            fclose(f);
            i++;
        }
    }
    return read_bytes;
}

static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    char frag_path[512];
    snprintf(frag_path, sizeof(frag_path), "%s/%s.000", RELIC_DIR, name);
    FILE *f = fopen(frag_path, "wb");
    if (!f) return -EACCES;
    fclose(f);

    int fd = open(frag_path, O_RDWR);
    if (fd == -1) return -EACCES;
    fi->fh = fd;

    return 0;
}


static int fs_write(const char *path, const char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    int pieces = 0;
    for (size_t i = 0; i < size; i += MAX_PIECE_SIZE, pieces++) {
        char frag_path[512];
        snprintf(frag_path, sizeof(frag_path), "%s/%s.%03d", RELIC_DIR, name, pieces);
        FILE *f = fopen(frag_path, "wb");
        fwrite(buf + i, 1, (size - i > MAX_PIECE_SIZE ? MAX_PIECE_SIZE : size - i), f);
        fclose(f);
    }

    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "WRITE: %s -> %s.000 to %s.%03d", name, name, name, pieces - 1);
    write_log(log_msg);

    return size;
}

static int fs_unlink(const char *path) {
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    int i = 0;
    char frag[512];
    while (1) {
        snprintf(frag, sizeof(frag), "%s/%s.%03d", RELIC_DIR, name, i);
        if (access(frag, F_OK) != 0) break;
        remove(frag);
        i++;
    }

    if (i > 0) {
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "DELETE: %s.000 - %s.%03d", name, name, i - 1);
        write_log(log_msg);
    }

    return 0;
}

static int fs_release(const char *path, struct fuse_file_info *fi) {
    close(fi->fh);
    return 0;
}

static const struct fuse_operations fs_oper = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open    = fs_open,
    .read    = fs_read,
    .write   = fs_write,
    .create  = fs_create,
    .unlink  = fs_unlink,
    .release = fs_release,
};

int main(int argc, char *argv[]) {
    file_status_table = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);

    DIR *dir = opendir(RELIC_DIR);
    if (dir) {
        struct dirent *dp;
        GHashTable *seen_files = g_hash_table_new(g_str_hash, g_str_equal);

        while ((dp = readdir(dir)) != NULL) {
            char *dot = strrchr(dp->d_name, '.');
            if (!dot || strlen(dot) != 4) continue;

            char name[256];
            strncpy(name, dp->d_name, strlen(dp->d_name) - 4);
            name[strlen(dp->d_name) - 4] = '\0';

            if (!g_hash_table_contains(seen_files, name)) {
                g_hash_table_insert(seen_files, strdup(name), GINT_TO_POINTER(1));

                pthread_mutex_lock(&status_lock);
                file_status_t *status = g_hash_table_lookup(file_status_table, name);
                if (!status || !status->is_copy) {
                    char log_msg[1024];
                    snprintf(log_msg, sizeof(log_msg), "READ: %s", name);
                    write_log(log_msg);
                }
                pthread_mutex_unlock(&status_lock);
            }
        }
        g_hash_table_destroy(seen_files);
        closedir(dir);
    }

    return fuse_main(argc, argv, &fs_oper, NULL);
}
