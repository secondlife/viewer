/**
 * @file llaudiodecodemgr.cpp
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "linden_common.h"

#include "llaudiodecodemgr.h"

#include "llaudioengine.h"
#include "lllfsthread.h"
#include "llfilesystem.h"
#include "llstring.h"
#include "lldir.h"
#include "llendianswizzle.h"
#include "llassetstorage.h"
#include "llrefcount.h"
#include "threadpool.h"
#include "workqueue.h"

#include "llvorbisencode.h"

#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#include <iterator>
#include <deque>

extern LLAudioEngine *gAudiop;

static const S32 WAV_HEADER_SIZE = 44;


//////////////////////////////////////////////////////////////////////////////


class LLVorbisDecodeState : public LLThreadSafeRefCount
{
public:
    class WriteResponder : public LLLFSThread::Responder
    {
    public:
        WriteResponder(LLVorbisDecodeState* decoder) : mDecoder(decoder) {}
        ~WriteResponder() {}
        void completed(S32 bytes)
        {
            mDecoder->ioComplete(bytes);
        }
        LLPointer<LLVorbisDecodeState> mDecoder;
    };

    LLVorbisDecodeState(const LLUUID &uuid, const std::string &out_filename);

    bool initDecode();
    bool decodeSection(); // Return true if done.
    bool finishDecode();

    void flushBadFile();

    void ioComplete(S32 bytes)          { mBytesRead = bytes; }
    bool isValid() const                { return mValid; }
    bool isDone() const                 { return mDone; }
    const LLUUID &getUUID() const       { return mUUID; }

protected:
    virtual ~LLVorbisDecodeState();

    bool mValid;
    bool mDone;
    LLAtomicS32 mBytesRead;
    LLUUID mUUID;

    std::vector<U8> mWAVBuffer;
    std::string mOutFilename;
    LLLFSThread::handle_t mFileHandle;

    LLFileSystem *mInFilep;
    OggVorbis_File mVF;
    S32 mCurrentSection;
};

size_t cache_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    LLFileSystem *file = (LLFileSystem *)datasource;

    if (file->read((U8*)ptr, (S32)(size * nmemb)))  /*Flawfinder: ignore*/
    {
        S32 read = file->getLastBytesRead();
        return  read / size;    /*Flawfinder: ignore*/
    }
    else
    {
        return 0;
    }
}

