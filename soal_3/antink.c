#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SOURCE_DIR "/home/antink"
#define LOG_PATH "/var/log/it24.log"

void log_event(const char *level, const char *msg, const char *path) {
    FILE *log = fopen(LOG_PATH, "a");
    if (!log) return;
    fprintf(log, "[%02d:%02d:%02d] %s: %s: %s\n", 
            (int)(time(NULL) % 86400 / 3600), 
            (int)(time(NULL) % 3600 / 60), 
            (int)(time(NULL) % 60), 
            level, msg, path);
    fclose(log);
}

void reverse_string(char *dest, const char *src) {
    int len = strlen(src);
    for (int i = 0; i < len; i++) dest[i] = src[len - 1 - i];
    dest[len] = '\0';
}

void reverse_parts(const char *name, char *reversed) {
    char name_part[256] = {0}, ext_part[256] = {0};
    const char *dot = strrchr(name, '.');
    if (dot) {
        strncpy(name_part, name, dot - name);
        strcpy(ext_part, dot + 1);
    } else {
        strcpy(name_part, name);
    }

    char reversed_name[256] = {0}, reversed_ext[256] = {0};
    reverse_string(reversed_name, name_part);
    reverse_string(reversed_ext, ext_part);

    if (dot)
        sprintf(reversed, "%s.%s", reversed_ext, reversed_name);
    else
        strcpy(reversed, reversed_name);
}

int is_dangerous(const char *name) {
    return strstr(name, "nafis") || strstr(name, "kimcun");
}

static int antink_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    int res;

    char real_path[512];
    if (strcmp(path, "/") == 0) {
        sprintf(real_path, "%s", SOURCE_DIR);
    } else {
        char reversed[256];
        const char *req = path + 1;

        if (is_dangerous(req)) {
            reverse_parts(req, reversed);
            log_event("WARNING", "Detected dangerous file", reversed);
            return -ENOENT;
        }
        sprintf(real_path, "%s/%s", SOURCE_DIR, req);
    }

    res = lstat(real_path, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    DIR *dp;
    struct dirent *de;
    char real_path[512];
    sprintf(real_path, "%s", SOURCE_DIR);

    dp = opendir(real_path);
    if (dp == NULL)
        return -errno;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

        char shown_name[256];
        if (is_dangerous(de->d_name)) {
            reverse_parts(de->d_name, shown_name);
        } else {
            strcpy(shown_name, de->d_name);
        }

        struct stat st = {0};
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        filler(buf, shown_name, &st, 0, 0);
    }

    closedir(dp);
    return 0;
}

void rot13(char *buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if ((buf[i] >= 'a' && buf[i] <= 'z'))
            buf[i] = 'a' + (buf[i] - 'a' + 13) % 26;
        else if ((buf[i] >= 'A' && buf[i] <= 'Z'))
            buf[i] = 'A' + (buf[i] - 'A' + 13) % 26;
    }
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    (void) fi;
    int fd;
    int res;

    char real_path[512];
    const char *req = path + 1;

    if (is_dangerous(req)) {
        char reversed[256];
        reverse_parts(req, reversed);
        log_event("WARNING", "Detected dangerous file", reversed);
        return -EACCES;
    }

    sprintf(real_path, "%s/%s", SOURCE_DIR, req);
    log_event("READ", "", req);

    fd = open(real_path, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    } else {
        rot13(buf, res);
    }
    close(fd);
    return res;
}

static const struct fuse_operations antink_oper = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .read = antink_read,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &antink_oper, NULL);
}
