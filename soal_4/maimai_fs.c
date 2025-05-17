#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <zlib.h>

#define ROOT_DIR "/path/to/fuse_dir"
#define EXT_MAI ".mai"
#define EXT_GZ ".gz"
#define AES_KEY "0123456789abcdef0123456789abcdef"
#define AES_BLOCK_SIZE 16

static const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "youth", "7sref"};

// Helper: Generate real path
void get_real_path(const char *path, char *out_path) {
    sprintf(out_path, "%s%s", ROOT_DIR, path);
}

// Helper: Check area prefix
const char *get_area(const char *path) {
    for (int i = 0; i < 7; i++) {
        if (strncmp(path + 1, areas[i], strlen(areas[i])) == 0) return areas[i];
    }
    return NULL;
}

// ROT13
void rot13(char *s) {
    for (int i = 0; s[i]; i++) {
        if ((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z')) {
            char base = (s[i] >= 'a') ? 'a' : 'A';
            s[i] = (s[i] - base + 13) % 26 + base;
        }
    }
}

// Shift encode (Metro)
void shift_encode(char *s) {
    for (int i = 0; s[i]; i++) {
        s[i] = s[i] + ((i + 1) % 256);
    }
}

// AES Encrypt/Decrypt
int aes_encrypt(const char *infile, const char *outfile) {
    FILE *in = fopen(infile, "rb");
    FILE *out = fopen(outfile, "wb");
    if (!in || !out) return -1;

    unsigned char key[32], iv[AES_BLOCK_SIZE];
    memcpy(key, AES_KEY, 32);
    RAND_bytes(iv, AES_BLOCK_SIZE);
    fwrite(iv, 1, AES_BLOCK_SIZE, out);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    unsigned char inbuf[1024], outbuf[1040];
    int inlen, outlen;
    while ((inlen = fread(inbuf, 1, 1024, in)) > 0) {
        EVP_EncryptUpdate(ctx, outbuf, &outlen, inbuf, inlen);
        fwrite(outbuf, 1, outlen, out);
    }
    EVP_EncryptFinal_ex(ctx, outbuf, &outlen);
    fwrite(outbuf, 1, outlen, out);

    EVP_CIPHER_CTX_free(ctx);
    fclose(in); fclose(out);
    return 0;
}

int aes_decrypt(const char *infile, const char *outfile) {
    FILE *in = fopen(infile, "rb");
    FILE *out = fopen(outfile, "wb");
    if (!in || !out) return -1;

    unsigned char key[32], iv[AES_BLOCK_SIZE];
    fread(iv, 1, AES_BLOCK_SIZE, in);
    memcpy(key, AES_KEY, 32);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    unsigned char inbuf[1024], outbuf[1040];
    int inlen, outlen;
    while ((inlen = fread(inbuf, 1, 1024, in)) > 0) {
        EVP_DecryptUpdate(ctx, outbuf, &outlen, inbuf, inlen);
        fwrite(outbuf, 1, outlen, out);
    }
    EVP_DecryptFinal_ex(ctx, outbuf, &outlen);
    fwrite(outbuf, 1, outlen, out);

    EVP_CIPHER_CTX_free(ctx);
    fclose(in); fclose(out);
    return 0;
}

// gzip compress & decompress
int gzip_compress(const char *infile, const char *outfile) {
    FILE *in = fopen(infile, "rb");
    gzFile out = gzopen(outfile, "wb");
    if (!in || !out) return -1;
    char buf[1024];
    int len;
    while ((len = fread(buf, 1, 1024, in)) > 0) {
        gzwrite(out, buf, len);
    }
    fclose(in);
    gzclose(out);
    return 0;
}

int gzip_decompress(const char *infile, const char *outfile) {
    gzFile in = gzopen(infile, "rb");
    FILE *out = fopen(outfile, "wb");
    if (!in || !out) return -1;
    char buf[1024];
    int len;
    while ((len = gzread(in, buf, 1024)) > 0) {
        fwrite(buf, 1, len, out);
    }
    gzclose(in);
    fclose(out);
    return 0;
}

// Implementasi dasar getattr, readdir, read, write, create, unlink
// Untuk setiap chiho, buatkan logika pada masing-masing syscall sesuai ketentuan soal

static struct fuse_operations chiho_oper = {
    // .getattr = chiho_getattr,
    // .readdir = chiho_readdir,
    // .read = chiho_read,
    // .write = chiho_write,
    // .create = chiho_create,
    // .unlink = chiho_unlink,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &chiho_oper, NULL);
}