S32 cache_seek(void *datasource, ogg_int64_t offset, S32 whence)
{
    LLFileSystem *file = (LLFileSystem *)datasource;

    // cache has 31-bit files
    if (offset > S32_MAX)
    {
        return -1;
    }

    S32 origin;
    switch (whence) {
    case SEEK_SET:
        origin = 0;
        break;
    case SEEK_END:
        origin = file->getSize();
        break;
    case SEEK_CUR:
        origin = -1;
        break;
    default:
        LL_ERRS("AudioEngine") << "Invalid whence argument to cache_seek" << LL_ENDL;
        return -1;
    }

    if (file->seek((S32)offset, origin))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

S32 cache_close (void *datasource)
{
    LLFileSystem *file = (LLFileSystem *)datasource;
    delete file;
    return 0;
}

long cache_tell (void *datasource)
{
    LLFileSystem *file = (LLFileSystem *)datasource;
    return file->tell();
}

LLVorbisDecodeState::LLVorbisDecodeState(const LLUUID &uuid, const std::string &out_filename)
{
    mDone = false;
    mValid = false;
    mBytesRead = -1;
    mUUID = uuid;
    mInFilep = NULL;
    mCurrentSection = 0;
    mOutFilename = out_filename;
    mFileHandle = LLLFSThread::nullHandle();

    // No default value for mVF, it's an ogg structure?
    // Hey, let's zero it anyway, for predictability.
    memset(&mVF, 0, sizeof(mVF));
}

LLVorbisDecodeState::~LLVorbisDecodeState()
{
    if (!mDone)
    {
        delete mInFilep;
        mInFilep = NULL;
    }
}


bool LLVorbisDecodeState::initDecode()
{
    ov_callbacks cache_callbacks;
    cache_callbacks.read_func = cache_read;
    cache_callbacks.seek_func = cache_seek;
    cache_callbacks.close_func = cache_close;
    cache_callbacks.tell_func = cache_tell;

    LL_DEBUGS("AudioEngine") << "Initing decode from vfile: " << mUUID << LL_ENDL;

    mInFilep = new LLFileSystem(mUUID, LLAssetType::AT_SOUND);
    if (!mInFilep || !mInFilep->getSize())
    {
        LL_WARNS("AudioEngine") << "unable to open vorbis source vfile for reading" << LL_ENDL;
        delete mInFilep;
        mInFilep = NULL;
        return false;
    }

    S32 r = ov_open_callbacks(mInFilep, &mVF, NULL, 0, cache_callbacks);
    if(r < 0)
    {
        LL_WARNS("AudioEngine") << r << " Input to vorbis decode does not appear to be an Ogg bitstream: " << mUUID << LL_ENDL;
        return(false);
    }

    S32 sample_count = (S32)ov_pcm_total(&mVF, -1);
    size_t size_guess = (size_t)sample_count;
    vorbis_info* vi = ov_info(&mVF, -1);
    size_guess *= (vi? vi->channels : 1);
    size_guess *= 2;
    size_guess += 2048;

    bool abort_decode = false;

    if (vi)
    {
        if( vi->channels < 1 || vi->channels > LLVORBIS_CLIP_MAX_CHANNELS )
        {
            abort_decode = true;
            LL_WARNS("AudioEngine") << "Bad channel count: " << vi->channels << LL_ENDL;
        }
    }
    else // !vi
    {
        abort_decode = true;
        LL_WARNS("AudioEngine") << "No default bitstream found" << LL_ENDL;
    }

    if( (size_t)sample_count > LLVORBIS_CLIP_REJECT_SAMPLES ||
        (size_t)sample_count <= 0)
    {
        abort_decode = true;
        LL_WARNS("AudioEngine") << "Illegal sample count: " << sample_count << LL_ENDL;
    }

    if( size_guess > LLVORBIS_CLIP_REJECT_SIZE )
    {
        abort_decode = true;
        LL_WARNS("AudioEngine") << "Illegal sample size: " << size_guess << LL_ENDL;
    }

    if( abort_decode )
    {
        LL_WARNS("AudioEngine") << "Canceling initDecode. Bad asset: " << mUUID << LL_ENDL;
        vorbis_comment* comment = ov_comment(&mVF,-1);
        if (comment && comment->vendor)
        {
            LL_WARNS("AudioEngine") << "Bad asset encoded by: " << comment->vendor << LL_ENDL;
        }
        delete mInFilep;
        mInFilep = NULL;
        return false;
    }

    try
    {
        mWAVBuffer.reserve(size_guess);
        mWAVBuffer.resize(WAV_HEADER_SIZE);
    }
    catch (std::bad_alloc&)
    {
        LL_WARNS("AudioEngine") << "Out of memory when trying to alloc buffer: " << size_guess << LL_ENDL;
        delete mInFilep;
        mInFilep = NULL;
        return false;
    }

    {
        // write the .wav format header
        //"RIFF"
        mWAVBuffer[0] = 0x52;
        mWAVBuffer[1] = 0x49;
        mWAVBuffer[2] = 0x46;
        mWAVBuffer[3] = 0x46;

        // length = datalen + 36 (to be filled in later)
        mWAVBuffer[4] = 0x00;
        mWAVBuffer[5] = 0x00;
        mWAVBuffer[6] = 0x00;
        mWAVBuffer[7] = 0x00;

        //"WAVE"
        mWAVBuffer[8] = 0x57;
        mWAVBuffer[9] = 0x41;
        mWAVBuffer[10] = 0x56;
        mWAVBuffer[11] = 0x45;

        // "fmt "
        mWAVBuffer[12] = 0x66;
        mWAVBuffer[13] = 0x6D;
        mWAVBuffer[14] = 0x74;
        mWAVBuffer[15] = 0x20;

        // chunk size = 16
        mWAVBuffer[16] = 0x10;
        mWAVBuffer[17] = 0x00;
        mWAVBuffer[18] = 0x00;
        mWAVBuffer[19] = 0x00;

        // format (1 = PCM)
        mWAVBuffer[20] = 0x01;
        mWAVBuffer[21] = 0x00;

        // number of channels
        mWAVBuffer[22] = 0x01;
        mWAVBuffer[23] = 0x00;

        // samples per second
        mWAVBuffer[24] = 0x44;
        mWAVBuffer[25] = 0xAC;
        mWAVBuffer[26] = 0x00;
        mWAVBuffer[27] = 0x00;

        // average bytes per second
        mWAVBuffer[28] = 0x88;
        mWAVBuffer[29] = 0x58;
        mWAVBuffer[30] = 0x01;
        mWAVBuffer[31] = 0x00;

        // bytes to output at a single time
        mWAVBuffer[32] = 0x02;
        mWAVBuffer[33] = 0x00;

        // 16 bits per sample
        mWAVBuffer[34] = 0x10;
        mWAVBuffer[35] = 0x00;

        // "data"
        mWAVBuffer[36] = 0x64;
        mWAVBuffer[37] = 0x61;
        mWAVBuffer[38] = 0x74;
        mWAVBuffer[39] = 0x61;

        // these are the length of the data chunk, to be filled in later
        mWAVBuffer[40] = 0x00;
        mWAVBuffer[41] = 0x00;
        mWAVBuffer[42] = 0x00;
        mWAVBuffer[43] = 0x00;
    }

    //{
        //char **ptr=ov_comment(&mVF,-1)->user_comments;
//      vorbis_info *vi=ov_info(&vf,-1);
        //while(*ptr){
        //  fprintf(stderr,"%s\n",*ptr);
        //  ++ptr;
        //}
//    fprintf(stderr,"\nBitstream is %d channel, %ldHz\n",vi->channels,vi->rate);
//    fprintf(stderr,"\nDecoded length: %ld samples\n", (long)ov_pcm_total(&vf,-1));
//    fprintf(stderr,"Encoded by: %s\n\n",ov_comment(&vf,-1)->vendor);
    //}
    return true;
}

bool LLVorbisDecodeState::decodeSection()
{
    if (!mInFilep)
    {
        LL_WARNS("AudioEngine") << "No cache file to decode in vorbis!" << LL_ENDL;
        return true;
    }
    if (mDone)
    {
//      LL_WARNS("AudioEngine") << "Already done with decode, aborting!" << LL_ENDL;
        return true;
    }
    char pcmout[4096];  /*Flawfinder: ignore*/

    bool eof = false;
    long ret=ov_read(&mVF, pcmout, sizeof(pcmout), 0, 2, 1, &mCurrentSection);
    if (ret == 0)
    {
        /* EOF */
        eof = true;
        mDone = true;
        mValid = true;
//          LL_INFOS("AudioEngine") << "Vorbis EOF" << LL_ENDL;
    }
    else if (ret < 0)
    {
        /* error in the stream.  Not a problem, just reporting it in
           case we (the app) cares.  In this case, we don't. */

        LL_WARNS("AudioEngine") << "BAD vorbis decode in decodeSection." << LL_ENDL;

        mValid = false;
        mDone = true;
        // We're done, return true.
        return true;
    }
    else
    {
//          LL_INFOS("AudioEngine") << "Vorbis read " << ret << "bytes" << LL_ENDL;
        /* we don't bother dealing with sample rate changes, etc, but.
           you'll have to*/
        std::copy(pcmout, pcmout+ret, std::back_inserter(mWAVBuffer));
    }
    return eof;
}

bool LLVorbisDecodeState::finishDecode()
{
    if (!isValid())
    {
        LL_WARNS("AudioEngine") << "Bogus vorbis decode state for " << getUUID() << ", aborting!" << LL_ENDL;
        return true; // We've finished
    }

    if (mFileHandle == LLLFSThread::nullHandle())
    {
        ov_clear(&mVF);

        // write "data" chunk length, in little-endian format
        S32 data_length = static_cast<S32>(mWAVBuffer.size()) - WAV_HEADER_SIZE;
        mWAVBuffer[40] = (data_length) & 0x000000FF;
        mWAVBuffer[41] = (data_length >> 8) & 0x000000FF;
        mWAVBuffer[42] = (data_length >> 16) & 0x000000FF;
        mWAVBuffer[43] = (data_length >> 24) & 0x000000FF;
        // write overall "RIFF" length, in little-endian format
        data_length += 36;
        mWAVBuffer[4] = (data_length) & 0x000000FF;
        mWAVBuffer[5] = (data_length >> 8) & 0x000000FF;
        mWAVBuffer[6] = (data_length >> 16) & 0x000000FF;
        mWAVBuffer[7] = (data_length >> 24) & 0x000000FF;

        //
        // FUDGECAKES!!! Vorbis encode/decode messes up loop point transitions (pop)
        // do a cheap-and-cheesy crossfade
        //
        {
            S16 *samplep;
            S32 i;
            S32 fade_length;
            char pcmout[4096];      /*Flawfinder: ignore*/

            fade_length = llmin((S32)128,(S32)(data_length-36)/8);
            if((S32)mWAVBuffer.size() >= (WAV_HEADER_SIZE + 2* fade_length))
            {
                memcpy(pcmout, &mWAVBuffer[WAV_HEADER_SIZE], (2 * fade_length));    /*Flawfinder: ignore*/
            }
            llendianswizzle(&pcmout, 2, fade_length);

            samplep = (S16 *)pcmout;
            for (i = 0 ;i < fade_length; i++)
            {
                *samplep = llfloor((F32)*samplep * ((F32)i/(F32)fade_length));
                samplep++;
            }

            llendianswizzle(&pcmout, 2, fade_length);
            if((WAV_HEADER_SIZE+(2 * fade_length)) < (S32)mWAVBuffer.size())
            {
                memcpy(&mWAVBuffer[WAV_HEADER_SIZE], pcmout, (2 * fade_length));    /*Flawfinder: ignore*/
            }
            S32 near_end = static_cast<S32>(mWAVBuffer.size()) - (2 * fade_length);
            if ((S32)mWAVBuffer.size() >= ( near_end + 2* fade_length))
            {
                memcpy(pcmout, &mWAVBuffer[near_end], (2 * fade_length));   /*Flawfinder: ignore*/
            }
            llendianswizzle(&pcmout, 2, fade_length);

            samplep = (S16 *)pcmout;
            for (i = fade_length-1 ; i >=  0; i--)
            {
                *samplep = llfloor((F32)*samplep * ((F32)i/(F32)fade_length));
                samplep++;
            }

            llendianswizzle(&pcmout, 2, fade_length);
            if (near_end + (2 * fade_length) < (S32)mWAVBuffer.size())
            {
                memcpy(&mWAVBuffer[near_end], pcmout, (2 * fade_length));/*Flawfinder: ignore*/
            }
        }

        if (36 == data_length)
        {
            LL_WARNS("AudioEngine") << "BAD Vorbis decode in finishDecode!" << LL_ENDL;
            mValid = false;
            return true; // we've finished
        }
        mBytesRead = -1;
        mFileHandle = LLLFSThread::sLocal->write(mOutFilename, &mWAVBuffer[0], 0, static_cast<S32>(mWAVBuffer.size()),
                             new WriteResponder(this));
    }

    if (mFileHandle != LLLFSThread::nullHandle())
    {
        if (mBytesRead >= 0)
        {
            if (mBytesRead == 0)
            {
                LL_WARNS("AudioEngine") << "Unable to write file in LLVorbisDecodeState::finishDecode" << LL_ENDL;
                mValid = false;
                return true; // we've finished
            }
        }
        else
        {
            return false; // not done
        }
    }

    mDone = true;

    LL_DEBUGS("AudioEngine") << "Finished decode for " << getUUID() << LL_ENDL;

    return true;
}

void LLVorbisDecodeState::flushBadFile()
{
    if (mInFilep)
    {
        LL_WARNS("AudioEngine") << "Flushing bad vorbis file from cache for " << mUUID << LL_ENDL;
        mInFilep->remove();
    }
}

//////////////////////////////////////////////////////////////////////////////

class LLAudioDecodeMgr::Impl
{
    friend class LLAudioDecodeMgr;
    Impl();
  public:

    void processQueue();

    void startMoreDecodes();
    void enqueueFinishAudio(const LLUUID &decode_id, LLPointer<LLVorbisDecodeState>& decode_state);
    void checkDecodesFinished();

  protected:
    std::deque<LLUUID> mDecodeQueue;
    std::map<LLUUID, LLPointer<LLVorbisDecodeState>> mDecodes;
};

LLAudioDecodeMgr::Impl::Impl()
{
}

// Returns the in-progress decode_state, which may be an empty LLPointer if
// there was an error and there is no more work to be done.
LLPointer<LLVorbisDecodeState> beginDecodingAndWritingAudio(const LLUUID &decode_id);

// Return true if finished
bool tryFinishAudio(const LLUUID &decode_id, LLPointer<LLVorbisDecodeState> decode_state);

void LLAudioDecodeMgr::Impl::processQueue()
{
    // First, check if any audio from in-progress decodes are ready to play. If
    // so, mark them ready for playback (or errored, in case of error).
    checkDecodesFinished();

    // Second, start as many decodes from the queue as permitted
    startMoreDecodes();
}

void LLAudioDecodeMgr::Impl::startMoreDecodes()
{
    llassert_always(gAudiop);

    LL::WorkQueue::ptr_t main_queue = LL::WorkQueue::getInstance("mainloop");
    // *NOTE: main_queue->postTo casts this refcounted smart pointer to a weak
    // pointer
    LL::WorkQueue::ptr_t general_queue = LL::WorkQueue::getInstance("General");
    const LL::ThreadPool::ptr_t general_thread_pool = LL::ThreadPool::getInstance("General");
    llassert_always(main_queue);
    llassert_always(general_queue);
    llassert_always(general_thread_pool);
    // Set max decodes to double the thread count of the general work queue.
    // This ensures the general work queue is full, but prevents theoretical
    // buildup of buffers in memory due to disk writes once the
    // LLVorbisDecodeState leaves the worker thread (see
    // LLLFSThread::sLocal->write). This is probably as fast as we can get it
    // without modifying/removing LLVorbisDecodeState, at which point we should
    // consider decoding the audio during the asset download process.
    // -Cosmic,2022-05-11
    const size_t max_decodes = general_thread_pool->getWidth() * 2;

    while (!mDecodeQueue.empty() && mDecodes.size() < max_decodes)
    {
        const LLUUID decode_id = mDecodeQueue.front();
        mDecodeQueue.pop_front();

        // Don't decode the same file twice
        if (mDecodes.find(decode_id) != mDecodes.end())
        {
            continue;
        }
        if (gAudiop->hasDecodedFile(decode_id))
        {
            continue;
        }

        // Kick off a decode
        mDecodes[decode_id] = LLPointer<LLVorbisDecodeState>(NULL);
        bool posted = main_queue->postTo(
            general_queue,
            [decode_id]() // Work done on general queue
            {
                LLPointer<LLVorbisDecodeState> decode_state = beginDecodingAndWritingAudio(decode_id);

                if (!decode_state)
                {
                    // Audio decode has errored
                    return decode_state;
                }

                // Disk write of decoded audio is now in progress off-thread
                return decode_state;
            },
            [decode_id, this](LLPointer<LLVorbisDecodeState> decode_state) // Callback to main thread
            mutable {
                if (!gAudiop)
                {
                    // There is no LLAudioEngine anymore. This might happen if
                    // an audio decode is enqueued just before shutdown.
                    return;
                }

                // At this point, we can be certain that the pointer to "this"
                // is valid because the lifetime of "this" is dependent upon
                // the lifetime of gAudiop.

                enqueueFinishAudio(decode_id, decode_state);
            });
        if (! posted)
        {
            // Shutdown
            // Consider making processQueue() do a cleanup instead
            // of starting more decodes
            LL_WARNS() << "Tried to start decoding on shutdown" << LL_ENDL;
        }
    }
}

LLPointer<LLVorbisDecodeState> beginDecodingAndWritingAudio(const LLUUID &decode_id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_MEDIA;

    LL_DEBUGS() << "Decoding " << decode_id << " from audio queue!" << LL_ENDL;

    std::string                    d_path       = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, decode_id.asString()) + ".dsf";
    LLPointer<LLVorbisDecodeState> decode_state = new LLVorbisDecodeState(decode_id, d_path);

    if (!decode_state->initDecode())
    {
        return NULL;
    }

    // Decode in a loop until we're done
    while (!decode_state->decodeSection())
    {
        // decodeSection does all of the work above
    }

    if (!decode_state->isDone())
    {
        // Decode stopped early, or something bad happened to the file
        // during decoding.
        LL_WARNS("AudioEngine") << decode_id << " has invalid vorbis data or decode has been canceled, aborting decode" << LL_ENDL;
        decode_state->flushBadFile();
        return NULL;
    }

    if (!decode_state->isValid())
    {
        // We had an error when decoding, abort.
        LL_WARNS("AudioEngine") << decode_id << " has invalid vorbis data, aborting decode" << LL_ENDL;
        decode_state->flushBadFile();
        return NULL;
    }

    // Kick off the writing of the decoded audio to the disk cache.
    // The receiving thread can then cheaply call finishDecode() again to check
    // if writing has finished. Someone has to hold on to the refcounted
    // decode_state to prevent it from getting destroyed during write.
    decode_state->finishDecode();

    return decode_state;
}

