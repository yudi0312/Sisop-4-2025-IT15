# Praktikum Sisop Modul 4-2025-IT15

Anggota kelompok : 
- Putu Yudi Nandanjaya Wiraguna	5027241080
- Naruna Vicranthyo Putra Gangga	5027241105
- Az Zahrra Tasya Adelia	        5027241087

---

## Soal 2 - Baymax
Author : Putu Yudi Nandanjaya Wiraguna - 5027241080

### Deskripsi 

Pada suatu hari, seorang ilmuwan muda menemukan sebuah drive tua yang tertanam di reruntuhan laboratorium robotik. Saat diperiksa, drive tersebut berisi pecahan data dari satu-satunya robot perawat legendaris yang dikenal dengan nama Baymax. Sayangnya, akibat kerusakan sistem selama bertahun-tahun, file utuh Baymax telah terfragmentasi menjadi 14 bagian kecil, masing-masing berukuran 1 kilobyte, dan tersimpan dalam direktori bernama relics. Pecahan tersebut diberi nama berurutan seperti Baymax.jpeg.000, Baymax.jpeg.001, hingga Baymax.jpeg.013. Ilmuwan tersebut kini ingin membangkitkan kembali Baymax ke dalam bentuk digital yang utuh, namun ia tidak ingin merusak file asli yang telah rapuh tersebut. 

a. Sebagai asisten teknis, tugasmu adalah membuat sebuah sistem file virtual menggunakan FUSE (Filesystem in Userspace) yang dapat membantu sang ilmuwan. Buatlah sebuah direktori mount bernama bebas (misalnya mount_dir) yang merepresentasikan tampilan Baymax dalam bentuk file utuh Baymax.jpeg. File sistem tersebut akan mengambil data dari folder relics sebagai sumber aslinya.

  ![image](https://github.com/user-attachments/assets/ddf9e06f-d0b3-4e40-8abe-9cf86a935a05)

b. Ketika direktori FUSE diakses, pengguna hanya akan melihat Baymax.jpeg seolah-olah tidak pernah terpecah, meskipun aslinya terdiri dari potongan .000 hingga .013. File Baymax.jpeg tersebut dapat dibaca, ditampilkan, dan disalin sebagaimana file gambar biasa, hasilnya merupakan gabungan sempurna dari keempat belas pecahan tersebut.

c. Namun sistem ini harus lebih dari sekadar menyatukan. Jika pengguna membuat file baru di dalam direktori FUSE, maka sistem harus secara otomatis memecah file tersebut ke dalam potongan-potongan berukuran maksimal 1 KB, dan menyimpannya di direktori relics menggunakan format [namafile].000, [namafile].001, dan seterusnya. 

d. Ketika file tersebut dihapus dari direktori mount, semua pecahannya di relics juga harus ikut dihapus.

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
