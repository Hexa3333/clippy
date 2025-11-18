#include <iostream>
#include <cstring>
#include <getopt.h>
#include <string>
#include <unistd.h>
#include <wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "device.hpp"

struct Config {
    int framerate;
};

int main(int argc, char** argv) {
    Device device;

#if 0 // TODO
    char c;
    while ((c=getopt(argc, argv, ""))) {

    }
#endif

    Config config;
    config.framerate = 25;
    // Device information is used for setting configs up

    mkdir("/tmp/clippy/", S_IRUSR | S_IWUSR | S_IXUSR);

    int ffmpeg_instance_iter = 0;
    while (1) {
        std::string ffmpeg_instance_log = "/tmp/clippy/log-" + std::to_string(ffmpeg_instance_iter) + ".txt";
        int ffmpeg_instance_log_fd = open(ffmpeg_instance_log.c_str(), O_CREAT | O_TRUNC | O_RDWR, S_IRUSR);

        pid_t ffmpeg = fork();
        if (ffmpeg == 0) {
            std::cout << "[Child] forking ffmpeg instance...\n";

            // ffmpeg stderr to tmpfile
            dup2(ffmpeg_instance_log_fd, STDERR_FILENO);

            // exec ffmpeg
            // ffmpeg -video_size 1920x1080 -f x11grab -i :0.0 -c:v libx264 -f segment -segment_format mp4 -segment_time 5 -reset_timestamps 1 -segment_wrap 10 /tmp/clippy-output-N-%01d.mp4
            std::string output_filename = "/tmp/clippy/output-" + std::to_string(ffmpeg_instance_iter) + "-%01d.mp4";
            execl("/usr/bin/ffmpeg", "ffmpeg", "-y", "-video_size", "1920x1080", "-f", "x11grab", "-i", ":0.0", "-c:v", "libx264", "-f", "segment", "-segment_format", "mp4", "-segment_time", "5", "-reset_timestamps", "1", "-segment_wrap", "10", output_filename.c_str(), NULL);

            // exec fail
            std::cout << "[Child] ffmpeg exec failed: " << strerror(errno) << "\n";
        } else if (ffmpeg == -1) {
            // Error
            std::cout << "[Master] fork failed: " << strerror(errno) << "\n";
            return -1;
        } else {
            close(ffmpeg_instance_log_fd);
            std::cout << "[Master] Waiting on rec event.\n";

            // While rec event not received, wait etc.
            
            // On receive, exec handler that concatenates video

            // Quit and roll back
            int wstatus;
            waitpid(ffmpeg, &wstatus, 0);
            std::cout << "[Master] ffmpeg exited with status: " << wstatus << ".\n";
            ++ffmpeg_instance_iter;
        }
    }
}
