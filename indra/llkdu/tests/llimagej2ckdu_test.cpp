/** 
 * @file llimagej2ckdu_test.cpp
 * @author Merov Linden
 * @date 2010-12-17
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
// Class to test 
#include "llimagej2ckdu.h"

#if __clang__
// For this source, it's true that private fields in llkdumem.h are unused.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#include "llkdumem.h"
#pragma clang diagnostic pop
#else
#include "llkdumem.h"
#endif
#include "kdu_block_coding.h"
// Tut header
#include "lltut.h"

// -------------------------------------------------------------------------------------------
// Stubbing: Declarations required to link and run the class being tested
// Notes: 
// * Add here stubbed implementation of the few classes and methods used in the class to be tested
// * Add as little as possible (let the link errors guide you)
// * Do not make any assumption as to how those classes or methods work (i.e. don't copy/paste code)
// * A simulator for a class can be implemented here. Please comment and document thoroughly.

// End Stubbing
// -------------------------------------------------------------------------------------------
// Stub the LL Image Classes
//LLTrace::MemStatHandle    LLImageBase::sMemStat("LLImage");

LLImageRaw::LLImageRaw() { }
LLImageRaw::~LLImageRaw() { }
U8* LLImageRaw::allocateData(S32 ) { return NULL; }
void LLImageRaw::deleteData() { }
U8* LLImageRaw::reallocateData(S32 ) { return NULL; }
bool LLImageRaw::resize(U16, U16, S8) { return true; } // this method always returns true...

LLImageBase::LLImageBase()
: mData(NULL),
mDataSize(0),
mWidth(0),
mHeight(0),
mComponents(0),
mBadBufferAllocation(false),
mAllowOverSize(false)
{ }
LLImageBase::~LLImageBase() { }
U8* LLImageBase::allocateData(S32 ) { return NULL; }
void LLImageBase::deleteData() { }
void LLImageBase::dump() { }
const U8* LLImageBase::getData() const { return NULL; }
U8* LLImageBase::getData() { return NULL; }
U8* LLImageBase::reallocateData(S32 ) { return NULL; }
void LLImageBase::sanityCheck() { }
void LLImageBase::setSize(S32 , S32 , S32 ) { }

LLImageJ2CImpl::~LLImageJ2CImpl() { }

LLImageFormatted::LLImageFormatted(S8 ) { }
LLImageFormatted::~LLImageFormatted() { }
U8* LLImageFormatted::allocateData(S32 ) { return NULL; }
S32 LLImageFormatted::calcDataSize(S32 ) { return 0; }
S32 LLImageFormatted::calcDiscardLevelBytes(S32 ) { return 0; }
bool LLImageFormatted::decodeChannels(LLImageRaw*, F32, S32, S32) { return false; }
bool LLImageFormatted::copyData(U8 *, S32) { return true; }  // this method always returns true...
void LLImageFormatted::deleteData() { }
void LLImageFormatted::dump() { }
U8* LLImageFormatted::reallocateData(S32 ) { return NULL; }
void LLImageFormatted::resetLastError() { }
void LLImageFormatted::sanityCheck() { }
void LLImageFormatted::setLastError(const std::string& , const std::string& ) { }

LLImageJ2C::LLImageJ2C() : LLImageFormatted(IMG_CODEC_J2C), mRate(DEFAULT_COMPRESSION_RATE) { }
LLImageJ2C::~LLImageJ2C() { }
S32 LLImageJ2C::calcDataSize(S32 ) { return 0; }
S32 LLImageJ2C::calcDiscardLevelBytes(S32 ) { return 0; }
S32 LLImageJ2C::calcHeaderSize() { return 0; }
bool LLImageJ2C::decode(LLImageRaw*, F32) { return false; }
bool LLImageJ2C::decodeChannels(LLImageRaw*, F32, S32, S32 ) { return false; }
void LLImageJ2C::decodeFailed() { }
bool LLImageJ2C::encode(const LLImageRaw*, F32) { return false; }
S8  LLImageJ2C::getRawDiscardLevel() { return 0; }
void LLImageJ2C::resetLastError() { }
void LLImageJ2C::setLastError(const std::string&, const std::string&) { }
bool LLImageJ2C::updateData() { return false; }
void LLImageJ2C::updateRawDiscardLevel() { }

LLKDUMemIn::LLKDUMemIn(const U8*, const U32, const U16, const U16, const U8, kdu_core::siz_params*) { }
LLKDUMemIn::~LLKDUMemIn() { }
bool LLKDUMemIn::get(int, kdu_core::kdu_line_buf&, int) { return false; }

// Stub Kakadu Library calls
// they're all namespaced now
namespace kdu_core_local
{
    class kd_coremem;
}
namespace kdu_core {
kdu_tile_comp kdu_tile::access_component(int ) { kdu_tile_comp a; return a; }
kdu_block_encoder::kdu_block_encoder() { }
kdu_block_decoder::kdu_block_decoder() { }
void kdu_block::set_max_passes(int , bool ) { }
void kdu_block::set_max_bytes(int , bool ) { }
void kdu_tile::close(kdu_thread_env *, bool) {}
int kdu_tile::get_num_components() { return 0; }
bool kdu_tile::get_ycc() { return false; }
void kdu_tile::set_components_of_interest(int , const int* ) { }
int kdu_tile::get_tnum() { return 0; }
kdu_resolution kdu_tile_comp::access_resolution() { kdu_resolution a; return a; }
kdu_resolution kdu_tile_comp::access_resolution(int ) { kdu_resolution a; return a; }
int kdu_tile_comp::get_bit_depth(bool ) { return 8; }
bool kdu_tile_comp::get_reversible() { return false; }
int kdu_tile_comp::get_num_resolutions() { return 1; }
kdu_subband kdu_resolution::access_subband(int ) { kdu_subband a; return a; }
void kdu_resolution::get_dims(kdu_dims& ) { }
int kdu_resolution::which() { return 0; }
int kdu_resolution::get_valid_band_indices(int &) { return 1; }
kdu_synthesis::kdu_synthesis(kdu_resolution, kdu_sample_allocator*, bool, float, kdu_thread_env*, kdu_thread_queue*) { }
//kdu_params::kdu_params(const char*, bool, bool, bool, bool, bool) { }
kdu_params::kdu_params(const char*, bool, bool, bool, bool, bool, kd_core_local::kd_coremem*) {}
kdu_params::~kdu_params() { }
void kdu_params::set(const char* , int , int , bool ) { }
void kdu_params::set(const char* , int , int , int ) { }
void kdu_params::finalize_all(bool ) { }
void kdu_params::finalize_all(int, bool ) { }
void kdu_params::copy_from(kdu_params*, int, int, int, int, int, bool, bool, bool) { }
bool kdu_params::parse_string(const char*) { return false; }
bool kdu_params::get(const char*, int, int, bool&, bool, bool, bool) { return false; }
bool kdu_params::get(const char*, int, int, float&, bool, bool, bool) { return false; }
bool kdu_params::get(const char*, int, int, int&, bool, bool, bool) { return false; }
kdu_params* kdu_params::access_relation(int, int, int, bool) { return NULL; }
kdu_params* kdu_params::access_cluster(const char*) { return NULL; }
void kdu_codestream::set_fast() { }
void kdu_codestream::set_fussy() { }
void kdu_codestream::get_dims(int, kdu_dims&, bool ) { }
int kdu_codestream::get_min_dwt_levels() { return 5; }
int kdu_codestream::get_max_tile_layers() { return 1; }
void kdu_codestream::change_appearance(bool, bool, bool, kdu_thread_env *) {}
void kdu_codestream::get_tile_dims(kdu_coords, int, kdu_dims&, bool ) { }
void kdu_codestream::destroy() { }
void kdu_codestream::collect_timing_stats(int ) { }
void kdu_codestream::set_max_bytes(kdu_long, bool, bool ) { }
void kdu_codestream::get_valid_tiles(kdu_dims& ) { }
void kdu_codestream::create(
    siz_params*,
    kdu_compressed_target*,
    kdu_dims*,
    int,
    kdu_long,
    kdu_thread_env*,
    kdu_membroker*) {}
void kdu_codestream::create(kdu_compressed_source *,kdu_thread_env *, kdu_membroker *) {}
void kdu_codestream::apply_input_restrictions(int, int, int, int, kdu_dims const *, kdu_component_access_mode, kdu_thread_env *, kdu_quality_limiter const *) {}
void kdu_codestream::get_subsampling(int , kdu_coords&, bool ) { }
void kdu_codestream::flush(kdu_long *, int, kdu_uint16 *, bool, bool, double, kdu_thread_env*, int) { }
void kdu_codestream::set_resilient(bool ) { }
int kdu_codestream::get_num_components(bool ) { return 0; }
kdu_long kdu_codestream::get_total_bytes(bool ) { return 0; }
kdu_long kdu_codestream::get_compressed_data_memory(bool ) const {return 0; }
void kdu_codestream::share_buffering(kdu_codestream ) { }
int kdu_codestream::get_num_tparts() { return 0; }
int kdu_codestream::trans_out(kdu_long, kdu_long*, int, bool, kdu_thread_env* ) { return 0; }
bool kdu_codestream::ready_for_flush(kdu_thread_env*) { return false; }
siz_params* kdu_codestream::access_siz() { return NULL; }
kdu_tile kdu_codestream::open_tile(kdu_coords , kdu_thread_env* ) { kdu_tile a; return a; }
kdu_codestream_comment kdu_codestream::add_comment(kdu_thread_env*) { kdu_codestream_comment a; return a; }
void kdu_subband::close_block(kdu_block*, kdu_thread_env*) { }
void kdu_subband::get_valid_blocks(kdu_dims &indices) const { }
kdu_block * kdu_subband::open_block(kdu_coords, int *, kdu_thread_env *, int, bool) { return NULL; }
bool kdu_codestream_comment::put_text(const char*) { return false; }
void kdu_customize_warnings(kdu_message*) { }
void kdu_customize_errors(kdu_message*) { }
kdu_long kdu_multi_analysis::create(
    kdu_codestream,
    kdu_tile,
    kdu_thread_env*,
    kdu_thread_queue*,
    int,
    kdu_roi_image*,
    int,
    kdu_sample_allocator*,
    const kdu_push_pull_params*,
    kdu_membroker*) { return kdu_long(0); }
void kdu_multi_analysis::destroy(kdu_thread_env *) {}
siz_params::siz_params(kd_core_local::kd_coremem*) : kdu_params(NULL, false, false, false, false, false) { }
siz_params::~siz_params() {}
void siz_params::finalize(bool ) { }
void siz_params::copy_with_xforms(kdu_params*, int, int, bool, bool, bool) { }
int siz_params::write_marker_segment(kdu_output*, kdu_params*, int) { return 0; }
bool siz_params::check_marker_segment(kdu_uint16, int, kdu_byte a[], int&) { return false; }
bool siz_params::read_marker_segment(kdu_uint16, int, kdu_byte a[], int) { return false; }
kdu_decoder::kdu_decoder(
    kdu_subband subband,
    kdu_sample_allocator*,
    bool, float, int,
    kdu_thread_env*,
    kdu_thread_queue*,
    int, float*) {}
kdu_sample_allocator::~kdu_sample_allocator() {}
void kdu_sample_allocator::do_finalize(kdu_codestream) {}
void (*kdu_convert_ycc_to_rgb_rev16)(kdu_int16*,kdu_int16*,kdu_int16*,int);
void (*kdu_convert_ycc_to_rgb_irrev16)(kdu_int16*,kdu_int16*,kdu_int16*,int);
void (*kdu_convert_ycc_to_rgb_rev32)(kdu_int32*,kdu_int32*,kdu_int32*,int);
void (*kdu_convert_ycc_to_rgb_irrev32)(float*,float*,float*,int);
bool kdu_core_sample_alignment_checker(int, int, int, int, bool, bool) { return false; }
void kdu_pull_ifc::destroy() {}
void kdu_sample_allocator::advance_pre_frag() {}
void kdu_params::operator delete(void *) {}
} // namespace kdu_core

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------

namespace tut
{
    // Test wrapper declarations
    struct llimagej2ckdu_test
    {
        // Derived test class
        class LLTestImageJ2CKDU : public LLImageJ2CKDU
        {
        public:
            // Provides public access to some protected methods for testing
            bool callGetMetadata(LLImageJ2C &base) { return getMetadata(base); }
            bool callDecodeImpl(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, S32 first_channel, S32 max_channel_count)
            {
                return decodeImpl(base, raw_image, decode_time, first_channel, max_channel_count);
            }
            bool callEncodeImpl(LLImageJ2C &base, const LLImageRaw &raw_image, const char* comment_text)
            {
                return encodeImpl(base, raw_image, comment_text);
            }
        };
        // Instance to be tested
        LLTestImageJ2CKDU* mImage;
        
        // Constructor and destructor of the test wrapper
        llimagej2ckdu_test()
        {
            mImage = new LLTestImageJ2CKDU;
        }
        ~llimagej2ckdu_test()
        {
            delete mImage;
        }
    };
    
    // Tut templating thingamagic: test group, object and test instance
    typedef test_group<llimagej2ckdu_test> llimagej2ckdu_t;
    typedef llimagej2ckdu_t::object llimagej2ckdu_object_t;
    tut::llimagej2ckdu_t tut_llimagej2ckdu("LLImageJ2CKDU");
    
    // ---------------------------------------------------------------------------------------
    // Test functions
    // Notes:
    // * Test as many as you possibly can without requiring a full blown simulation of everything
    // * The tests are executed in sequence so the test instance state may change between calls
    // * Remember that you cannot test private methods with tut
    // ---------------------------------------------------------------------------------------

    // Test 1 : test getMetadata()
    template<> template<>
    void llimagej2ckdu_object_t::test<1>()
    {
        LLImageJ2C* image = new LLImageJ2C();
        bool res = mImage->callGetMetadata(*image);
        // Trying to set up a data stream with all NIL values and stubbed KDU will "work" and return true
        // Note that is linking with KDU, that call will throw an exception and fail, returning false
        ensure("getMetadata() test failed", res);
    }

    // Test 2 : test decodeImpl()
    template<> template<>
    void llimagej2ckdu_object_t::test<2>()
    {
        LLImageJ2C* image = new LLImageJ2C();
        LLImageRaw* raw = new LLImageRaw();
        bool res = mImage->callDecodeImpl(*image, *raw, 0.0, 0, 0);
        // Decoding returns true whenever there's nothing else to do, including if decoding failed, so we'll get true here
        ensure("decodeImpl() test failed", res);
    }

    // Test 3 : test encodeImpl()
    template<> template<>
    void llimagej2ckdu_object_t::test<3>()
    {
        LLImageJ2C* image = new LLImageJ2C();
        LLImageRaw* raw = new LLImageRaw();
        bool res = mImage->callEncodeImpl(*image, *raw, NULL);
        // Encoding returns true unless an exception was raised, so we'll get true here though nothing really was done
        ensure("encodeImpl() test failed", res);
    }
}
