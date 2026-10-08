/**
 * @file audioengine_openal.cpp
 * @brief implementation of audio engine using OpenAL
 * support as a OpenAL 3D implementation
 *
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */


#ifndef LL_AUDIOENGINE_OPENAL_H
#define LL_AUDIOENGINE_OPENAL_H

#include "llaudioengine.h"
#include "lllistener_openal.h"
#include "llwindgen.h"
#include "llatomic.h"

#include <atomic>
#include <vector>

class LLStreamedAudioSourceOpenAL;

class LLAudioEngine_OpenAL : public LLAudioEngine
{
    public:
        LLAudioEngine_OpenAL();
        ~LLAudioEngine_OpenAL() override;

        bool init(void *user_data, const std::string &app_title) override;
        std::string getDriverName(bool verbose) override;
        LLStreamingAudioInterface* createDefaultStreamingAudioImpl() const override { return nullptr; }
        void allocateListener() override;

        void shutdown() override;
        void idle() override;

        void setInternalGain(F32 gain) override;

        LLAudioBuffer* createBuffer() override;
        LLAudioChannel* createChannel() override;

        bool initWind() override;
        void cleanupWind() override;
        void updateWind(LLVector3 direction, F32 camera_altitude) override;

        std::shared_ptr<LLStreamedAudioSource> createStreamedSource(U32 prebuffer_ms, bool spatial) override;

    private:
        // Gives every live streamed source its per-frame updateAL() -- see idle().
        void updateStreamedSources();

        // AL_SOFT_callback_buffer -- what LLStreamedAudioSourceOpenAL is built on. Without
        // it, createStreamedSource() returns null.
        bool mHasCallbackBufferExt;
        LPALBUFFERCALLBACKSOFT mBufferCallbackSOFT;

        // Every streamed source handed out, so shutdown() can release their AL objects
        // before the context goes away even if a caller is still holding one.
        std::vector<std::weak_ptr<LLStreamedAudioSourceOpenAL>> mStreamedSources;

        typedef F32 WIND_SAMPLE_T;
        LLWindGen<WIND_SAMPLE_T> *mWindGen;
        F32 *mWindBuf;
        U32 mWindBufFreq;
        U32 mWindBufSamples;
        U32 mWindBufBytes;
        ALuint mWindSource;
        int mNumEmptyWindALBuffers;

        static const int MAX_NUM_WIND_BUFFERS = 80;
        static const float WIND_BUFFER_SIZE_SEC; // 1/20th sec

        ALCdevice* mALDevice;

        bool mHasReopenExt;
        LPALCREOPENDEVICESOFT mReopenDeviceSOFT;

        bool mHasSystemEventsExt;
        LPALCEVENTCONTROLSOFT mEventControlSOFT;
        LPALCEVENTCALLBACKSOFT mEventCallbackSOFT;

        // Set by onDeviceEventSOFT(), consumed by idle()
        LLAtomicBool mDefaultDeviceChanged;

        static void ALC_APIENTRY onDeviceEventSOFT(ALCenum eventType, ALCenum deviceType,
                                                     ALCdevice* device, ALCsizei length,
                                                     const ALCchar* message, void* userParam) noexcept;
        void reopenOnDefaultDevice();
};

class LLAudioChannelOpenAL : public LLAudioChannel
{
    public:
        LLAudioChannelOpenAL();
        virtual ~LLAudioChannelOpenAL();
    protected:
        /*virtual*/ void play();
        /*virtual*/ void playSynced(LLAudioChannel *channelp);
        /*virtual*/ void cleanup();
        /*virtual*/ bool isPlaying();

        /*virtual*/ bool updateBuffer();
        /*virtual*/ void update3DPosition();
        /*virtual*/ void updateLoop();

        ALuint mALSource;
            ALint mLastSamplePos;
};

class LLAudioBufferOpenAL : public LLAudioBuffer{
    public:
        LLAudioBufferOpenAL();
        virtual ~LLAudioBufferOpenAL();

        bool loadWAV(const std::string& filename);
        U32 getLength();

        friend class LLAudioChannelOpenAL;
    protected:
        void cleanup();
        ALuint getBuffer() {return mALBuffer;}

        ALuint mALBuffer;
};

