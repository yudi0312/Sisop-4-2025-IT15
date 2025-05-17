#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PATH 1024

void reverse_str(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char tmp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = tmp;
    }
}

void reverse_filename(char *dest, const char *filename) {
    char name[256], ext[256];
    memset(name, 0, sizeof(name));
    memset(ext, 0, sizeof(ext));

    char *dot = strrchr(filename, '.');
    if (dot) {
        strncpy(name, filename, dot - filename);
        strcpy(ext, dot + 1);
    } else {
        strcpy(name, filename);
    }

    reverse_str(name);
    reverse_str(ext);

    if (strlen(ext) > 0)
        sprintf(dest, "%s.%s", ext, name);
    else
        sprintf(dest, "%s", name);
}

void rot13(char *buf, int len) {
    for (int i = 0; i < len; i++) {
        char c = buf[i];
        if ('a' <= c && c <= 'z')
            buf[i] = 'a' + (c - 'a' + 13) % 26;
        else if ('A' <= c && c <= 'Z')
            buf[i] = 'A' + (c - 'A' + 13) % 26;
    }
}

void log_action(const char *fmt, const char *param) {
    FILE *log = fopen("/var/log/it24.log", "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
    fprintf(log, fmt, param);
    fprintf(log, "\n");
    fclose(log);
}

static int antink_getattr(const char *path, struct stat *stbuf) {
    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "/it24_host%s", path);
    return lstat(fullpath, stbuf);
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;
    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "/it24_host%s", path);

    dp = opendir(fullpath);
    if (!dp) return -errno;

    while ((de = readdir(dp)) != NULL) {
        char reversed[256];
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        if (strstr(de->d_name, "nafis") || strstr(de->d_name, "kimcun")) {
            char target[MAX_PATH];
            snprintf(target, sizeof(target), "%s/%s", fullpath, de->d_name);
            remove(target);
            log_action("WARNING: Detected dangerous file: %s", de->d_name);
            continue;
        }

        reverse_filename(reversed, de->d_name);

        struct stat st = {0};
        filler(buf, reversed, &st, 0);
    }
    closedir(dp);
    return 0;
}

static int antink_open(const char *path, struct fuse_file_info *fi) {
    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "/it24_host%s", path);

    int res = open(fullpath, O_RDONLY);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "/it24_host%s", path);

    FILE *fp = fopen(fullpath, "r");
    if (!fp) return -errno;

    fseek(fp, offset, SEEK_SET);
    size_t res = fread(buf, 1, size, fp);
    fclose(fp);

    rot13(buf, res);

    log_action("READ: %s", path);
    return res;
}

static struct fuse_operations antink_oper = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open    = antink_open,
    .read    = antink_read,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &antink_oper, NULL);
}
