/**
 * @file lldiskcache.cpp
 * @brief Use a worker thread read/write from/to disk
 *        via a thread safe request queue and avoid the
 *        need for complex code or file locking.
 *
 * There are 2 interesting components to this class:
 * 1/ The work (reading/writing) from disk happens in its
 *    own thread to avoid stalling the main loop. To do
 *    some work on this thread, you construct a request with
 *    the appropriate parameters and add it to the input queue
 *    which is implemented using LLThreadSafeQuue. At some point
 *    later, the result (id, payload, result code) appears on a
 *    second queue (also implemented as an LLThreadSafeQueue).
 *    The advantage of this approach is that so long as the
 *    LLThreadSafeQueue works correctly, there is no locking/mutex
 *    control needed as the queues behave like thread boundaries.
 *    Similarly, since all the file access is done sequentially
 *    in a single thread, there is no file level locking required.
 *    There may be a small performance increase to be gained
 *    by implementing N queues and the LLThreadSafe code would take
 *    care of managing the callable functions. However, since you
 *    would have to take account of the possibility of reading and/or
 *    writing the same file (it's a cache so that's a possibility)
 *    from multiple threads, the code complexity would rise
 *    dramatically. The assertion is that this code will be plenty
 *    fast enough and is very straightforward.
 * 2/ The caching mechanism... TODO: describe cache here...
 *
 *
 *
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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
#include "llcoros.h"
#include "lleventcoro.h"
#include "lldir.h"

#include "lldiskcache.h"

llDiskCache::llDiskCache() :
    LLEventTimer(mTimePeriod)
{
    /**
     * Create the worker thread and the function that
     * processes requests submitted to the thread
     */
    mWorkerThread = std::thread([this]()
    {
        requestThread();
    });

    /**
     * Start the timer that drives the request queue via the
     * LLEventTimer API. The time period is defined in the
     * header file - can be set to 0.0 which means service the
     * tick() override every frame
     */
    mEventTimer.start();
}

/**
 * Overrides the LLSingleton method that cleans up after we
 * are all finished - this idiom is now preferred over the
 * regular use of our own class destructor
 */
void llDiskCache::cleanupSingleton()
{
    /**
     * Note:
     * We do cleanup here vs the destructor based on recommendations found
     * in the llsingleton.h comments: Any cleanup logic that might take
     * significant real time -- or throw an exception -- must not be placed
     * in your LLSingleton's destructor, but rather in its cleanupSingleton()
     * method, which is called implicitly by deleteSingleton().
     */

    /**
     * we won't be putting anything else onto the outbound request queue
     * so we close it to indicate to the worker function that we are finished
     */
    mITaskQueue.close();

    /**
     * Some idioms have a test here for std::thread::joinable() before the
     * call to join() is made but we don't need it here. There are a very
     * narrow set of circumstances where it won't be joinable and none of
     * them apply here so we can safely just call join() and have the thread
     * come back to the main loop
     */
    mWorkerThread.join();

    /**
     * Note:
     * There is a possibility that there may be remaining items in
     * this queue that will not be acted up - thrown away in fact. Given
     * the nature of this caching code and the fact we are shutting down
     * here, this is okay - it would introduce more complexity than we want
     * to take on to ensure than this wasn't the case
     */
}

/**
 * Process requests on the worker thread from the input queue and push
 * the result out to the output queue
 */
void llDiskCache::requestThread()
{
    /**
     * We used to use a while (!mITaskQueue.isClosed()) test for this loop
     * but there is an issue with LLThreadSafeQueue whereby the queue
     * has not yet been closed so isClosed() returns false. After we
     * make the call to popBack() the queue does get closed and the
     * next time through it throws an LLThreadSafeQueueInterrupt
     * exception. The solution for the moment is to catch the exception
     * and do nothing with it.
     */
    try
    {
        while (true)
        {
            /**
             * Grab a request off the queue to process
             */
            auto request = mITaskQueue.popBack();

            /**
             * This is where the real work happens. The callable we pulled off
             * of the input queue is executed here and the result captured.
             */
            auto result = request();

            /**
             * Push the result out to the outbound results queue
             */
            mResultQueue.pushFront(result);
        }
    }
    catch (const LLThreadSafeQueueInterrupt&)
    {
        /**
         * Do nothing with this for the moment
         */
    }

    /**
     * Calling close() here indicates that we are finished with the output queue
     * the it can be closed. See the note about potentially losing the last few
     * items in the queue under some circumstances elsewhere in this file
     */
    mResultQueue.close();
}

