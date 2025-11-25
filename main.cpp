#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <cstring>
#include <getopt.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "device.hpp"

// HOLY race condition...

static int G_ffmpeg_instance_log_fd = -1;
static int G_ffmpeg_instance_iter = 0;
static pid_t G_ffmpeg;

struct Config {
    int framerate;
};

static int G_CycleCount = 10;

static int G_concatenated = false;
sighandler_t concatenate(int sig) {
    std::cout << "SIGPOLL received. Clip will be created...\n";
    kill(G_ffmpeg, SIGINT);

    int wstatus;
    waitpid(G_ffmpeg, &wstatus, 0);
    std::cout << "[Master] ffmpeg exited with status: " << wstatus << ".\n";
    ++G_ffmpeg_instance_iter;

    // Read instance log fd and gather which clips should be thingied. Concat video.
    FILE* ffmpeg_log = fdopen(G_ffmpeg_instance_log_fd, "r");
    if (!ffmpeg_log) {
        std::cout << "[Master] ffmpeg log fd failed.\n";
        return 0;
    }
    fseek(ffmpeg_log, 0, SEEK_SET);

    // TODO: Optimize. Perhaps by reading from the end.
    char line[4096];
    char last_segment_filename[64];
    while (fgets(line, 4096, ffmpeg_log)) {
        char* last_segment_filename_opening;
        if ((last_segment_filename_opening = strstr(line, "Opening"))) {
            char* start = strchr(last_segment_filename_opening, '\'');
            char* end = strchr(start+1, '\'');
            if (start && end) {
                size_t len = end - start -1;
                memcpy(last_segment_filename, start+1, len);
                last_segment_filename[len] = '\0';
            }
        }
    }

    // Extract the iter and cycle of the last segment
    int iteration, last_cycle;
    if (sscanf(last_segment_filename, "%*[^0-9]%d-%d", &iteration, &last_cycle) != 2) {
        std::cout << "[Master] Error parsing last segment iteration and/or cycle.\n";
        return 0;
    }

    // NOTE: This assumes all cycles are full. They may not necessarily be.
    std::vector<std::string> segment_filenames_to_concatenate;
    segment_filenames_to_concatenate.reserve(G_CycleCount);

    for (int i = 0; i < G_CycleCount; ++i) {
        int cycle_iter = (last_cycle+1+i) % G_CycleCount;
        segment_filenames_to_concatenate.push_back(
                std::string("/tmp/clippy/capture-" + std::to_string(iteration) + "-" + std::to_string(cycle_iter) + ".mp4")
                );
    }

    std::ofstream concat_file("/tmp/clippy/concat.txt", std::ios::trunc);
    for (const auto& s : segment_filenames_to_concatenate) {
        std::string a = "file \'" + s + "\'\n";
        concat_file.write(a.data(), a.size());
    }

    time_t time_now = time(NULL);
    char* time_str = ctime(&time_now);

    concat_file.flush();
    pid_t process_concat = fork();
    if (process_concat == 0) {
        std::string file_path = "/home/hexa/" + std::string(time_str) + ".mp4";
        execl("/usr/bin/ffmpeg", "ffmpeg", "-y", "-f", "concat", "-safe", "0", "-i", "/tmp/clippy/concat.txt", "-c", "copy", file_path.data(), NULL);
        perror("what...");
        std::cerr << "ffmpeg concat spawn failed.\n";
        std::exit(-1);
    } else {
        int wstatus;
        waitpid(process_concat, &wstatus, 0);
        std::cout << "[Master] ffmpeg (concat) exited with status: " << wstatus << ".\n";
    }

    G_concatenated = true;
    return 0;
}

int main(int argc, char** argv) {
    Device device;

#if 0 // TODO
    char c;
    while ((c=getopt(argc, argv, ""))) {

    }
#endif

    Config config;
    config.framerate = 25;
    
    signal(SIGPOLL, (__sighandler_t)concatenate);

    // Device information is used for setting configs up

    mkdir("/tmp/clippy/", S_IRUSR | S_IWUSR | S_IXUSR);
    while (1) {
        std::string ffmpeg_instance_log_filename = "/tmp/clippy/log-" + std::to_string(G_ffmpeg_instance_iter) + ".txt";
        close(G_ffmpeg_instance_log_fd);
        G_ffmpeg_instance_log_fd = open(ffmpeg_instance_log_filename.c_str(), O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);

        G_ffmpeg = fork();
        if (G_ffmpeg == 0) {
            std::cout << "[Child] forking ffmpeg instance...\n";

            // ffmpeg stderr to tmpfile
            dup2(G_ffmpeg_instance_log_fd, STDERR_FILENO);

            // exec ffmpeg
            // ffmpeg -video_size 1920x1080 -f x11grab -i :0.0 -c:v libx264 -f segment -segment_format mp4 -segment_time 5 -reset_timestamps 1 -segment_wrap 10 /tmp/clippy-output-N-%01d.mp4
            std::string ffmpeg_output_filename = "/tmp/clippy/capture-" + std::to_string(G_ffmpeg_instance_iter) + "-%01d.mp4";
            execl("/usr/bin/ffmpeg", "ffmpeg", "-y", "-video_size", "1920x1080", "-f", "x11grab", "-i", ":0.0", "-c:v", "libx264", "-f", "segment", "-segment_format", "mp4", "-segment_time", "5", "-reset_timestamps", "1", "-segment_wrap", "10", ffmpeg_output_filename.c_str(), NULL);

            // exec fail
            std::cout << "[Child] ffmpeg exec failed: " << strerror(errno) << "\n";
        } else if (G_ffmpeg == -1) {
            // Error
            std::cout << "[Master] fork failed: " << strerror(errno) << "\n";
            return -1;
        } else {
            std::cout << "[Master] Waiting on rec event.\n";

            // While rec event not received, wait
            // TODO: smarter way to wait
            while (!G_concatenated) {}
            G_concatenated = false;
        }
    }
}
