#include "views/RecordingView.h"

#include "simulation/Properties.h"
#include "simulation/display/SimulationDisplay2D.h"
#include "simulation/display/SimulationDisplay3D.h"
#include "util/GuiHelper.h"

#include "imgui.h"
#include "renderer/Renderer3D.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {

static const ImVec4 kACC   {0.31f, 0.56f, 1.00f, 1.0f};
static const ImVec4 kDIM   {0.44f, 0.46f, 0.54f, 1.0f};
static const ImVec4 kRED   {0.92f, 0.36f, 0.36f, 1.0f};
static const ImVec4 kOK    {0.22f, 0.78f, 0.50f, 1.0f};
static const ImVec4 kBORD  {0.20f, 0.21f, 0.27f, 1.0f};
static const ImVec4 kFRAME {0.15f, 0.15f, 0.20f, 1.0f};

// Button ramps
static const ImVec4 kBTN_REC   {0.50f, 0.12f, 0.12f, 1.0f}; // dark red idle
static const ImVec4 kBTN_REC_H {0.70f, 0.18f, 0.18f, 1.0f};
static const ImVec4 kBTN_REC_A {0.88f, 0.28f, 0.28f, 1.0f};
static const ImVec4 kBTN_STP   {0.14f, 0.22f, 0.48f, 1.0f}; // blue
static const ImVec4 kBTN_STP_H {0.20f, 0.34f, 0.68f, 1.0f};
static const ImVec4 kBTN_STP_A {0.28f, 0.48f, 0.88f, 1.0f};

static ImU32 U32(ImVec4 v)         { return ImGui::ColorConvertFloat4ToU32(v); }
static ImU32 U32A(ImVec4 v, float a) {
    return ImGui::ColorConvertFloat4ToU32({v.x, v.y, v.z, a});
}

static void checkFFmpeg(int r, const char* op) {
    if (r >= 0) return;
    char err[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(r, err, sizeof(err));
    throw std::runtime_error(std::string(op) + ": " + err);
}

}


std::string RecordingView::fmtDuration(double s) {
    int h  = static_cast<int>(s) / 3600;
    int m  = (static_cast<int>(s) % 3600) / 60;
    int se = static_cast<int>(s) % 60;
    int cs = static_cast<int>(std::fmod(s, 1.0) * 10); // tenths
    char buf[32];
    if (h > 0)
        std::snprintf(buf, sizeof(buf), "%d:%02d:%02d.%d", h, m, se, cs);
    else
        std::snprintf(buf, sizeof(buf), "%02d:%02d.%d", m, se, cs);
    return buf;
}

std::string RecordingView::fmtBytes(double b) {
    char buf[32];
    if      (b >= 1e9) std::snprintf(buf, sizeof(buf), "%.2f GB", b / 1e9);
    else if (b >= 1e6) std::snprintf(buf, sizeof(buf), "%.1f MB", b / 1e6);
    else if (b >= 1e3) std::snprintf(buf, sizeof(buf), "%.0f KB", b / 1e3);
    else               std::snprintf(buf, sizeof(buf), "%.0f B",  b);
    return buf;
}

void RecordingView::drawStatRow(const char* label, const char* value) {
    ImGui::PushStyleColor(ImGuiCol_Text, kDIM);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::SameLine(130.0f);
    ImGui::TextUnformatted(value);
}

void RecordingView::renderStats() {
    char buf[64];

    double  dur    = recording ? wallDuration        : lastFinishedDuration;
    double  simT   = recording ? simulationTime      : lastFinishedSimTime;
    int64_t frames = recording ? outputFrameCount    : lastFinishedFrames;
    double  bytes  = recording ? estimatedBytes()
                               : (lastFinishedFrames / 60.0) * kBitrate / 8.0;

    ImGui::Spacing();

    if (width > 0 && height > 0) {
        std::snprintf(buf, sizeof(buf), "%d × %d", width, height);
        drawStatRow("Resolution", buf);
    } else {
        drawStatRow("Resolution", "—");
    }

    drawStatRow("Duration",       fmtDuration(dur).c_str());

    std::snprintf(buf, sizeof(buf), "%.3f s", simT);
    drawStatRow("Sim Time",       buf);

    std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(frames));
    drawStatRow("Output Frames",  buf);

    std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(frameCount));
    drawStatRow("Sim Frames",     buf);

    drawStatRow("Est. Size",      fmtBytes(bytes).c_str());

    drawStatRow("Codec",          "H.264 / YUV420P");
    drawStatRow("Bitrate",        "8.0 Mbps");
    drawStatRow("Output FPS",     "60");
}