BOOL llDiskCache::tick()
{
    /**
     * Here will pull all outstanding results off of the output
     * queue and call their callbacks to that the consumer can
     * retrieve the result of the operation they requested.
     * Depending on how this code evolves, we might consider
     * adding a throttle here so that the full contents of the
     * queue is not read each time. Instead, we might only
     * taken N items off, or taken items for M milliseconds
     * before aborting for this update pass and picking up
     * where we left off next time through
     */
    mResult res;
    while (mResultQueue.tryPopBack((res)))
    {
        /**
         * Optional debugging messages
         */
        //LL_INFOS("DiskCache") << "Working: thread returned " << res.ok << " and id " << res.id << LL_ENDL;

        /**
         * Note:
         * There is no need to lock the map because it's only accessed
         * on the main thread - one of the benefits of this idiom
         */
        auto found = mRequestMap.find(res.id);
        if (found != mRequestMap.end())
        {
            /**
             * Execute the callback and pass the payload/result status
             * back to the consumer
             */
            found->second.cb(res.payload, res.filename, res.ok);

            /**
             * We have processed this entry in the queue so delete it
             */
            mRequestMap.erase(found);
        }
        else
        {
            /**
             * It should not be possible to hit this code but we might
             * consider what happens if we do - assert or throw perhaps?
             */
        }
    }

    return FALSE;
}

/**
 * Adds a request to read a file from disk asynchronously to
 * the request queue and activate a callback containing the
 * read payload when it is complete.
 * TODO: Not clear yet if this is the model we will use but useful
 * for testing in any event. The id is used as the basis for
 * generating a filename - see idToFilepath() for more details on
 * how the final file/path is created. The callback cb is triggered
 * once the request is processed
 */
void llDiskCache::addReadRequest(std::string id,
                                 request_callback_t cb)
{
    /**
     * This is an ID we pass in to our worker function so
     * we can match up requests and results by comparing the id
     */
    auto request_id = mRequestId++;

    /**
     * Record the ID in a map - used to compare against results
     * queue in update (per tick) function
     */
    mRequestMap.emplace(request_id, mRequest{ cb });

    /**
     * In the future, we should consider if the code that runs on the
     * request thread here can throw an exception - this needs to be accounted
     * and @Nat has a sugestion around using std::packaged_task(..). We do not
     * need it for this use case - we assert this code will not throw but
     * other, more complex code that uses this pattern in the future might
     */
    mITaskQueue.pushFront([this, request_id, id]()
    {
        /**
         * Munge the ID we get into a full file/path name. This might change
         * once we decide how to actually store the files we read - on
         * disk directly? In a database? Pointed to be a database?
         * TODO: This works for now but will need to revisit
         */
        const std::string filename = idToFilepath(id, LLAssetType::AT_UNKNOWN);

        /**
         * This is an interesting idiom. We will be passing back the contents of files
         * we read and an std::vector<U8> is suitable for that. However, that means
         * we will typically be moving or copying it around and that is inefficient.
         * Adding a smart (shared) pointer into the mix means we can pass that around
         * and the consumer can either hang onto it (its ref count is incremented) or
         * just look and ignore it - when no one has a reference to it anymore, the
         * C++ mechanics will automatically delete it.
         */
        request_payload_t file_contents = nullptr;

        /**
         * TODO: This is a place holder for doing some work on the worker
         * thread - eventually, for this use case, it will be used to
         * read and write cache files and update their meta data
         */
        bool success = true;
        std::ifstream ifs(filename);
        if (!ifs.is_open())
        {
            success = false;
        }
        else
        {
            std::string contents;
            if (std::getline(ifs, contents))
            {
                const U32 filesize = contents.length();
                file_contents = std::make_shared<std::vector<U8>>(filesize + 1);
                memset(file_contents->data(), 0, filesize + 1);
                memcpy(file_contents->data(), contents.c_str(), filesize);
                success = true;
            }
        }

        /**
         * We pass back the ID (for lookup), the contents of the file we read
         * (in our use case) and a flag indicating success/failure. We might
         * also consider using file_contents and comparing with ! vs a
         * separate bool. This is okay for now though.
         */
        return mResult{ request_id, file_contents, filename, success };
    });
}

/**
 * Adds a request to read a file from disk synchronously to
 * the request queue and returns a read payload when the operation
 * is complete.
 * TODO: Not clear yet if this is the model we will use but useful
 * for testing in any event. The id is used as the basis for
 * generating a filename - see idToFilepath() for more details on
 * how the final file/path is created. The callback cb is triggered
 * once the request is processed
 */
