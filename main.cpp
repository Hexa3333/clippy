#include <iostream>
#include <cstring>
#include <getopt.h>
#include <string>
#include <unistd.h>
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

static int G_concatenated = false;
sighandler_t concatenate(int sig) {
    std::cout << "SIGPOLL received. Clip will be created...\n";
    kill(G_ffmpeg, SIGINT);

    int wstatus;
    waitpid(G_ffmpeg, &wstatus, 0);
    std::cout << "[Master] ffmpeg exited with status: " << wstatus << ".\n";
    ++G_ffmpeg_instance_iter;

    // Read instance log fd and gather which clips should be thingied. Concat video.

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
            close(G_ffmpeg_instance_log_fd);
            std::cout << "[Master] Waiting on rec event.\n";

            // While rec event not received, wait
            // TODO: smarter way to wait
            while (!G_concatenated) {}
            G_concatenated = false;
        }
    }
}
