# Praktikum Sisop Modul 4-2025-IT15

Anggota kelompok : 
- Putu Yudi Nandanjaya Wiraguna	5027241080
- Naruna Vicranthyo Putra Gangga	5027241105
- Az Zahrra Tasya Adelia	        5027241087

---
## Soal 1 - Hex->Image
Author : Az Zahrra Tasya Adelia - 5027241087

### Deskripsi

a. Mengambil sampel anomali .txt dan memastikan ZIP terhapus
Fungsi download_and_unpack() melakukan:

1. Download ZIP dari Google Drive menggunakan:

```
snprintf(cmd, sizeof(cmd),
         "wget -q --show-progress \"%s\" -O %s",
         ZIP_URL, ZIP_FILE);
system(cmd);
```

2. Ekstrak ZIP ke folder anomali/anomali/ (HEX_DIR):

```
snprintf(cmd, sizeof(cmd),
         "unzip -oq %s -d %s",
         ZIP_FILE, TXT_DIR);
system(cmd);
```

3. Hapus file ZIP segera setelah ekstraksi:

```
 remove(ZIP_FILE);
```

b. Mengonversi string heksadesimal menjadi file image di direktori image/

1. Fungsi convert_all_hex() melakukan scan anomali/anomali/:

```
DIR *d = opendir(TXT_DIR);
while ((e = readdir(d))) {
    if (e->d_type==DT_REG && strstr(e->d_name, ".txt")) {
        // buka dan baca seluruh isi file ke buffer `hex`
        // … lalu:
        FILE *fout = fopen(png_path, "wb");
        for (char *p = hex; p[0] && p[1]; p += 2) {
            if (isxdigit(p[0]) && isxdigit(p[1]))
                fputc(hex2byte(p[0],p[1]), fout);
        }
        fclose(fout);
    }
}
```
-hex2byte() mengubah dua karakter hex menjadi satu byte biner, dan fputc menulisnya ke file .png.
-Semua gambar disimpan di folder anomali/image/ yang dibuat oleh ensure_dirs().

c. 
[nama file asli tanpa .txt]_image_[YYYY-mm-dd]_[HH:MM:SS].png


```
char base[256];
size_t L = strlen(e->d_name);
// ambil nama sebelum “.txt”
strncpy(base, e->d_name, L-4);
base[L-4] = '\0';

// tanggal & waktu
time_t t = time(NULL);
struct tm *tm = localtime(&t);
char date[11], tmstr[9];
strftime(date,  sizeof(date),  "%Y-%m-%d", tm);
strftime(tmstr, sizeof(tmstr), "%H:%M:%S", tm);

// bentuk nama output
snprintf(png_name, sizeof(png_name),
         "%s_image_%s_%s.png", base, date, tmstr);
```