llDiskCache::request_payload_t llDiskCache::waitForReadComplete(std::string id)
{
    /**
     * There are two cases to consider for the synchronous case. One is when
     * the initiating request is on the main coroutine/fiber (where tick() is
     * called from) and one when is is initiated from a different coroutine/fiber.
     * We have asserted in code that the way to tell if you are on the main
     * coroutine is if the name is empty so we can use that to test.
     * Note:
     * Here we will return a payload object if the operation succeeded and
     * we will throw an exception indicating failure if it does not. The code
     * that consumes this function must catch the error or the viewer will crash
     */

    /**
     * Here we are on the same coroutine/fiber as tick()
     */
    if (LLCoros::getName().empty())
    {
        /**
         * The payload we will return containing the result of the read request
         */
        request_payload_t payload_out;

        /**
         * A flag indicating if the read succeeded or not
         */
        bool succeeded = false;

        /**
         * A flag indicating if the operation is complete
         */
        bool done = false;

        /**
         * The filename being read that is returned
         * to consumer if a read error occurs
         */
        std::string filename_out;

        /**
         * Add an asynchronous read request to the request queue
         * running in our worker thread
         */
        addReadRequest(id, [&done, &succeeded, &payload_out, &filename_out]
                       (llDiskCache::request_payload_t payload_in, std::string filename_in, bool result)
        {
            /**
             * Note the result of the operation
             */
            succeeded = result;

            /**
             * Note that the operation is complete
             */
            done = true;

            /**
             * Record the name of the file that was being read
             * so we can return it via the exception if read fails
             */
            filename_out = filename_in;

            /**
             * Save the payload we received in the callback so we
             * can return it
             */
            payload_out = payload_in;
        });

        /**
         * Wait for the flag indicating the operation is complete
         */
        while (!done)
        {
            /**
             * This lets other, unrelated coroutines proceed while we
             * wait for the read request to complete
             */
            llcoro::suspend();

            /**
             * We are running on the same coroutine/fiber as tick() so
             * while we are waiting here, it won't be called which is bad.
             * The fix for this is to call it ourselves directly and keep
             * the request queue running
             */
            tick();
        }

        if (! succeeded)
        {
            /**
             * Throw an exception indicating that the read operation failed.
             * Include a helpful message and the name of the file being read
             */
            const std::string exc_msg = "Unable to read from: " + filename_out;

            throw ReadError(exc_msg);
        }

        /**
         * Everything seems okay so we can safely return the payload object
         * containing the result of the read operation
         */
        return payload_out;
    }
    else
        /**
         * This is the case where we are NOT on same coroutine/fiber as tick()
         * so we are allowed to block
         */
    {
        /**
         * We use a promise to return the payload
         */
        LLCoros::Promise<request_payload_t> promise;

        /**
         * We use a future to track the promise
         */
        auto future = LLCoros::getFuture(promise);

        /**
         * Note:
         * A promise is not copyable so we have to move it instead using std::move(promise).
         * However, VS2017 did not allow the (promise = std::move(promise)) syntax in the
         * addReadRequest(..) call so we have to revert to using a reference which in this
         * use case is okay because the lifetime of the promise is gated by future.get()
         * @Nat assures me that the move syntax we want is valid so it is a mystery why
         * it doesn't work here in VS2017.
         * TODO: we should find out why - the lifetime might be more important for other
         * use cases.
         * Note:
         * We have to use the mutable keyboard in the addReadRequest() expression to indicate
         * we are allowing the lambda body to make non-const method calls (normally, all const)
         */
        addReadRequest(id, [&promise](request_payload_t payload, std::string filename, bool result) mutable
        {
            /**
             * Note:
             * We ought to deal with the case where result is false - I.E. the operation
             * failed for some reason. We might consider using
             * promise.set_exception(std:::make_exception_ptr()) to make futre.get() throw
             * and assert that the calling code catch it
             */

            /**
             * When the callback is triggered, we update the promise with the payload
             * containing the result of the read reqest
             */
            promise.set_value(payload);
        });

        /**
         * When our promise is set, we can return the result via the future
         */
        return future.get();
    }
}

/**
 * Adds a request to write a file to disk asynchronously to
 * the request queue and activate a callback containing the
 * status of the operation when it is complete.
 * TODO: Not clear yet if this is the model we will use but useful
 * for testing in any event. The id is used as the basis for
 * generating a filename - see idToFilepath() for more details on
 * how the final file/path is created. The callback cb is triggered
 * once the request is processed
 */
