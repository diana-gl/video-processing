#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <jpeglib.h>
#include <unistd.h>
#include <sys/types.h>

#define WIDTH 600
#define HEIGHT 600

void convert_image_to_jpg(const char *input, const char *output);
void resize_image(const char *input, const char *output);
double get_audio_duration(const char *audio_file);
void create_video(const char *audio_file, double image_duration);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <folder_path> <audio_file>\n", argv[0]);
        return 1;
    }

    const char *folder_path = argv[1];
    const char *audio_file = argv[2];
    struct dirent *entry;
    DIR *dp = opendir(folder_path);

    if (dp == NULL) {
        perror("opendir");
        return 1;
    }

    // Creează un folder temporar pentru imaginile procesate
    system("mkdir -p processed_images");

    int image_count = 0;

    // Parcurge fișierele din folder
    while ((entry = readdir(dp))) {
        if (entry->d_type == DT_REG) { // Asigură-te că este un fișier obișnuit
            char input_path[1024];
            snprintf(input_path, sizeof(input_path), "%s/%s", folder_path, entry->d_name);

            // Verifică dacă fișierul este o imagine
            if (strstr(entry->d_name, ".jpg") || strstr(entry->d_name, ".png") || strstr(entry->d_name, ".bmp") || strstr(entry->d_name, ".jpeg")) {
                char output_path[1024];
                snprintf(output_path, sizeof(output_path), "processed_images/%s.jpg", entry->d_name);
                
                convert_image_to_jpg(input_path, output_path);
                resize_image(output_path, output_path);
                image_count++;
            }
        }
    }

    closedir(dp);

    // Obține durata fișierului audio
    double audio_duration = get_audio_duration(audio_file);
    if (audio_duration <= 0) {
        fprintf(stderr, "Failed to get audio duration\n");
        return 1;
    }

    // Calculează durata fiecărei imagini
    double image_duration = audio_duration / image_count;

    // Creează video-ul
    create_video(audio_file, image_duration);

    // Șterge folderul temporar
    system("rm -rf processed_images");

    return 0;
}

void convert_image_to_jpg(const char *input, const char *output) {
    // Functie simpla care copiază imaginea la format JPG
    char command[1024];
    snprintf(command, sizeof(command), "ffmpeg -i %s %s", input, output);
    system(command);
}

void resize_image(const char *input, const char *output) {
    // Functie simpla care redimensioneaza imaginea
    char command[1024];
    snprintf(command, sizeof(command), "ffmpeg -i %s -vf scale=%d:%d %s", input, WIDTH, HEIGHT, output);
    system(command);
}

double get_audio_duration(const char *audio_file) {
    char command[1024];
    snprintf(command, sizeof(command), "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 %s", audio_file);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen");
        return -1;
    }

    char buffer[128];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        pclose(fp);
        return -1;
    }

    pclose(fp);
    return atof(buffer);
}

void create_video(const char *audio_file, double image_duration) {
    char command[2048];
    snprintf(command, sizeof(command), "ffmpeg -y -framerate 1/%f -pattern_type glob -i 'processed_images/*.jpg' -i %s -c:v libx264 -r 30 -pix_fmt yuv420p -shortest output.mp4", image_duration, audio_file);
    system(command);
}