![image](https://github.com/user-attachments/assets/43c49e6e-af66-456c-bbd6-f79764caa01a)

d. Mencatat setiap konversi dalam conversion.log

```
fprintf(logf,
    "[%04d-%02d-%02d][%02d:%02d:%02d]: Successfully converted hexadecimal text %s to %s\n",
    tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
    tm->tm_hour, tm->tm_min, tm->tm_sec,
    e->d_name, png_name);
```


![image](https://github.com/user-attachments/assets/83017056-7196-4e8e-99f1-e2a46926af27)


-hex pada text 1

![image](https://github.com/user-attachments/assets/f632c173-44c7-4d1e-8441-f39571d26a23)


-contoh gambar 1

![image](https://github.com/user-attachments/assets/e83a5a3e-ec0f-403f-a167-fecc379aa043)


Kendala dalam pengerjaan: 

![Screenshot 2025-05-22 161806](https://github.com/user-attachments/assets/f39d4fbb-865f-494a-9bb8-6a279763dda4)

ketika text masih belum bisa di convert.

---

## Soal 2 - Baymax
Author : Putu Yudi Nandanjaya Wiraguna - 5027241080

### Deskripsi 

Pada suatu hari, seorang ilmuwan muda menemukan sebuah drive tua yang tertanam di reruntuhan laboratorium robotik. Saat diperiksa, drive tersebut berisi pecahan data dari satu-satunya robot perawat legendaris yang dikenal dengan nama Baymax. Sayangnya, akibat kerusakan sistem selama bertahun-tahun, file utuh Baymax telah terfragmentasi menjadi 14 bagian kecil, masing-masing berukuran 1 kilobyte, dan tersimpan dalam direktori bernama relics. Pecahan tersebut diberi nama berurutan seperti Baymax.jpeg.000, Baymax.jpeg.001, hingga Baymax.jpeg.013. Ilmuwan tersebut kini ingin membangkitkan kembali Baymax ke dalam bentuk digital yang utuh, namun ia tidak ingin merusak file asli yang telah rapuh tersebut. 

a. Sebagai asisten teknis, tugasmu adalah membuat sebuah sistem file virtual menggunakan FUSE (Filesystem in Userspace) yang dapat membantu sang ilmuwan. Buatlah sebuah direktori mount bernama bebas (misalnya mount_dir) yang merepresentasikan tampilan Baymax dalam bentuk file utuh Baymax.jpeg. File sistem tersebut akan mengambil data dari folder relics sebagai sumber aslinya.

  ![image](https://github.com/user-attachments/assets/ddf9e06f-d0b3-4e40-8abe-9cf86a935a05)

Directory yang saya pakai : 

![image](https://github.com/user-attachments/assets/ccabac7b-662c-486b-9313-d9268f827216)


b. Ketika direktori FUSE diakses, pengguna hanya akan melihat Baymax.jpeg seolah-olah tidak pernah terpecah, meskipun aslinya terdiri dari potongan .000 hingga .013. File Baymax.jpeg tersebut dapat dibaca, ditampilkan, dan disalin sebagaimana file gambar biasa, hasilnya merupakan gabungan sempurna dari keempat belas pecahan tersebut.

- Pertama-tama kita melakukan compile tersebut dengan ```gcc baymax.c `pkg-config fuse3 --cflags --libs` -o baymax```, setelah itu kita run `./baymax mount_dir`, dan kita akan mendapatkan output sebuah gambar :

![image](https://github.com/user-attachments/assets/6b18185e-25fe-4a28-a991-cf3eed97fc34)

Gambar ini merupakan hasil gabungan dari 14 pecahan relics yang dijadikan satu. 


c. Namun sistem ini harus lebih dari sekadar menyatukan. Jika pengguna membuat file baru di dalam direktori FUSE, maka sistem harus secara otomatis memecah file tersebut ke dalam potongan-potongan berukuran maksimal 1 KB, dan menyimpannya di direktori relics menggunakan format [namafile].000, [namafile].001, dan seterusnya. 

- Pertama kita membuat sebuah file yang disimpan di dalam mount_dir, contohnya : `yes "aku suka sisop" | head -c 3000 > mount_dir/kamu.txt`, setelah itu file akan muncul pada directory mount_dir dan otomatis akan memecah menjadi beberapa bagian sesuai dengan ukuran dari file tersebut. Untuk contoh ini berukuran 3 kb, dan akan memecah menjadi 3 bagian pada relics.

![image](https://github.com/user-attachments/assets/97548fa5-a9ba-4461-97da-92fe3a5a03ef)


d. Ketika file tersebut dihapus dari direktori mount, semua pecahannya di relics juga harus ikut dihapus.

- Untuk menghapus file tersebut, kita bisa menggunakan `rm mount_dir/Baymax.jpeg` sebagai contohnya. Outputnya kita bisa lihat pada log :

![image](https://github.com/user-attachments/assets/c7d998e6-51b4-4641-86a3-0d63cabd29ec)

e. Untuk keperluan analisis ilmuwan, sistem juga harus mencatat seluruh aktivitas pengguna dalam sebuah file log bernama activity.log yang disimpan di direktori yang sama. Aktivitas yang dicatat antara lain:

- Membaca file (misalnya membuka baymax.png)
- Membuat file baru (termasuk nama file dan jumlah pecahan)
- Menghapus file (termasuk semua pecahannya yang terhapus)
- Menyalin file (misalnya cp baymax.png /tmp/)
Contoh Log : 

[2025-05-11 10:24:01] READ: Baymax.jpeg

[2025-05-11 10:25:14] WRITE: hero.txt -> hero.txt.000, hero.txt.001

[2025-05-11 10:26:03] DELETE: Baymax.jpeg.000 - Baymax.jpeg.013

[2025-05-11 10:27:45] COPY: Baymax.jpeg -> /tmp/Baymax.jpeg

![image](https://github.com/user-attachments/assets/98cd7e17-e4c2-4985-9201-a387b399e219)

Untuk melakukan copy file ke tmp, bisa menggunakan `cp mount_dir/kamu.txt /tmp/`. 

Kendala dalam pengerjaan : tidak ada. 


## Soal 3
> Soal ini terdapat revisi pada bagian reverse dan enkripsi

Author: Naruna Vicranthyo Putra Gangga - 5027241105

### Deskripsi
Implementasi sistem file virtual berbasis FUSE bernama `antink`, yang dimount di `/mnt/antink` dan menampilkan isi dari direktori host (`/it24_host`) dengan beberapa perlakuan khusus pada file tertentu.

---

## A - Penamaan File (Reverse)
### Ketentuan:
- Semua file yang ditampilkan oleh sistem file harus dibalik **nama dan ekstensinya**, **masing-masing** secara terpisah.
- Contoh: `kimcun.txt` → `txt.nucmik`

### Implementasi:
- Fungsi `reverse_parts()` memisahkan nama dan ekstensi, membalik masing-masing, lalu menggabungkan ulang.
- Hasil `readdir` ditampilkan menggunakan hasil dari `reverse_parts()`.

### Contoh Output:
```bash
$ docker exec -it antink-server ls /mnt/antink
test.txt  vsc.nucmik
```

---

## B - File Berbahaya Tetap Muncul
### Ketentuan:
- File yang mengandung kata `kimcun` atau `nafis` **tetap ditampilkan** dalam direktori (dengan nama dibalik), tetapi tidak dapat diakses.

### Implementasi:
- `readdir`: tetap menampilkan semua file.
- `getattr` dan `read`: melakukan pengecekan dengan `is_dangerous()`, jika berbahaya maka dikembalikan error `-ENOENT` / `-EACCES`.

---

## C - ROT13 untuk Isi File Normal
### Ketentuan:
- Isi file selain `kimcun` dan `nafis` harus dienkripsi menggunakan ROT13 saat dibaca.

### Implementasi:
- Fungsi `rot13()` diterapkan pada buffer hasil pembacaan file di `read`.

### Contoh Output:
```bash
$ docker exec -it antink-server cat /mnt/antink/txt.tset
vav svyr abezny
```
Isi file asli `test.txt`: `ini file normal`

---

## D - Logging Aktivitas
### Ketentuan:
- Semua akses `read` dan deteksi file berbahaya harus dicatat ke log `/var/log/it24.log` dalam format `[HH:MM:SS] TYPE: path`.

### Implementasi:
- Fungsi `log_activity(type, path)` menulis log ke `/var/log/it24.log`.
- Logging dilakukan di:
  - `getattr` → untuk file berbahaya: `WARNING`
  - `read` → untuk file normal: `READ`

### Contoh Output Log:
```bash
$ docker exec -it antink-logger tail /var/log/it24.log
[14:00:01] WARNING: /vsc.nucmik
[14:00:14] READ: /txt.tset
```

---

## Struktur Direktori

![image](https://github.com/user-attachments/assets/de3d4976-ffe1-4d87-b225-b4c5264aa1b8)

---

## Dockerfile
```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    gcc \
    fuse3 \
    libfuse3-dev \
    pkg-config \
    make \
    vim \
    && apt-get clean

RUN mkdir -p /mnt/antink && chmod 777 /mnt/antink

COPY antink.c /antink.c

RUN gcc -Wall -D_FILE_OFFSET_BITS=64 /antink.c $(pkg-config fuse3 --cflags --libs) -o /antink_fs

CMD ["/antink_fs", "-f", "/mnt/antink"]
```

---

## docker-compose.yml
```yaml
version: "3.8"

services:
  antink-server:
    build:
      context: .
    cap_add:
      - SYS_ADMIN
    devices:
      - /dev/fuse
    security_opt:
      - apparmor:unconfined
    volumes:
      - ./it24_host:/home/antink
      - antink_mount:/mnt/antink
      - ./antink-logs:/var/log

  antink-logger:
    image: ubuntu:22.04
    volumes:
      - ./antink-logs:/var/log
    command: tail -f /var/log/it24.log

volumes:
  antink_mount:
```

---

## antink.c
```C
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
```

---

### Sebelum revisi

![Screenshot 2025-05-22 223156](https://github.com/user-attachments/assets/4f9e03af-e4b1-453c-880e-79e425aec78a)

### Setelah revisi

![image](https://github.com/user-attachments/assets/9c12075e-ce97-401d-a52c-241e531a8eb0)

### Kendala: pada bagian me-reverse nama file beserta ekstensinya dan pada bagian enkripsi file
---
