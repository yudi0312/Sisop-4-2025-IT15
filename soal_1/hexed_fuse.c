#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <stdio.h>
#include <ctype.h>     
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>

#define ZIP_URL        "https://drive.google.com/uc?export=download&id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5"
#define ROOT_DIR       "anomali"
#define HEX_DIR        ROOT_DIR "/anomali"
#define IMG_DIR        ROOT_DIR "/image"
#define LOG_FILE       ROOT_DIR "/conversion.log"
#define ZIP_FILE       "anomali.zip"

// forward declarations
static void ensure_dirs(void);
static void download_and_unpack(void);
static void convert_all_hex(void);
static unsigned char hex2byte(char hi, char lo);

// FUSE ops
static const char *basepath = ROOT_DIR;
static int xmp_getattr(const char *path, struct stat *stbuf);
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi);
static int xmp_open(const char *path, struct fuse_file_info *fi);
static int xmp_read(const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi);

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open    = xmp_open,
    .read    = xmp_read,
};

int main(int argc, char *argv[]) {
    ensure_dirs();
    download_and_unpack();
    convert_all_hex();

    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}

// pastikan direktori anomali/, anomali/anomali/, anomali/image/ ada
static void ensure_dirs(void) {
    struct stat st;
    if (stat(ROOT_DIR, &st) == -1)  mkdir(ROOT_DIR, 0755);
    if (stat(HEX_DIR,  &st) == -1)  mkdir(HEX_DIR,  0755);
    if (stat(IMG_DIR,  &st) == -1)  mkdir(IMG_DIR,  0755);
}

// wget + unzip
static void download_and_unpack(void) {
    char cmd[512];
    // download
    snprintf(cmd, sizeof(cmd),
      "wget -q --show-progress \"%s\" -O %s", ZIP_URL, ZIP_FILE);
    system(cmd);
    // unzip into ROOT_DIR
    snprintf(cmd, sizeof(cmd),
      "unzip -q %s -d %s", ZIP_FILE, ROOT_DIR);
    system(cmd);
    remove(ZIP_FILE);
}

// scan semua .txt di HEX_DIR, convert ke IMG_DIR/*.png
static void convert_all_hex(void) {
    DIR *d = opendir(HEX_DIR);
    if (!d) return;
    FILE *logf = fopen(LOG_FILE, "a");
    if (!logf) { closedir(d); return; }

    struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_type == DT_REG && strstr(e->d_name, ".txt")) {
            // baca seluruh file txt
            char inpath[512];
            snprintf(inpath, sizeof(inpath), "%s/%s", HEX_DIR, e->d_name);
            FILE *fin = fopen(inpath, "r");
            if (!fin) continue;
            fseek(fin, 0, SEEK_END);
            long sz = ftell(fin);
            fseek(fin, 0, SEEK_SET);
            char *hex = malloc(sz + 1);
            fread(hex, 1, sz, fin);
            hex[sz] = 0;
            fclose(fin);

            // nama base
            char base[64];
            size_t len = strchr(e->d_name, '.') - e->d_name;
            strncpy(base, e->d_name, len);
            base[len] = '\0';

            // timestamp
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            char stamps[32];
            strftime(stamps, sizeof(stamps), "%Y-%m-%d_%H:%M:%S", tm);

            // output path
            char outname[128], outpath[256];
            snprintf(outname, sizeof(outname), "%s_image_%s.png", base, stamps);
            snprintf(outpath, sizeof(outpath), "%s/%s", IMG_DIR, outname);

            // konversi
            FILE *fout = fopen(outpath, "wb");
            for (char *p = hex; *(p+1); p += 2) {
                if (isxdigit((unsigned char)p[0]) && isxdigit((unsigned char)p[1])) {
                    fputc(hex2byte(p[0], p[1]), fout);
                }
            }
            fclose(fout);
            free(hex);

            // log
            fprintf(logf,
              "[%04d-%02d-%02d][%02d:%02d:%02d]: Successfully converted hexadecimal text %s to %s\n",
              tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
              tm->tm_hour, tm->tm_min, tm->tm_sec,
              e->d_name, outname);
        }
    }
    fclose(logf);
    closedir(d);
}

static unsigned char hex2byte(char hi, char lo) {
    hi = tolower((unsigned char)hi);
    lo = tolower((unsigned char)lo);
    unsigned char vhi = isdigit(hi) ? hi - '0' : hi - 'a' + 10;
    unsigned char vlo = isdigit(lo) ? lo - '0' : lo - 'a' + 10;
    return (vhi << 4) | vlo;
}

// --- FUSE callbacks ---

static int xmp_getattr(const char *path, struct stat *stbuf) {
    char fpath[512];
    snprintf(fpath, sizeof(fpath), "%s%s", basepath, path);
    int res = lstat(fpath, stbuf);
    return res == -1 ? -errno : 0;
}

static int xmp_readdir(const char *path, void *buf,
                       fuse_fill_dir_t filler, off_t offset,
                       struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;
    char fpath[512];
    snprintf(fpath, sizeof(fpath), "%s%s", basepath, path);

    dp = opendir(fpath);
    if (!dp) return -errno;
    while ((de = readdir(dp))) {
        struct stat st = {0};
        st.st_ino  = de->d_ino;
        st.st_mode = de->d_type << 12;
        filler(buf, de->d_name, &st, 0);
    }
    closedir(dp);

    // hilangkan warning unused parameter
    (void)offset;
    (void)fi;

    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    char fpath[512];
    snprintf(fpath, sizeof(fpath), "%s%s", basepath, path);
    int fd = open(fpath, fi->flags);
    if (fd < 0) return -errno;
    close(fd);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
    char fpath[512];
    snprintf(fpath, sizeof(fpath), "%s%s", basepath, path);
    int fd = open(fpath, O_RDONLY);
    if (fd < 0) return -errno;
    int res = pread(fd, buf, size, offset);
    close(fd);

    // hilangkan warning unused parameter
    (void)fi;

    return res < 0 ? -errno : res;
}