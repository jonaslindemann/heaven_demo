#include "music_clock.h"

#include <ivf/audio_manager.h>
#include <ivf/logger.h>

#include <algorithm>
#include <cmath>

using namespace demo;

namespace {

// Larger differences between the smoothed time and the audio position are treated as a jump
constexpr double kSnapThreshold = 0.1;
// How quickly the smoothed time converges to the audio position (per second)
constexpr double kConvergenceRate = 8.0;

ivf::AudioManager *audioManager()
{
    static bool initialized = false;
    auto audio = ivf::AudioManager::instance();
    if (!initialized)
        initialized = audio->init();
    return initialized ? audio : nullptr;
}

} // namespace

bool MusicClock::load(const std::string &path)
{
    m_time = 0.0;
    m_playing = false;
    m_hasAudio = false;

    auto audio = audioManager();
    if (audio && audio->startFileAnalysis(path, false))
    {
        audio->pauseFileAnalysis();
        audio->seekFileAnalysis(0.0);
        m_length = audio->fileAnalysisLength();
        m_hasAudio = true;
        ivf::logInfo("Loaded " + path + " (" + std::to_string(m_length) + " s)", "MusicClock");
    }
    else
    {
        ivf::logWarning("No audio for " + path + (audio ? ": " + audio->lastError() : "") +
                            ", using silent time",
                        "MusicClock");
    }
    return m_hasAudio;
}

void MusicClock::play()
{
    if (m_length > 0.0 && m_time >= m_length)
        seek(0.0);
    m_playing = true;
    if (m_hasAudio && !m_freeRunning)
        ivf::AudioManager::instance()->resumeFileAnalysis();
}

void MusicClock::pause()
{
    m_playing = false;
    if (m_hasAudio)
        ivf::AudioManager::instance()->pauseFileAnalysis();
}

void MusicClock::togglePlay()
{
    m_playing ? pause() : play();
}

void MusicClock::seek(double seconds)
{
    m_time = std::max(seconds, 0.0);
    if (m_length > 0.0)
        m_time = std::min(m_time, m_length);
    if (m_hasAudio)
        ivf::AudioManager::instance()->seekFileAnalysis(m_time);
}

void MusicClock::update(double dt)
{
    if (!m_hasAudio || m_freeRunning)
    {
        if (m_playing)
            m_time += dt;
        if (m_length > 0.0 && m_time >= m_length)
        {
            m_time = m_length;
            m_playing = false;
        }
        return;
    }

    auto audio = ivf::AudioManager::instance();
    const double audioTime = audio->fileAnalysisPosition();

    if (!m_playing)
    {
        m_time = audioTime;
        return;
    }
    if (audio->fileAnalysisAtEnd())
    {
        m_time = m_length;
        m_playing = false;
        return;
    }

    // Advance smoothly and converge to the (coarse) audio position
    const double predicted = m_time + dt;
    const double error = audioTime - predicted;
    if (std::abs(error) > kSnapThreshold)
        m_time = audioTime;
    else
        m_time = predicted + error * std::min(1.0, dt * kConvergenceRate);
}

void MusicClock::setFreeRunning(bool freeRunning)
{
    if (freeRunning == m_freeRunning)
        return;
    m_freeRunning = freeRunning;
    if (!m_hasAudio)
        return;

    auto audio = ivf::AudioManager::instance();
    if (freeRunning)
    {
        audio->pauseFileAnalysis();
    }
    else
    {
        audio->seekFileAnalysis(m_time);
        if (m_playing)
            audio->resumeFileAnalysis();
    }
}

void MusicClock::setVolume(float volume)
{
    ivf::AudioManager::instance()->setFileAnalysisVolume(volume);
}

float MusicClock::volume() const
{
    return ivf::AudioManager::instance()->fileAnalysisVolume();
}

void MusicClock::setTempo(double bpm, double firstBeat, int beatsPerBar)
{
    m_bpm = std::max(bpm, 1.0);
    m_firstBeat = firstBeat;
    m_beatsPerBar = std::max(beatsPerBar, 1);
}

double MusicClock::beatsAt(double seconds) const
{
    return (seconds - m_firstBeat) * m_bpm / 60.0;
}

double MusicClock::timeAtBeat(double beat) const
{
    return m_firstBeat + beat * 60.0 / m_bpm;
}

double MusicClock::timeAtBar(double bar) const
{
    return timeAtBeat(bar * m_beatsPerBar);
}

double MusicClock::beatPhase() const
{
    const double b = beats();
    return b - std::floor(b);
}
