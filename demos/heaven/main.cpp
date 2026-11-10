#include "heaven_window.h"

int main(int argc, char **argv)
{
    demo::DemoConfig config;
    config.title = "Paranoid Frog - Heaven";
    config.audioFile = "assets/heaven.mp3";
    config.songFile = "assets/heaven.xrns"; // tempo and note events
    config.timelineFile = "assets/heaven_timeline.json"; // acts and scene parameters

    return demo::runDemo<heaven::HeavenWindow>(config, argc, argv);
}