void RecordingView::render(void* renderData) {
    if (!_visible || !renderData) return;
    GLuint textureId = 0;

    if (PROPERTY(SIMULATION_TYPE, Properties::SimulatorType) <= Properties::SimulatorType::END_2D_SIMULATIONS) {
        const auto* d = static_cast<SimulationDisplay2D::RenderData2D*>(renderData);
        if (d->dim != 2) return;
        textureId = d->textureId;
    }
    else {
        const auto* d = static_cast<SimulationDisplay3D::RenderData3D*>(renderData);
        if (d->dim != 3 || !d->renderer) return;
        textureId = d->renderer->getTextureId();
    }
    if (!textureId) return;

    if (recording)
        wallDuration = std::chrono::duration<double>(
            Clock::now() - recordingStart).count();

    ImGui::SetNextWindowSizeConstraints({260, 0}, {400, FLT_MAX});
    ImGui::Begin("Recording");

    Util::SectionHeader("RECORDING");

    // Status badge
    {
        ImDrawList* dl  = ImGui::GetWindowDrawList();
        ImVec2      pos = ImGui::GetCursorScreenPos();
        float       lh  = ImGui::GetTextLineHeight();

        // Pulsing dot: sine-wave alpha when recording, dim when idle
        float  t     = static_cast<float>(ImGui::GetTime());
        float  pulse = recording
                     ? 0.55f + 0.45f * std::sin(t * 5.0f)
                     : 1.0f;
        ImVec4 dotC  = recording
                     ? ImVec4{kRED.x, kRED.y, kRED.z, pulse}
                     : ImVec4{0.30f, 0.30f, 0.38f, 1.0f};

        dl->AddCircleFilled({pos.x + 8.0f, pos.y + lh * 0.5f},
                             5.0f, U32(dotC));

        ImGui::SetCursorScreenPos({pos.x + 20.0f, pos.y});
        if (recording) {
            ImGui::PushStyleColor(ImGuiCol_Text, kRED);
            ImGui::Text("REC  %s", fmtDuration(wallDuration).c_str());
            ImGui::PopStyleColor();
        } else if (lastFinishedDuration > 0.0) {
            ImGui::PushStyleColor(ImGuiCol_Text, kDIM);
            ImGui::Text("IDLE  last: %s", fmtDuration(lastFinishedDuration).c_str());
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, kDIM);
            ImGui::TextUnformatted("IDLE");
            ImGui::PopStyleColor();
        }
    }

    ImGui::Spacing();

    // File path
    {
        ImGui::PushStyleColor(ImGuiCol_Text, kDIM);
        ImGui::TextUnformatted("Output File");
        ImGui::PopStyleColor();
        ImGui::SameLine(130.0f);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, kFRAME);
        ImGui::SetNextItemWidth(-FLT_MIN);
        char pathBuf[256];
        std::snprintf(pathBuf, sizeof(pathBuf), "%s", outputFile.c_str());
        if (ImGui::InputText("##outfile", pathBuf, sizeof(pathBuf),
                             recording ? ImGuiInputTextFlags_ReadOnly : 0))
            outputFile = pathBuf;
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();

    // Start/Stop button
    {
        const float btnW = ImGui::GetContentRegionAvail().x;

        if (!recording) {
            ImGui::PushStyleColor(ImGuiCol_Button,        kBTN_REC);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kBTN_REC_H);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  kBTN_REC_A);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {8.0f, 7.0f});
            if (ImGui::Button("Start Recording", {btnW, 0}))
                startRecording();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        kBTN_STP);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kBTN_STP_H);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  kBTN_STP_A);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {8.0f, 7.0f});
            if (ImGui::Button("Stop Recording", {btnW, 0}))
                stopRecording();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
        }
    }

    // Stats
    ImGui::Spacing();
    ImGui::Separator();
    renderStats();

    ImGui::End();

    deltaTime = PROPERTY_F(DT);
    if (!recording || deltaTime <= 0.0)
        return;

    GLint prevTex = 0, prevPack = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glGetIntegerv(GL_PACK_ALIGNMENT,     &prevPack);

    glBindTexture(GL_TEXTURE_2D, textureId);

    GLint texW = 0, texH = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &texW);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);

    if (texW <= 0 || texH <= 0) {
        glBindTexture(GL_TEXTURE_2D, prevTex);
        return;
    }

    try {
        if (!encoderInitialized)
            initializeEncoder(texW, texH);

        if (texW != width || texH != height) {
            glBindTexture(GL_TEXTURE_2D, prevTex);
            return;
        }

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glPixelStorei(GL_PACK_ALIGNMENT, prevPack);
        glBindTexture(GL_TEXTURE_2D, prevTex);

        simulationTime += deltaTime;
        ++frameCount;

        constexpr double kOutputInterval = 1.0 / 60.0;
        while (nextOutputTime < simulationTime) {
            writeFrame();
            nextOutputTime += kOutputInterval;
        }
    }
    catch (...) {
        glPixelStorei(GL_PACK_ALIGNMENT, prevPack);
        glBindTexture(GL_TEXTURE_2D, prevTex);
        throw;
    }
}

void RecordingView::startRecording() {
    if (recording) stopRecording();
    SET_PROPERTY_B(PAUSED, false);

    releaseEncoder();

    width = height = 0;
    frameCount = 0;
    outputFrameCount = 0;
    simulationTime = nextOutputTime = wallDuration = 0.0;
    pixels.clear();

    recordingStart = Clock::now();
    recording = true;
}