void llDiskCache::addWriteRequest(std::string id,
                                  LLAssetType::EType at,
                                  request_payload_t buffer,
                                  request_callback_t cb)
{
    /**
     * This is an ID we pass in to our worker function so
     * we can match up requests and results by comparing the id
     */
    auto request_id = mRequestId++;

    /**
     * Record the ID in a map - used to compare against results
     * queue in update (per tick) function
     */
    mRequestMap.emplace(request_id, mRequest{ cb });

    /**
     * In the future, we should consider if the code that runs on the
     * request thread here can throw an exception - this needs to be accounted
     * and @Nat has a sugestion around using std::packaged_task(..). We do not
     * need it for this use case - we assert this code will not throw but
     * other, more complex code that uses this pattern in the future might
     */
    mITaskQueue.pushFront([this, request_id, id, at, buffer]()
    {
        /**
         * Munge the ID we get into a full file/path name. This might change
         * once we decide how to actually store the files we read - on
         * disk directly? In a database? Pointed to be a database?
         * TODO: This works for now but will need to revisit
         */
        const std::string filename = idToFilepath(id, at);

        /**
         * TODO: This is a place holder for doing some work on the worker
         * thread - eventually, for this use case, it will be used to
         * read and write cache files and update their meta data
         */
        bool success;
        std::ofstream ofs(filename, std::ios::binary);
        if (ofs)
        {
            ofs.write(reinterpret_cast<char*>(buffer->data()), buffer->size());
            success = true;
        }
        else
        {
            success = false;
        }

        /**
         * We don't send anything back when we are writing. In
         * this case, the request result is returned as a bool
         */
        request_payload_t file_contents = nullptr;

        /**
         * We pass back the ID (for lookup), the contents of the file we read
         * (in our use case) and a flag indicating success/failure. We might
         * also consider using file_contents and comparing with ! vs a
         * separate bool. This is okay for now though.
         */
        return mResult{ request_id, file_contents, filename, success };
    });
}

/**
 * Utility function to return the name of an asset type as
 * a string given an LLAssetType enum. This is useful for
 * debugging and might be useful elsewhere too.
 */
const std::string llDiskCache::assetTypeToString(LLAssetType::EType at)
{
    /**
     * Make use of the C++17 (or is it 14) feature that allows
     * for inline initialization of an std::map<>
     */
    typedef std::map<LLAssetType::EType, std::string> asset_type_to_name_t;
    asset_type_to_name_t asset_type_to_name =
    {
        { LLAssetType::AT_TEXTURE, "TEXTURE" },
        { LLAssetType::AT_SOUND, "SOUND" },
        { LLAssetType::AT_CALLINGCARD, "CALLINGCARD" },
        { LLAssetType::AT_LANDMARK, "LANDMARK" },
        { LLAssetType::AT_SCRIPT, "SCRIPT" },
        { LLAssetType::AT_CLOTHING, "CLOTHING" },
        { LLAssetType::AT_OBJECT, "OBJECT" },
        { LLAssetType::AT_NOTECARD, "NOTECARD" },
        { LLAssetType::AT_CATEGORY, "CATEGORY" },
        { LLAssetType::AT_LSL_TEXT, "LSL_TEXT" },
        { LLAssetType::AT_LSL_BYTECODE, "LSL_BYTECODE" },
        { LLAssetType::AT_TEXTURE_TGA, "TEXTURE_TGA" },
        { LLAssetType::AT_BODYPART, "BODYPART" },
        { LLAssetType::AT_SOUND_WAV, "SOUND_WAV" },
        { LLAssetType::AT_IMAGE_TGA, "IMAGE_TGA" },
        { LLAssetType::AT_IMAGE_JPEG, "IMAGE_JPEG" },
        { LLAssetType::AT_ANIMATION, "ANIMATION" },
        { LLAssetType::AT_GESTURE, "GESTURE" },
        { LLAssetType::AT_SIMSTATE, "SIMSTATE" },
        { LLAssetType::AT_LINK, "LINK" },
        { LLAssetType::AT_LINK_FOLDER, "LINK_FOLDER" },
        { LLAssetType::AT_MARKETPLACE_FOLDER, "MARKETPLACE_FOLDER" },
        { LLAssetType::AT_WIDGET, "WIDGET" },
        { LLAssetType::AT_PERSON, "PERSON" },
        { LLAssetType::AT_MESH, "MESH" },
        { LLAssetType::AT_UNKNOWN, "UNKNOWN" }
    };

    asset_type_to_name_t::iterator iter = asset_type_to_name.find(at);
    if (iter != asset_type_to_name.end())
    {
        return iter->second;
    }

    return std::string("UNKNOWN");
}

/**
 * Utility function to construct a fully qualified file/path based
 * on an ID (typically a UUID) that is passed in. If you pass in
 * something other than a UUID (I.E. not unique) then there may
 * be a logical file collision (no attempt to 'uniquify' the file
 * is made)
 */
const std::string llDiskCache::idToFilepath(const std::string id, LLAssetType::EType at)
{
    /**
     * For the moment this is just {UUID}_{ASSET_TYPE}.txt but of
     * course,  will be greatly expanded upon
     */
    std::ostringstream ss;
    ss << "cp_";
    ss << id;
    ss << "_";
    ss << assetTypeToString(at);
    ss << ".txt";

    const std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, ss.str());

    return filepath;
}
