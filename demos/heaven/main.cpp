#include "heaven_window.h"

int main(int argc, char **argv)
{
    ivfmusic::MusicSceneConfig config;
    config.title = "Paranoid Frog - Heaven";
    config.audioFile = "assets/heaven.mp3";
    config.scoreFile = "assets/heaven.xrns"; // tempo and note events
    config.automationFile = "assets/heaven_timeline.json"; // acts and scene parameters

    return ivfmusic::run<heaven::HeavenWindow>(config, argc, argv);
}