// An LLStreamedAudioSource on an AL_SOFT_callback_buffer buffer: OpenAL's own mixer thread
// pulls samples straight out of a lock-free single-producer/single-consumer ring that
// pushPCM() fills, so playback never depends on the main thread's frame rate. float32:
// stereo for a non-spatial source, mono for a spatial one (OpenAL only positions mono
// sources, so pushPCM() mixes down on the way in). Plays at whatever sample rate is being
// pushed -- OpenAL resamples to the device -- re-specifying its buffer on the main thread
// (see updateAL()) whenever that changes.
class LLStreamedAudioSourceOpenAL : public LLStreamedAudioSource
{
    public:
        LLStreamedAudioSourceOpenAL(U32 prebuffer_ms, bool spatial);
        ~LLStreamedAudioSourceOpenAL();

        // Main thread. Creates the AL source/buffer and starts it playing (silence until
        // prebuffered). False if any AL call failed, with nothing left allocated.
        bool initAL(LPALBUFFERCALLBACKSOFT buffer_callback_fn);
        // Main thread, once per frame (LLAudioEngine_OpenAL::idle()). Re-specifies the
        // callback buffer if pushPCM() has started seeing a different sample rate.
        void updateAL();
        // Main thread. Stops and deletes the AL objects -- idempotent, and safe to call
        // after the engine has already done so at shutdown.
        void releaseAL();

        void pushPCM(const F32* interleaved, U32 frames, U32 sample_rate, U32 channels) override;
        void streamStopped() override;
        void setGain(F32 gain) override;
        void setPositionGlobal(const LLVector3d& pos_global) override;
        void clearPosition() override;
        Stats getStats() const override;

    private:
        static ALsizei AL_APIENTRY bufferCallback(ALvoid* userptr, ALvoid* sampledata, ALsizei numbytes) noexcept;
        // OpenAL mixer thread only: must not block, allocate or log.
        void fill(F32* out, U32 frames) noexcept;
        // Main thread. (Re)binds mALBuffer as a callback buffer at sample_rate.
        void specifyBuffer(U32 sample_rate);

        // ~680ms at 48kHz -- comfortably above MAX_LATENCY_MS at any common rate, so the
        // writer only ever finds it full if the mixer has stopped pulling altogether.
        static constexpr U32 RING_FRAMES = 32768; // power of two
        static constexpr U32 MAX_LATENCY_MS = 200;
        // Until the first pushPCM() says otherwise.
        static constexpr U32 DEFAULT_SAMPLE_RATE = 48000;
        static constexpr U32 MIN_SAMPLE_RATE = 8000;
        static constexpr U32 MAX_SAMPLE_RATE = 192000;

        const bool mSpatial;
        const U32 mChannels; // 1 when mSpatial, else 2
        std::vector<F32> mRing; // RING_FRAMES * mChannels, interleaved
        // Monotonic frame counts; position in mRing is (pos & (RING_FRAMES - 1)). mWritePos
        // is advanced only by pushPCM(), mReadPos only by fill().
        std::atomic<U64> mWritePos{0};
        std::atomic<U64> mReadPos{0};
        std::atomic<bool> mStreamStopped{false};

        // Thresholds are kept in time, not frames, since the frame rate can change.
        const U32 mPrebufferMs;
        // The rate pushPCM() is currently being fed at (0 until the first push) -- written
        // by the feeder, acted on by updateAL().
        std::atomic<U32> mStreamRate{0};
        // The rate mALBuffer is actually specified at -- written by the main thread, read
        // by fill()/getStats().
        std::atomic<U32> mPlayRate{DEFAULT_SAMPLE_RATE};
        bool mPlaying = false; // fill() only: false while (re)prebuffering

        std::atomic<U64> mUnderruns{0};
        std::atomic<U64> mOverruns{0};
        std::atomic<U64> mSkippedFrames{0};
        std::atomic<U64> mFormatDrops{0};

        ALuint mALSource = AL_NONE;
        ALuint mALBuffer = AL_NONE;
        LPALBUFFERCALLBACKSOFT mBufferCallbackFn = nullptr;
        // Main thread only: whether the AL source is currently listener-relative, so
        // setPositionGlobal()/clearPosition() only flip AL_SOURCE_RELATIVE on a change.
        bool mHeadRelative = true;
};

#endif
