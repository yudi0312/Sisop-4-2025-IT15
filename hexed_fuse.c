#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define SOURCE_DIR "anomali"

struct hex_file {
    char *hex_data;
    size_t size;
};

struct hex_file *hex_to_binary(const char *txt_path) {
    FILE *input = fopen(txt_path, "r");
    if (!input) return NULL;

    struct hex_file *result = malloc(sizeof(struct hex_file));
    result->hex_data = NULL;
    result->size = 0;

    char hex[3] = {0};
    unsigned char byte;
    size_t capacity = 0;

    while (fscanf(input, "%2s", hex) == 1) {
        if (result->size >= capacity) {
            capacity += 512;
            result->hex_data = realloc(result->hex_data, capacity);
        }
        byte = (unsigned char)strtol(hex, NULL, 16);
        result->hex_data[result->size++] = byte;
    }

    fclose(input);
    return result;
}

static int hex_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void)fi;
    memset(stbuf, 0, sizeof(struct stat));
    
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    char filename[256];
    if (sscanf(path, "/%255[^_]", filename) != 1) {
        return -ENOENT;
    }

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s.txt", SOURCE_DIR, filename);
    
    if (stat(full_path, stbuf) == -1) {
        return -ENOENT;
    }

    stbuf->st_mode = S_IFREG | 0644;
    stbuf->st_size /= 2;
    return 0;
}

static int hex_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi,
                      enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);

    DIR *dir = opendir(SOURCE_DIR);
    if (!dir) return -ENOENT;

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".txt")) {
            char virtual_name[256];
            snprintf(virtual_name, sizeof(virtual_name), "%.*s.png",
                     (int)(strlen(entry->d_name) - 4), entry->d_name);
            filler(buf, virtual_name, NULL, 0, FUSE_FILL_DIR_PLUS);
        }
    }

    closedir(dir);
    return 0;
}

static int hex_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static int hex_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
    char txt_name[256];
    if (sscanf(path, "/%255[^.]", txt_name) != 1) {
        return -ENOENT;
    }

    char txt_path[512];
    snprintf(txt_path, sizeof(txt_path), "%s/%s.txt", SOURCE_DIR, txt_name);

    struct hex_file *data = hex_to_binary(txt_path);
    if (!data) return -ENOENT;

    size_t len = data->size - offset;
    if (len > size) len = size;
    if (offset < data->size) {
        memcpy(buf, data->hex_data + offset, len);
    } else {
        len = 0;
    }

    free(data->hex_data);
    free(data);
    return len;
}

static struct fuse_operations hex_oper = {
    .getattr = hex_getattr,
    .readdir = hex_readdir,
    .open    = hex_open,
    .read    = hex_read,
};

int main(int argc, char *argv[]) {
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;
    char *mountpoint = NULL;
    int ret;

    if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) != 0)
        return 1;
    
    se = fuse_session_new(&args, &hex_oper, sizeof(hex_oper), NULL);
    if (!se) {
        fprintf(stderr, "Failed to create FUSE session\n");
        return 1;
    }

    if (fuse_session_mount(se, mountpoint) != 0) {
        fprintf(stderr, "Failed to mount FUSE filesystem\n");
        fuse_session_destroy(se);
        return 1;
    }

    ret = fuse_session_loop(se);

    fuse_session_unmount(se);
    fuse_session_destroy(se);
    free(mountpoint);
    fuse_opt_free_args(&args);
    
    return ret ? 1 : 0;
}