/**
 * @file decklink_output.cpp
 * @author
 * @brief decklink output, send SDI frame by decklink card
 * @version 1.0
 * @date 2022-07-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <boost/core/noncopyable.hpp>
#include <boost/lockfree/policies.hpp>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include <boost/thread/thread.hpp>

#include <chrono>

#include "core/util/logger.h"
#include "decklink/util.h"
#include "decklink/manager.h"
#include "core/util/color_conversion.h"
#include "decklink/output/decklink_output.h"

#include <memory>
#include <mtl/st_convert_api.h>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <boost/lockfree/queue.hpp>

namespace {
    template<class T>
    class frame_queue {
    public:
        explicit frame_queue(std::size_t size): producer_items(size), consumer_items(size) {}

        bool producer_pop(T &ret) {
            return producer_items.pop(ret);
        }

        bool producer_push(const T &v) {
            return producer_items.bounded_push(v);
        }

        bool consumer_push(const T &v) {
            return consumer_items.bounded_push(v);
        }

        bool consumer_pop(T &ret) {
            return consumer_items.pop(ret);
        }
    private:
        boost::lockfree::queue<T, boost::lockfree::fixed_sized<true>> producer_items;
        boost::lockfree::queue<T, boost::lockfree::fixed_sized<true>> consumer_items;
    };
} // namespace

using namespace seeder::core;
using namespace seeder::decklink::util;
namespace seeder::decklink
{
    decklink_output::decklink_output(
        const std::string &output_id, 
        int device_id, 
        const core::video_format_desc &format_desc, 
        const std::string &pixel_format
    )
    :output(output_id), decklink_index_(device_id - 1),
    format_desc_(format_desc)
    {
        pixel_format_string_ = pixel_format;
        if (pixel_format == "bmdFormat10BitYUV") {
            pixel_format_ = bmdFormat10BitYUV;
        } else if (pixel_format == "bmdFormat8BitBGRA") {
            pixel_format_ = bmdFormat8BitBGRA;
        }
        sample_type_ = bmdAudioSampleType16bitInteger; //bmdAudioSampleType32bitInteger;
        if(format_desc_.audio_samples > 16)
            sample_type_ = bmdAudioSampleType32bitInteger;
        HRESULT result;
        bmd_mode_ = get_decklink_video_format(format_desc_.format);
        video_flags_ = 0;
        
        // get the decklink device
        decklink_ = device_manager::instance().get_decklink(decklink_index_);
        if(decklink_ == nullptr)
        {
            logger->error("Failed to get DeckLink device. Make sure that the device ID is valid");
            throw std::runtime_error("Failed to get DeckLink device.");
        }

        // get output interface of the decklink device
        result = decklink_->QueryInterface(IID_IDeckLinkOutput, (void**)&output_);
        if(result != S_OK)
        {
            logger->error("Failed to get DeckLink output interface.");
            throw std::runtime_error("Failed to get DeckLink output interface.");
        }

        // get decklink output mode
        result = output_->GetDisplayMode(bmd_mode_, &display_mode_);
        if(result != S_OK)
        {
           logger->error("Failed to get DeckLink output mode.");
           throw std::runtime_error("Failed to get DeckLink output mode.");
        }

        // enable video output
        result = output_->EnableVideoOutput(bmd_mode_, bmdVideoOutputFlagDefault);
        if (result != S_OK)
        {
            logger->error("Unable to enable video output {}", result);
            throw std::runtime_error("Unable to enable video output");
        }

        // create display frame
        auto outputBytesPerRow = display_mode_->GetWidth() * 4; //display_mode_->GetWidth() * 4;//bmdFormat8BitBGRA //((display_mode_->GetWidth() + 47) / 48) * 128; // bmdFormat10BitYUV
        result = output_->CreateVideoFrame((int32_t)display_mode_->GetWidth(),
													  (int32_t)display_mode_->GetHeight(),
													  outputBytesPerRow,
													  pixel_format_,
													  bmdFrameFlagDefault,
													  &playbackFrame_);
        if(result != S_OK)
        {
           logger->error("Unable to create video frame.");
           throw std::runtime_error("Unable to create video frame.");
        }
        auto PerRow = ((display_mode_->GetWidth() + 47) / 48) * 128; //display_mode_->GetWidth() * 4;//bmdFormat8BitBGRA //((display_mode_->GetWidth() + 47) / 48) * 128; // bmdFormat10BitYUV
        result = output_->CreateVideoFrame((int32_t)display_mode_->GetWidth(),
													  (int32_t)display_mode_->GetHeight(),
													  PerRow,
													  bmdFormat10BitYUV,
													  bmdFrameFlagDefault,
													  &yuv10Frame_);
        vframe_buffer = (uint8_t*)malloc(format_desc_.height * ((display_mode_->GetWidth() + 47) / 48) * 128);

        // frame buffer pointer
        // if (playbackFrame_->GetBytes((void**)&vframe_buffer) != S_OK)
        // {
        //     logger->error("Could not get DeckLinkVideoFrame buffer pointer");
        // }

        // enable audio output
        result = output_->EnableAudioOutput(format_desc.audio_sample_rate, sample_type_, 
                                            format_desc.audio_channels, bmdAudioOutputStreamTimestamped);
        if (result != S_OK)
        {
            logger->error("Unable to enable audio output {}", result);
            throw std::runtime_error("Unable to enable audio output");
        }

        frameConverter = CreateVideoConversionInstance();
        if (frameConverter == nullptr)
        {
            logger->error("Unable to get Video Conversion interface");
            throw std::runtime_error("Unable to get Video Conversion interface");
        }

        // create audio frame buffer
        aframe_buffer = (uint8_t*)malloc(sample_type_ * format_desc_.audio_channels * format_desc_.sample_num);

        // // ffmpeg sws context, AV_PIX_FMT_UYVY422 to AV_PIX_FMT_RGBA
        // sws_ctx_ = sws_getContext(format_desc.width, format_desc.height, AV_PIX_FMT_UYVY422,
        //                     format_desc.width, format_desc.height, AV_PIX_FMT_BGRA, 
        //                     SWS_BICUBIC, NULL, NULL, NULL);
        
        // dst_frame_ = av_frame_alloc();
        // int buf_size = av_image_get_buffer_size(AV_PIX_FMT_BGRA, format_desc.width, format_desc.height, 1);
        // dst_frame_buffer_ = (uint8_t*)av_malloc(buf_size);
        // av_image_fill_arrays(dst_frame_->data, dst_frame_->linesize, dst_frame_buffer_, AV_PIX_FMT_BGRA, 
        //                     format_desc.width, format_desc.height, 1);
    }

    decklink_output::~decklink_output()
    {
        stop();
        
        if (display_mode_ != nullptr) display_mode_->Release();
 
        if (output_ != nullptr) output_->Release();
 
        if (playbackFrame_ != nullptr) playbackFrame_->Release();
 
        if (sws_ctx_) sws_freeContext(sws_ctx_);
 
        if (dst_frame_)  av_frame_free(&dst_frame_);
 
        if (dst_frame_buffer_) av_free(dst_frame_buffer_);
 
        if (aframe_buffer) free(aframe_buffer);
 
        if (vframe_buffer) free(vframe_buffer);

        if (frameConverter) frameConverter->Release();
    }

    /**
     * @brief start output stream handle
     * 
     */
    void decklink_output::start()
    {
        output::start();
        abort_ = false;
        logger->info("decklink_output start, device_index={} pixel_format={} format_desc={}", decklink_index_, pixel_format_string_, format_desc_.name);
        /*
        decklink_thread_ = std::make_unique<std::thread>(std::thread([&](){
            while(!abort_)
            {
                HRESULT result;
                auto frm = get_video_frame();

                // convert frame format
                // sws_scale(sws_ctx_, (const uint8_t *const *)frm->data,
                //                         frm->linesize, 0, format_desc_.height, dst_frame_->data, dst_frame_->linesize);
                // convert uyvy422 10bit to decklink bgra 8 bit
               auto bgra_frame = ycbcr422_to_bgra(frm->data[0], format_desc_);

                // fill playback frame
                uint8_t* deckLinkBuffer = nullptr;
                if (playbackFrame_->GetBytes((void**)&deckLinkBuffer) != S_OK)
                {
                    logger->error("Could not get DeckLinkVideoFrame buffer pointer");
                    continue;
                }
                //deckLinkBuffer = reinterpret_cast<uint8_t*>(dst_frame_->data[0]);
                memset(deckLinkBuffer, 0, playbackFrame_->GetRowBytes() * playbackFrame_->GetHeight());
                memcpy(deckLinkBuffer, bgra_frame->begin(), playbackFrame_->GetRowBytes() * playbackFrame_->GetHeight());

                result = output_->DisplayVideoFrameSync(playbackFrame_);
                if (result != S_OK)
                {
                    logger->error("Unable to display video output");
                }

            }
        }));
        */
    }
        
    /**
     * @brief stop output stream handle
     * 
     */
    void decklink_output::do_stop()
    {
        abort_ = true;
        if (decklink_thread_ && decklink_thread_->joinable()) {
            decklink_thread_->join();
        }

	    output_->DisableVideoOutput();
        output_->DisableAudioOutput();
        logger->info("decklink_output device_index={} stopped", decklink_index_);
    }

    /**
     * @brief push a frame into this output stream buffer
     * 
     */
    void decklink_output::set_frame(std::shared_ptr<core::frame> frm)
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() >= frame_capacity_)
        {
            auto f = frame_buffer_[0];
            frame_buffer_.pop_front(); // discard the last frame
            logger->error("The frame be discarded");
        }
        frame_buffer_.push_back(frm);
        frame_cv_.notify_all();
    }

    /**
     * @brief Get a frame from this output stream buffer
     * 
     */
    std::shared_ptr<core::frame> decklink_output::get_frame()
    {
        std::shared_ptr<core::frame> frm;
        std::unique_lock<std::mutex> lock(frame_mutex_);
        frame_cv_.wait(lock, [this](){return !frame_buffer_.empty();});
        frm = frame_buffer_[0];
        frame_buffer_.pop_front();
        return frm;
    }

    /**
     * @brief push a video frame into this output stream buffer
     * 
     */
    void decklink_output::set_video_frame(std::shared_ptr<AVFrame> vframe)
    {
        std::lock_guard<std::mutex> lock(vframe_mutex_);
        if(vframe_buffer_.size() >= vframe_capacity_)
        {
            auto f = vframe_buffer_[0];
            frame_buffer_.pop_front(); // discard the last frame
            //logger->warn("The video frame be discarded");
        }
        vframe_buffer_.push_back(vframe);
        vframe_cv_.notify_all();
    }

    /**
     * @brief push a audio frame into this output stream buffer
     * 
     */
    void decklink_output::set_audio_frame(std::shared_ptr<AVFrame> aframe)
    {
        std::lock_guard<std::mutex> lock(aframe_mutex_);
        if(aframe_buffer_.size() >= aframe_capacity_)
        {
            auto f = aframe_buffer_[0];
            aframe_buffer_.pop_front(); // discard the last frame
            //logger->error("The audio frame be discarded");
        }
        aframe_buffer_.push_back(aframe);
        aframe_cv_.notify_all();
    }

    /**
     * @brief Get a video frame from this output stream buffer
     * 
     */
    std::shared_ptr<AVFrame> decklink_output::get_video_frame()
    {
        std::shared_ptr<AVFrame> vframe;
        std::unique_lock<std::mutex> lock(vframe_mutex_);
        vframe_cv_.wait(lock, [this](){return !vframe_buffer_.empty();});
        vframe = vframe_buffer_[0];
        vframe_buffer_.pop_front();
        return vframe;
    }

    /**
     * @brief Get a audio frame from this output stream buffer
     * 
     */
    std::shared_ptr<AVFrame> decklink_output::get_audio_frame()
    {
        std::shared_ptr<AVFrame> aframe;
        std::unique_lock<std::mutex> lock(aframe_mutex_);
        aframe_cv_.wait(lock, [this](){return !aframe_buffer_.empty();});
        aframe = aframe_buffer_[0];
        aframe_buffer_.pop_front();
        return aframe;
    }

    /**
     * @brief push a video frame into this output stream
     * 
     */
    void decklink_output::display_video_frame()
    {
        auto start = std::chrono::steady_clock::now();
        HRESULT result;
        ////////////do not need format convert
        // fill playback frame
        // uint8_t* deckLinkBuffer = nullptr;
        // if (playbackFrame_->GetBytes((void**)&yuvBuffer) != S_OK)
        // {
        //     logger->error("Could not get DeckLinkVideoFrame buffer pointer");
        // }
        // // // deckLinkBuffer = bgra_frame->begin();
        // memset(deckLinkBuffer, 0, playbackFrame_->GetRowBytes() * playbackFrame_->GetHeight());
        // memcpy(deckLinkBuffer, vframe, playbackFrame_->GetRowBytes() * playbackFrame_->GetHeight());

        ///////////bmdFormat10BitYUV to bmdFormat8BitBGRA
        // fill playback frame
        // auto memcpy_start = std::chrono::steady_clock::now();
        // uint8_t* yuvBuffer = nullptr;
        // if (yuv10Frame_->GetBytes((void**)&yuvBuffer) != S_OK)
        // {
        //     logger->error("Could not get DeckLinkVideoFrame buffer pointer");
        //     return;
        // }
        // memset(yuvBuffer, 0, yuv10Frame_->GetRowBytes() * yuv10Frame_->GetHeight());
        // memcpy(yuvBuffer, vframe, yuv10Frame_->GetRowBytes() * yuv10Frame_->GetHeight());
        // auto memcpy_end = std::chrono::steady_clock::now();
        // memcpy_us_sum += std::chrono::nanoseconds(memcpy_end - memcpy_start).count();

        if (pixel_format_ == bmdFormat10BitYUV) {
            result = output_->DisplayVideoFrameSync(yuv10Frame_);
        } else {
            
            if (frameConverter->ConvertFrame(yuv10Frame_, playbackFrame_) != S_OK)
            {
                logger->error("Could not get DeckLinkVideoFrame buffer pointer");
                return;
            }

            result = output_->DisplayVideoFrameSync(playbackFrame_);
            frame_convert_count++;
        }
        display_frame_count++;
        auto end = std::chrono::steady_clock::now();
        display_video_frame_us_sum += std::chrono::nanoseconds(end - start).count();

        
        if (result != S_OK)
        {
            if(result == E_FAIL)
            {
                logger->error("E_FAIL");
            }
            else if(result == E_INVALIDARG)
            {
                logger->error("E_INVALIDARG");
            }
            else if(result == E_ACCESSDENIED)
            {
                logger->error("E_ACCESSDENIED");
            }
            else if(result == E_OUTOFMEMORY)
            {
                logger->error("E_OUTOFMEMORY");
            }
            else 
            {
                logger->error("unknow");
            }
            logger->error("Unable to display video output");
        }
    }

    /**
     * @brief push a audio frame into this output stream
     * 
     */
    void decklink_output::display_audio_frame()
    {
        HRESULT result;
        uint32_t n;
        auto start = std::chrono::steady_clock::now();
        result = output_->WriteAudioSamplesSync(aframe_buffer, format_desc_.sample_num, &n);
        auto end = std::chrono::steady_clock::now();
        display_audio_frame_us_sum += std::chrono::nanoseconds(end - start).count();
        if (result != S_OK)
        {
            logger->error("Unable to display audio output");
        }
    }

    decklink_output_stat decklink_output::get_stat() {
        decklink_output_stat r {};
        r.frame_cnt = display_frame_count;
        // logger->info("decklink_output device_index={} display_frame_count={} frame_convert_count={} display_video_frame_us_sum={} memcpy_us_sum={} display_audio_frame_us_sum={}", 
        //     decklink_index_, display_frame_count, frame_convert_count, display_video_frame_us_sum, memcpy_us_sum, display_audio_frame_us_sum);
        display_frame_count = 0;
        display_video_frame_us_sum = 0;
        frame_convert_count = 0;
        memcpy_us_sum = 0;
        display_audio_frame_us_sum = 0;
        return r;
    }

    void decklink_output::consume_st_video_frame(void *frame, uint32_t width, uint32_t height) {
        uint8_t* yuvBuffer = nullptr;
        if (yuv10Frame_->GetBytes((void**)&yuvBuffer) != S_OK)
        {
            logger->error("Could not get DeckLinkVideoFrame buffer pointer");
            return;
        }
        st20_rfc4175_422be10_to_v210((st20_rfc4175_422_10_pg2_be*)frame, yuvBuffer, width, height);
    }

    void decklink_output::consume_st_audio_frame(void *frame, size_t frame_size) {
        memset(aframe_buffer, 0, frame_size);
        uint8_t *st_frame_data = reinterpret_cast<uint8_t *>(frame);
        if (sample_type_ == bmdAudioSampleType16bitInteger) {
            for (int i = 0; i < frame_size; i += 2) {
                aframe_buffer[i] = st_frame_data[i + 1];
                aframe_buffer[i + 1] = st_frame_data[i];
            }
        } else if (format_desc_.audio_samples == 24) {
            // frame (network byte order 24bits) to aframe_buffer (little byte order 32bits)
            int sample_count = frame_size / 3;
            for (int j = 0; j < sample_count; ++j) {
                aframe_buffer[j * 4 + 1] = st_frame_data[j * 3 + 2];
                aframe_buffer[j * 4 + 2] = st_frame_data[j * 3 + 1];
                aframe_buffer[j * 4 + 3] = st_frame_data[j * 3 + 0];
            }
        } else {
            memcpy(aframe_buffer, frame, frame_size);
        }
    }


    class PlayoutDelegate : public IDeckLinkVideoOutputCallback
    {
    public:
        explicit PlayoutDelegate(decklink_async_output::impl *_p_impl): p_impl(_p_impl) {}

        HRESULT	STDMETHODCALLTYPE QueryInterface (REFIID /*iid*/, LPVOID* /*ppv*/) {
            return E_NOTIMPL;
        }
        ULONG STDMETHODCALLTYPE AddRef () {
            return 1;
        }
        ULONG STDMETHODCALLTYPE Release () {
            return 1;
        }

        HRESULT	STDMETHODCALLTYPE ScheduledFrameCompleted(IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result);
        
        HRESULT	STDMETHODCALLTYPE ScheduledPlaybackHasStopped() {
            return S_OK;
        }
    private:
        decklink_async_output::impl *p_impl = nullptr;
    };


    class decklink_async_output::impl {
        friend class decklink_async_output;
        friend class PlayoutDelegate;

        impl(
            int device_id, 
            const core::video_format_desc &format_desc, 
            const std::string &pixel_format,
            int frame_buffer_count
        ):
            decklink_index_(device_id - 1), 
            format_desc_(format_desc),
            video_frame_queue(frame_buffer_count)
        {
            pixel_format_string_ = pixel_format;
            if (pixel_format == "bmdFormat10BitYUV") {
                pixel_format_ = bmdFormat10BitYUV;
            } else if (pixel_format == "bmdFormat8BitBGRA") {
                pixel_format_ = bmdFormat8BitBGRA;
            }

            HRESULT result;
            bmd_mode_ = get_decklink_video_format(format_desc_.format);
            video_flags_ = 0;
            
            // get the decklink device
            decklink_ = device_manager::instance().get_decklink(decklink_index_);
            if(decklink_ == nullptr)
            {
                logger->error("Failed to get DeckLink device. Make sure that the device ID is valid");
                throw std::runtime_error("Failed to get DeckLink device.");
            }

            IDeckLinkOutput *output = nullptr;
            // get output interface of the decklink device
            result = decklink_->QueryInterface(IID_IDeckLinkOutput, (void**)&output);
            if(result != S_OK)
            {
                logger->error("Failed to get DeckLink output interface.");
                throw std::runtime_error("Failed to get DeckLink output interface.");
            }
            output_ = boost::intrusive_ptr<IDeckLinkOutput>(output, false);

            IDeckLinkDisplayMode *display_mode = nullptr;
            // get decklink output mode
            result = output_->GetDisplayMode(bmd_mode_, &display_mode);
            display_mode_ = boost::intrusive_ptr<IDeckLinkDisplayMode>(display_mode, false);
            if(result != S_OK)
            {
                logger->error("Failed to get DeckLink output mode.");
                throw std::runtime_error("Failed to get DeckLink output mode.");
            }

            // enable video output
            result = output_->EnableVideoOutput(bmd_mode_, bmdVideoOutputFlagDefault);
            if (result != S_OK)
            {
                logger->error("Unable to enable video output");
                throw std::runtime_error("Unable to enable video output");
            }
            auto PerRow = ((display_mode_->GetWidth() + 47) / 48) * 128; //display_mode_->GetWidth() * 4;//bmdFormat8BitBGRA //((display_mode_->GetWidth() + 47) / 48) * 128; // bmdFormat10BitYUV
            for (int i = 0; i < frame_buffer_count; ++i) {
                IDeckLinkMutableVideoFrame *video_frame = nullptr;
                result = output_->CreateVideoFrame(
                    (int32_t)display_mode_->GetWidth(),
                    (int32_t)display_mode_->GetHeight(),
                    PerRow,
                    bmdFormat10BitYUV,
                    bmdFrameFlagDefault,
                    &video_frame);
                if(result != S_OK)
                {
                    logger->error("Unable to create video frame.");
                    throw std::runtime_error("Unable to create video frame.");
                }
                video_frames_.emplace_back(video_frame, false);
                video_frame_queue.producer_push(video_frame);
            }
            
            auto outputBytesPerRow = display_mode_->GetWidth() * 4; //display_mode_->GetWidth() * 4;//bmdFormat8BitBGRA //((display_mode_->GetWidth() + 47) / 48) * 128; // bmdFormat10BitYUV
            IDeckLinkMutableVideoFrame *playbackFrame = nullptr;
            result = output_->CreateVideoFrame(
                (int32_t)display_mode_->GetWidth(),
                (int32_t)display_mode_->GetHeight(),
                outputBytesPerRow,
                pixel_format_,
                bmdFrameFlagDefault,
                &playbackFrame);
            playbackFrame_ = boost::intrusive_ptr(playbackFrame, false);
                                        
            if(result != S_OK)
            {
                logger->error("Unable to create video frame.");
                throw std::runtime_error("Unable to create video frame.");
            }

            result = output_->EnableAudioOutput(format_desc.audio_sample_rate, format_desc.audio_samples, 
                                                format_desc.audio_channels, bmdAudioOutputStreamTimestamped);
            if (result != S_OK)
            {
                logger->error("Unable to enable video output");
                throw std::runtime_error("Unable to enable video output");
            }

            IDeckLinkVideoConversion *_frameConverter = CreateVideoConversionInstance();
            if (_frameConverter == nullptr)
            {
                logger->error("Unable to get Video Conversion interface");
                throw std::runtime_error("Unable to get Video Conversion interface");
            }
            frameConverter = boost::intrusive_ptr(_frameConverter, false);

            // create audio frame buffer
            aframe_buffer = std::unique_ptr<uint8_t[]>(new uint8_t[format_desc_.st30_frame_size]);

            result = display_mode_->GetFrameRate(&frame_duration, &frame_timescale);
            if (result != S_OK)
            {
                logger->error("Unable to GetFrameRate");
                throw std::runtime_error("Unable to GetFrameRate");
            }
            frame_per_second = (frame_timescale + (frame_duration - 1)) / frame_duration;
            audio_buffer_sample_length = (frame_per_second * audio_sample_rate * frame_duration) / frame_timescale;

            playout_delegate.reset(new PlayoutDelegate(this));

            result = output_->SetScheduledFrameCompletionCallback(playout_delegate.get());
            if (result != S_OK)
            {
                logger->error("Unable to SetScheduledFrameCompletionCallback");
                throw std::runtime_error("Unable to SetScheduledFrameCompletionCallback");
            }

            std::size_t vframe_buffer_size = format_desc_.height * ((display_mode_->GetWidth() + 47) / 48) * 128;
            vframe_buffer.reset(new uint8_t[vframe_buffer_size]);
        }

        int decklink_index_;
        core::video_format_desc format_desc_;
        BMDVideoInputFlags video_flags_;
        BMDPixelFormat pixel_format_ = bmdFormat10BitYUV;//bmdFormat8BitBGRA;bmdFormat10BitYUV
        std::string pixel_format_string_;
        BMDDisplayMode bmd_mode_;

        // IDeckLinkDisplayMode* display_mode_;
        // IDeckLink* decklink_;
        // IDeckLinkOutput* output_;
        // IDeckLinkVideoConversion* frameConverter = nullptr;
        boost::intrusive_ptr<IDeckLinkMutableVideoFrame> playbackFrame_;
        std::vector<boost::intrusive_ptr<IDeckLinkMutableVideoFrame>> video_frames_;
        frame_queue<IDeckLinkMutableVideoFrame *> video_frame_queue;
        std::unique_ptr<uint8_t[]> vframe_buffer;
        uint64_t total_playout_frames = 0;
        BMDTimeValue frame_duration;
        BMDTimeScale frame_timescale;
        uint32_t frame_per_second;

        boost::intrusive_ptr<IDeckLinkDisplayMode> display_mode_;
        IDeckLink *decklink_;
        boost::intrusive_ptr<IDeckLinkOutput> output_;
        boost::intrusive_ptr<IDeckLinkVideoConversion> frameConverter;

        int display_frame_count = 0;
        int frame_convert_count = 0;
        int scheduled_frame_completed_count = 0;
        uint64_t display_video_frame_us_sum = 0;
        uint64_t display_audio_frame_us_sum = 0;
        std::unique_ptr<uint8_t[]> aframe_buffer;
        uint64_t total_audio_scheduled = 0;
        
        uint32_t audio_buffer_sample_length;
        const BMDAudioSampleRate audio_sample_rate = bmdAudioSampleRate48kHz;

        std::unique_ptr<PlayoutDelegate> playout_delegate;
    };


    decklink_async_output::decklink_async_output(
        const std::string &output_id, 
        int device_id, 
        const core::video_format_desc &format_desc, 
        const std::string &pixel_format
    ):
        output(output_id), 
        p_impl(new impl(
            device_id, 
            format_desc, 
            pixel_format, 
            6 // frame buffer count
        ))
    {
        
    }

    decklink_async_output::~decklink_async_output() {
        stop();
    }

    void decklink_async_output::start() {
        output::start();
        HRESULT result;
        logger->info("decklink_async_output start, device_index={}", p_impl->decklink_index_);
        result = p_impl->output_->StartScheduledPlayback(0, p_impl->frame_timescale, 1.0);
        if (result != S_OK)
        {
            logger->error("Unable to StartScheduledPlayback");
        }
    }
    void decklink_async_output::do_stop() {
        p_impl->output_->StopScheduledPlayback(0, NULL, 0);
        p_impl->output_->DisableVideoOutput();
        p_impl->output_->DisableAudioOutput();
        p_impl->output_.detach()->Release(); // ??? wei sha zhe yang jiu ke yi ???
        logger->info("decklink_async_output device_index={} stopped", p_impl->decklink_index_);
    }

    void decklink_async_output::consume_st_video_frame(void *frame, uint32_t width, uint32_t height) {
        IDeckLinkMutableVideoFrame *video_frame = nullptr;
        uint8_t* yuvBuffer = nullptr;
        if (p_impl->video_frame_queue.producer_pop(video_frame)) {
            if (video_frame->GetBytes((void**)&yuvBuffer) != S_OK)
            {
                p_impl->video_frame_queue.producer_push(video_frame);
                logger->error("Could not get DeckLinkVideoFrame buffer pointer");
                return;
            }
            st20_rfc4175_422be10_to_v210((st20_rfc4175_422_10_pg2_be*)frame, yuvBuffer, width, height);
            p_impl->video_frame_queue.consumer_push(video_frame);
        }
    }
    
    void decklink_async_output::consume_st_audio_frame(void *frame, size_t frame_size) {
        memset(p_impl->aframe_buffer.get(), 0, frame_size);
        memcpy(p_impl->aframe_buffer.get(), frame, frame_size);
    }

    void decklink_async_output::display_video_frame() {
        IDeckLinkMutableVideoFrame *video_frame = nullptr;
        if (p_impl->video_frame_queue.consumer_pop(video_frame)) {
            auto start = std::chrono::steady_clock::now();
            HRESULT result;
            if (p_impl->pixel_format_ == bmdFormat10BitYUV) {
                result = p_impl->output_->ScheduleVideoFrame(
                    video_frame, 
                    p_impl->total_playout_frames * p_impl->frame_duration, 
                    p_impl->frame_duration, 
                    p_impl->frame_timescale
                );
            } else {
                
                if (p_impl->frameConverter->ConvertFrame(video_frame, p_impl->playbackFrame_.get()) != S_OK)
                {
                    logger->error("Could not get DeckLinkVideoFrame buffer pointer");
                    return;
                }

                result = p_impl->output_->ScheduleVideoFrame(
                    p_impl->playbackFrame_.get(), 
                    p_impl->total_playout_frames * p_impl->frame_duration, 
                    p_impl->frame_duration, 
                    p_impl->frame_timescale
                );

                p_impl->frame_convert_count++;
            }
            if (result != S_OK) {
                logger->error("ScheduleVideoFrame error: {}", result);
            }
            p_impl->total_playout_frames++;
            p_impl->display_frame_count++;
            auto end = std::chrono::steady_clock::now();
            p_impl->display_video_frame_us_sum += std::chrono::nanoseconds(end - start).count();
        }
    }

    void decklink_async_output::display_audio_frame() {
        HRESULT result;
        auto start = std::chrono::steady_clock::now();

        result = p_impl->output_->ScheduleAudioSamples(
            p_impl->aframe_buffer.get(), 
            p_impl->format_desc_.sample_num, 
            (p_impl->total_audio_scheduled * p_impl->audio_buffer_sample_length), 
            p_impl->audio_sample_rate,
            nullptr // when sampleFramesWritten is NULL, ScheduleAudioSamples will block until all audio samples are written to the scheduling buffer
        );

        auto end = std::chrono::steady_clock::now();
        p_impl->display_audio_frame_us_sum += std::chrono::nanoseconds(end - start).count();

        p_impl->total_audio_scheduled++;
        if (result != S_OK)
        {
            logger->error("Unable to display audio output");
        }
    }

    void decklink_async_output::dump_stat() {
        logger->info("decklink_async_output device_index={} display_frame_count={} frame_convert_count={} display_video_frame_us_sum={} scheduled_frame_completed_count={} display_audio_frame_us_sum={}", 
            p_impl->decklink_index_, p_impl->display_frame_count, p_impl->frame_convert_count, p_impl->display_video_frame_us_sum, p_impl->scheduled_frame_completed_count, p_impl->display_audio_frame_us_sum);

        p_impl->display_frame_count = 0;
        p_impl->display_video_frame_us_sum = 0;
        p_impl->frame_convert_count = 0;
        p_impl->scheduled_frame_completed_count = 0;
        p_impl->display_audio_frame_us_sum = 0;
    }

    HRESULT PlayoutDelegate::ScheduledFrameCompleted(IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result) {
        p_impl->video_frame_queue.producer_push(dynamic_cast<IDeckLinkMutableVideoFrame*>(completedFrame));
        p_impl->scheduled_frame_completed_count++;
        return S_OK;
    }

    void decklink_async_output::set_frame(std::shared_ptr<core::frame> frm) {}
    void decklink_async_output::set_video_frame(std::shared_ptr<AVFrame> vframe) {}
    void decklink_async_output::set_audio_frame(std::shared_ptr<AVFrame> aframe) {}
    std::shared_ptr<core::frame> decklink_async_output::get_frame() { return {}; }
    std::shared_ptr<AVFrame> decklink_async_output::get_video_frame() { return {}; }
    std::shared_ptr<AVFrame> decklink_async_output::get_audio_frame() { return {}; }
}