#ifndef EULERIANFLUIDSIMULATION_RECORDINGVIEW_H
#define EULERIANFLUIDSIMULATION_RECORDINGVIEW_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "Viewable.h"

extern "C" {
    struct AVFormatContext;
    struct AVCodecContext;
    struct AVStream;
    struct AVFrame;
    struct AVPacket;
    struct SwsContext;
}

class RecordingView : public Viewable {
public:
    RecordingView() = default;

    void render(void* renderData) override;

    void startRecording();
    void stopRecording();

private:
    bool recording          = false;
    bool encoderInitialized = false;

    int            width             = 0;
    int            height            = 0;
    std::uint64_t  frameCount        = 0;   // simulation frames captured
    std::int64_t   outputFrameCount  = 0;   // encoded frames written

    double simulationTime  = 0.0;
    double deltaTime       = 0.0;
    double nextOutputTime  = 0.0;

    std::string                outputFile = "recording.mp4";
    std::vector<std::uint8_t>  pixels;

    AVFormatContext* formatContext = nullptr;
    AVCodecContext*  codecContext  = nullptr;
    AVStream*        videoStream   = nullptr;
    AVFrame*         frame         = nullptr;
    AVPacket*        packet        = nullptr;
    SwsContext*      swsContext    = nullptr;

    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    TimePoint  recordingStart;          // wall-clock start of current take
    double     wallDuration   = 0.0;    // seconds elapsed (updated each frame)
    double     lastFinishedDuration  = 0.0;  // duration of last completed take
    std::int64_t lastFinishedFrames  = 0;    // output frames of last take
    double     lastFinishedSimTime   = 0.0;

    // Estimated encoded size = outputFrameCount / 60 * bitrate / 8  (bytes)
    static constexpr double kBitrate = 8'000'000.0;
    double estimatedBytes() const {
        return (outputFrameCount / 60.0) * kBitrate / 8.0;
    }

    static void drawStatRow(const char* label, const char* value);
    static std::string fmtDuration(double seconds);
    static std::string fmtBytes(double bytes);

    void renderStats();

    void initializeEncoder(int w, int h);
    void writeFrame();
    void flushEncoder();
    void releaseEncoder();
};


#endif //EULERIANFLUIDSIMULATION_RECORDINGVIEW_H