void RecordingView::stopRecording() {
    if (!recording) return;

    lastFinishedDuration = wallDuration;
    lastFinishedFrames   = outputFrameCount;
    lastFinishedSimTime  = simulationTime;

    recording = false;

    try {
        if (encoderInitialized) {
            flushEncoder();
            checkFFmpeg(av_write_trailer(formatContext), "Write MP4 trailer");
        }
        releaseEncoder();
    }
    catch (...) {
        releaseEncoder();
        throw;
    }
}

static int evenUp(int v) { return (v + 1) & ~1; }

void RecordingView::initializeEncoder(int w, int h) {
    width  = evenUp(w);
    height = evenUp(h);

    checkFFmpeg(
        avformat_alloc_output_context2(
            &formatContext, nullptr, "mp4", outputFile.c_str()),
        "Create output context");
    if (!formatContext)
        throw std::runtime_error("Could not create MP4 output context");

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) throw std::runtime_error("H.264 encoder is unavailable");

    videoStream = avformat_new_stream(formatContext, nullptr);
    if (!videoStream) throw std::runtime_error("Could not create video stream");

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) throw std::runtime_error("Could not allocate codec context");

    codecContext->codec_id   = AV_CODEC_ID_H264;
    codecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    codecContext->width      = width;
    codecContext->height     = height;
    codecContext->pix_fmt    = AV_PIX_FMT_YUV420P;
    codecContext->time_base  = AVRational{1, 60};
    codecContext->framerate  = AVRational{60, 1};
    codecContext->bit_rate   = static_cast<int64_t>(kBitrate);
    codecContext->gop_size   = 60;
    codecContext->max_b_frames = 0;

    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    av_opt_set(codecContext->priv_data, "preset", "medium", 0);
    av_opt_set(codecContext->priv_data, "crf",    "18",     0);

    checkFFmpeg(avcodec_open2(codecContext, codec, nullptr), "Open H.264 encoder");
    checkFFmpeg(avcodec_parameters_from_context(videoStream->codecpar, codecContext),
                "Copy codec parameters");

    videoStream->time_base = codecContext->time_base;

    if (!(formatContext->oformat->flags & AVFMT_NOFILE))
        checkFFmpeg(avio_open(&formatContext->pb, outputFile.c_str(), AVIO_FLAG_WRITE),
                    "Open output file");

    checkFFmpeg(avformat_write_header(formatContext, nullptr), "Write MP4 header");

    frame  = av_frame_alloc();
    packet = av_packet_alloc();
    if (!frame || !packet) throw std::runtime_error("Could not allocate frame/packet");

    frame->format = codecContext->pix_fmt;
    frame->width  = width;
    frame->height = height;
    checkFFmpeg(av_frame_get_buffer(frame, 32), "Allocate frame buffer");

    swsContext = sws_getContext(
        width, height, AV_PIX_FMT_RGBA,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext) throw std::runtime_error("Could not create pixel converter");

    pixels.resize(static_cast<std::size_t>(width) * height * 4);
    encoderInitialized = true;
}

void RecordingView::writeFrame() {
    checkFFmpeg(av_frame_make_writable(frame), "Make frame writable");

    const int stride = width * 4;
    const std::uint8_t* src[]  = { pixels.data() + (std::size_t)(height-1) * stride };
    const int           srcSt[] = { -stride };
    sws_scale(swsContext, src, srcSt, 0, height, frame->data, frame->linesize);

    frame->pts = outputFrameCount++;
    checkFFmpeg(avcodec_send_frame(codecContext, frame), "Send frame");

    while (true) {
        int r = avcodec_receive_packet(codecContext, packet);
        if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) break;
        checkFFmpeg(r, "Receive packet");

        av_packet_rescale_ts(packet, codecContext->time_base, videoStream->time_base);
        packet->stream_index = videoStream->index;
        int wr = av_interleaved_write_frame(formatContext, packet);
        av_packet_unref(packet);
        checkFFmpeg(wr, "Write packet");
    }
}

void RecordingView::flushEncoder() {
    checkFFmpeg(avcodec_send_frame(codecContext, nullptr), "Flush encoder");

    while (true) {
        int r = avcodec_receive_packet(codecContext, packet);
        if (r == AVERROR_EOF || r == AVERROR(EAGAIN)) break;
        checkFFmpeg(r, "Receive flushed packet");

        av_packet_rescale_ts(packet, codecContext->time_base, videoStream->time_base);
        packet->stream_index = videoStream->index;
        int wr = av_interleaved_write_frame(formatContext, packet);
        av_packet_unref(packet);
        checkFFmpeg(wr, "Write flushed packet");
    }
}

void RecordingView::releaseEncoder() {
    sws_freeContext(swsContext);  swsContext  = nullptr;
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codecContext);

    if (formatContext) {
        if (!(formatContext->oformat->flags & AVFMT_NOFILE))
            avio_closep(&formatContext->pb);
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }

    videoStream        = nullptr;
    encoderInitialized = false;
}