void LLAudioDecodeMgr::Impl::enqueueFinishAudio(const LLUUID &decode_id, LLPointer<LLVorbisDecodeState>& decode_state)
{
    // Assumed fast
    if (tryFinishAudio(decode_id, decode_state))
    {
        // Done early!
        auto decode_iter = mDecodes.find(decode_id);
        llassert(decode_iter != mDecodes.end());
        mDecodes.erase(decode_iter);
        return;
    }

    // Not done yet... enqueue it
    mDecodes[decode_id] = decode_state;
}

void LLAudioDecodeMgr::Impl::checkDecodesFinished()
{
    auto decode_iter = mDecodes.begin();
    while (decode_iter != mDecodes.end())
    {
        const LLUUID& decode_id = decode_iter->first;
        const LLPointer<LLVorbisDecodeState>& decode_state = decode_iter->second;
        if (tryFinishAudio(decode_id, decode_state))
        {
            decode_iter = mDecodes.erase(decode_iter);
        }
        else
        {
            ++decode_iter;
        }
    }
}

bool tryFinishAudio(const LLUUID &decode_id, LLPointer<LLVorbisDecodeState> decode_state)
{
    // decode_state is a file write in progress unless finished is true
    bool finished = decode_state && decode_state->finishDecode();
    if (!finished)
    {
        return false;
    }

    llassert_always(gAudiop);

    LLAudioData *adp = gAudiop->getAudioData(decode_id);
    if (!adp)
    {
        LL_WARNS("AudioEngine") << "Missing LLAudioData for decode of " << decode_id << LL_ENDL;
        return true;
    }

    bool valid = decode_state && decode_state->isValid();
    // Mark current decode finished regardless of success or failure
    adp->setHasCompletedDecode(true);
    // Flip flags for decoded data
    adp->setHasDecodeFailed(!valid);
    adp->setHasDecodedData(valid);
    // When finished decoding, there will also be a decoded wav file cached on
    // disk with the .dsf extension
    if (valid)
    {
        adp->setHasWAVLoadFailed(false);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////

LLAudioDecodeMgr::LLAudioDecodeMgr()
{
    mImpl = new Impl();
}

LLAudioDecodeMgr::~LLAudioDecodeMgr()
{
    delete mImpl;
    mImpl = nullptr;
}

void LLAudioDecodeMgr::processQueue()
{
    mImpl->processQueue();
}

bool LLAudioDecodeMgr::addDecodeRequest(const LLUUID &uuid)
{
    if (gAudiop && gAudiop->hasDecodedFile(uuid))
    {
        // Already have a decoded version, don't need to decode it.
        LL_DEBUGS("AudioEngine") << "addDecodeRequest for " << uuid << " has decoded file already" << LL_ENDL;
        return true;
    }

    if (gAssetStorage->hasLocalAsset(uuid, LLAssetType::AT_SOUND))
    {
        // Just put it on the decode queue it if it's not already in the queue
        LL_DEBUGS("AudioEngine") << "addDecodeRequest for " << uuid << " has local asset file already" << LL_ENDL;
        if (std::find(mImpl->mDecodeQueue.begin(), mImpl->mDecodeQueue.end(), uuid) == mImpl->mDecodeQueue.end())
        {
            mImpl->mDecodeQueue.emplace_back(uuid);
        }
        return true;
    }

    LL_DEBUGS("AudioEngine") << "addDecodeRequest for " << uuid << " no file available" << LL_ENDL;
    return false;
}
