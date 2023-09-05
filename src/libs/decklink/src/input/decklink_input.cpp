/**
 * @file decklink_input.cpp
 * @author 
 * @brief decklink stream input handle
 * @version 1.0
 * @date 2022-07-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <condition_variable>

#include <thread>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/buffer.h>
}

#include "decklink/input/decklink_input.h"
#include "core/util/logger.h"
#include "decklink/util.h"
#include "decklink/manager.h"

using namespace seeder::decklink::util;
using namespace seeder::core;
namespace seeder::decklink
{
    struct decklink_audio_producer {
        decklink_input *input;
        std::thread thread;
        std::mutex mutex;
        std::atomic<bool> stopped = false;
        std::condition_variable cv;
        std::deque<boost::intrusive_ptr<IDeckLinkAudioInputPacket>> audio_packet_queue;
        std::vector<std::shared_ptr<buffer>> audio_slice_buffer;
        const int max_queue_size = 80;

        explicit decklink_audio_producer(decklink_input *_input):
            input(_input) {
        }

        void start() {
            if (stopped) {
                return;
            }
            thread = std::thread([this] {
                while (!stopped) {
                    boost::intrusive_ptr<IDeckLinkAudioInputPacket> packet;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        if (audio_packet_queue.empty()) {
                            cv.wait(lock);
                            continue;
                        }
                        packet = std::move(audio_packet_queue.front());
                        audio_packet_queue.pop_front();
                    }
                    process_audio_packet(packet.get());
                }
            });
        }

        void push_packet(IDeckLinkAudioInputPacket *audio) {
            std::unique_lock<std::mutex> lock(mutex);
            if (audio_packet_queue.size() >= max_queue_size) {
                audio_packet_queue.pop_front();
            }
            audio_packet_queue.emplace_back(audio);
            cv.notify_all();
        }

        void process_audio_packet(IDeckLinkAudioInputPacket *audio) {
            //one 20ms/40ms audio frame slices into multiple 125us/1ms/4ms frames
            int nb = input->format_desc_.st30_fps / input->format_desc_.fps; // slice number
            auto size = input->format_desc_.st30_frame_size;
            void* audio_bytes = nullptr;
            // auto sample_frame_count = audio->GetSampleFrameCount(); sample_frame_count = 960
            // const bool push_batch = false;
            const bool push_batch = true;
            
            if(audio->GetBytes(&audio_bytes) == S_OK && audio_bytes) {
                for(int i = 0; i < nb; i++)
                {
                    auto aframe = std::make_shared<buffer>(input->format_desc_.st30_frame_size);
                    if (input->sample_type_ == bmdAudioSampleType16bitInteger) {
                        // memcpy(aframe->begin(), (char*) audio_bytes + i * size , size);
                        // for (int j = 0; j < size; j += 2) {
                        //     std::swap(aframe->begin()[j], aframe->begin()[j + 1]);
                        // }
                        for (int j = 0; j < size; j += 2) {
                            char *b = reinterpret_cast<char*>(audio_bytes) + i * size;
                            aframe->begin()[j] = b[j + 1];
                            aframe->begin()[j + 1] = b[j];
                        }
                    } else if (input->format_desc_.audio_samples == 24) {
                        const int sample_count = size / 3;
                        const int src_slice_stride = (input->sample_type_ / 8) * sample_count;
                        for (int j = 0; j < sample_count; ++j) {
                            char *b = reinterpret_cast<char*>(audio_bytes) + i * src_slice_stride + j * 4;
                            aframe->begin()[j * 3] = b[3];
                            aframe->begin()[j * 3 + 1] = b[2];
                            aframe->begin()[j * 3 + 2] = b[1];
                        }
                    } else {
                        memcpy(aframe->begin(), (char*) audio_bytes + i * size , size);
                    }
                    if (push_batch) {
                        audio_slice_buffer.push_back(aframe);
                    } else {
                        input->set_audio_frame_slice(aframe);
                    }
                }
                if (push_batch) {
                    input->set_audio_frame_slice(audio_slice_buffer);
                    audio_slice_buffer.clear();
                }
            }
        }

        void stop() {
            if (stopped) {
                return;
            }
            stopped = true;
            cv.notify_all();
            thread.join();
        }
    };

    /**
     * @brief Construct a new decklink input object.
     * initialize deckllink device and start input stream
     */
    decklink_input::decklink_input(const std::string &source_id, int device_id, video_format_desc& format_desc)
    :input(source_id),
    decklink_index_(device_id - 1)
    ,format_desc_(format_desc)
    ,video_flags_(bmdVideoInputFlagDefault)
    ,pixel_format_(bmdFormat10BitYUV) //bmdFormat8BitYUV
    {
        sample_type_ = bmdAudioSampleType16bitInteger; //bmdAudioSampleType32bitInteger;
        if(format_desc_.audio_samples > 16)
            sample_type_ = bmdAudioSampleType32bitInteger;
        

        HRESULT result;
        bmd_mode_ = get_decklink_video_format(format_desc_.format);
        
        // get the decklink device
        decklink_ = device_manager::instance().get_decklink(decklink_index_);
        if(decklink_ == nullptr)
        {
            logger->error("Failed to get DeckLink device. Make sure that the device ID is valid");
            throw std::runtime_error("Failed to get DeckLink device.");
        }

        // get input interface of the decklink device
        result = decklink_->QueryInterface(IID_IDeckLinkInput, (void**)&input_);
        if(result != S_OK)
        {
            logger->error("Failed to get DeckLink input interface.");
            throw std::runtime_error("Failed to get DeckLink input interface.");
        }

        // get decklink input mode
        result =  input_->GetDisplayMode(bmd_mode_, &display_mode_);
        if(result != S_OK)
        {
           logger->error("Failed to get DeckLink input mode.");
           throw std::runtime_error("Failed to get DeckLink input mode.");
        }

        // set the capture callback
        result = input_->SetCallback(this);
        if(result != S_OK)
        {
            logger->error("Failed to set DeckLink input callback.");
            throw std::runtime_error("Failed to set DeckLink input callback.");
        }

        audio_producer = std::make_unique<decklink_audio_producer>(this);
    }

    decklink_input::~decklink_input()
    {
        stop();

        if(display_mode_ != nullptr)
            display_mode_->Release();

        if(input_ != nullptr)
            input_->Release();
    }

    /**
     * @brief start input stream handle
     * 
     */
    void decklink_input::start()
    {
        input::start();
        audio_producer->start();
        // enable input and start streams
        HRESULT result = input_->EnableVideoInput(bmd_mode_, pixel_format_, video_flags_);
        if(result != S_OK)
        {
            ///// for debug 
            // if(result == E_FAIL)
            // {
            //     logger->error("E_FAIL");
            // }
            // else if(result == E_INVALIDARG)
            // {
            //     logger->error("E_INVALIDARG");
            // }
            // else if(result == E_ACCESSDENIED)
            // {
            //     logger->error("E_ACCESSDENIED");
            // }
            // else if(result == E_OUTOFMEMORY)
            // {
            //     logger->error("E_OUTOFMEMORY");
            // }
            // else 
            // {
            //     logger->error("unknow");
            // }

            logger->error("Failed to enable DeckLink video input. result={:#x} decklink_index={}", result, decklink_index_);
            throw std::runtime_error("Failed to enable DeckLink video input.");
        }
        // TODO parameters from config file
        result = input_->EnableAudioInput(bmdAudioSampleRate48kHz, sample_type_, format_desc_.audio_channels);
        if(result != S_OK)
        {
            logger->error("Failed to enable DeckLink audio input. {}", result);
            throw std::runtime_error("Failed to enable DeckLink audio input.");
        }

        result = input_->StartStreams();
        if(result != S_OK)
        {
            logger->error("Failed to start DeckLink input streams.");
            throw std::runtime_error("Failed to start DeckLink input streams.");
        }

        logger->info("decklink_input start device_index={}", decklink_index_);
    }

    /**
     * @brief stop input stream handle
     * 
     */
    void decklink_input::do_stop()
    {
        if(input_ != nullptr)
        {
            HRESULT result;
            result = input_->StopStreams();
            if (FAILED(result)) {
                logger->error("decklink_input::stop StopStreams error: {}", result);
            }
            result = input_->DisableAudioInput();
            if (FAILED(result)) {
                logger->error("decklink_input::stop DisableAudioInput error: {}", result);
            }
            result = input_->DisableVideoInput();
            if (FAILED(result)) {
                logger->error("decklink_input::stop DisableVideoInput error: {}", result);
            }
        }
        audio_producer->stop();
        audio_producer.reset();
        logger->info("decklink_input stop device_index={}", decklink_index_);
    }

    ULONG decklink_input::AddRef(void)
    {
        return 1;
    }

    ULONG decklink_input::Release(void)
    {
        return 1;
    }

    /**
     * @brief change display mode and restart input streams
     * 
     * @param events 
     * @param mode 
     * @param format_flags 
     * @return HRESULT 
     */
    HRESULT decklink_input::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, 
                                                        IDeckLinkDisplayMode* mode, 
                                                        BMDDetectedVideoInputFormatFlags format_flags)
    {
        display_mode_ = mode;
        input_->StopStreams();
        input_->EnableVideoInput(mode->GetDisplayMode(), pixel_format_, video_flags_);
        input_->EnableAudioInput(bmdAudioSampleRate48kHz, sample_type_, format_desc_.audio_channels);
        input_->FlushStreams();
        input_->StartStreams();
        
        return S_OK;
    }

    /**
     * @brief handle arrived frame from decklink card. 
     * push frame into the buffer, if the buffer is full, discard the old frame
     * 
     * @param video_frame 
     * @param audio_frame 
     * @return HRESULT 
     */
    HRESULT decklink_input::VideoInputFrameArrived(IDeckLinkVideoInputFrame* video, 
                                                       IDeckLinkAudioInputPacket* audio)
    {
        BMDTimeValue in_video_pts = 0LL;
        // BMDTimeValue in_audio_pts = 0LL;
        auto frm = std::make_shared<core::frame>(); 

        if(audio && audio_producer) {
            audio_producer->push_packet(audio);
        }

        if(video)
        {
            bool is_frame_valid = (video->GetFlags() & bmdFrameHasNoInputSource) == 0;
            void* video_bytes = nullptr;
            if(video->GetBytes(&video_bytes) == S_OK && video_bytes)
            {
                // decklink 接收i信号时，接收到的帧率会变为一半，1080i50实际收到的帧率只有25
                for (int i = 0; i < format_desc_.field_count; ++i) {
                    video->AddRef();
                    auto vframe = std::shared_ptr<AVFrame>(av_frame_alloc(), [video](AVFrame* ptr) { 
                        av_frame_free(&ptr);
                        video->Release(); 
                    });
                    vframe->format = AV_PIX_FMT_UYVY422;
                    vframe->width = video->GetWidth();
                    vframe->height = video->GetHeight();
                    vframe->interlaced_frame = display_mode_->GetFieldDominance() != bmdProgressiveFrame;
                    vframe->top_field_first  = display_mode_->GetFieldDominance() == bmdUpperFieldFirst ? 1 : 0;
                    vframe->key_frame        = 1;

                    vframe->data[0] = reinterpret_cast<uint8_t*>(video_bytes);
                    vframe->linesize[0] = video->GetRowBytes();

                    BMDTimeValue duration;
                    if (video->GetStreamTime(&in_video_pts, &duration, AV_TIME_BASE)) 
                    {
                        vframe->pts = in_video_pts; //need bugging to ditermine the in_video_pts meets the requirement
                    }
                    vframe->opaque = reinterpret_cast<void*>(i);
                    this->set_video_frame(vframe);
                }
            }
            //frm->video = vframe;
            receive_frame_stat++;
            if (!is_frame_valid) {
                invalid_frame_stat++;
                has_signal = false;
            } else {
                has_signal = true;
            }
        }
        return S_OK;
    }

    /**
     * @brief push the frame into the stream buffer,
     * if the buffer is full, discard the oldest frame
     */
    void decklink_input::set_frame(std::shared_ptr<frame> frm)
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() >= frame_capacity_)
        {
            auto f = frame_buffer_[0];
            frame_buffer_.pop_front(); // discard the oldest frame
            //logger->error("The frame is discarded");
        }
        frame_buffer_.push_back(frm);
        frame_cv_.notify_all();
    }

    /**
     * @brief push the video frame into the stream buffer,
     * if the buffer is full, discard the oldest frame
     */
    void decklink_input::set_video_frame(std::shared_ptr<AVFrame> vframe)
    {
        std::lock_guard<std::mutex> lock(vframe_mutex_);
        if(vframe_buffer_.size() >= vframe_capacity_)
        {
            auto f = vframe_buffer_[0];
            vframe_buffer_.pop_front(); // discard the oldest frame
            //logger->warn("{}, The video frame is discarded, Decklink {}", __func__, decklink_index_ + 1);
        }
        vframe_buffer_.push_back(vframe);
        vframe_cv_.notify_all();
    }

    /**
     * @brief push the audio frame into the stream buffer,
     * if the buffer is full, discard the oldest frame
     */
    void decklink_input::set_audio_frame(std::shared_ptr<AVFrame> aframe)
    {
        std::lock_guard<std::mutex> lock(aframe_mutex_);
        if(aframe_buffer_.size() >= aframe_capacity_)
        {
            auto f = aframe_buffer_[0];
            aframe_buffer_.pop_front(); // discard the oldest frame
            //logger->error("{}, The audio frame is discarded", __func__);
        }
        aframe_buffer_.push_back(aframe);
        aframe_cv_.notify_all();
    }

    void decklink_input::set_audio_frame_slice(const std::vector<std::shared_ptr<buffer>> &frames) {
        std::lock_guard<std::mutex> lock(asframe_mutex_);
        for (auto &aframe : frames) {
            if (asframe_buffer_.size() >= aframe_capacity_) {
                asframe_buffer_.pop_front(); // discard the oldest frame
            }
            asframe_buffer_.push_back(aframe);
        }
        asframe_cv_.notify_all();
    }

    // /**
    //  * @brief Get a frame from this input stream
    //  * 
    //  */
    std::shared_ptr<frame> decklink_input::get_frame()
    {
        std::shared_ptr<core::frame> frm;
        std::unique_lock<std::mutex> lock(frame_mutex_);
        if(frame_buffer_.size() > 0)
        {
            frm = frame_buffer_[0];
            frame_buffer_.pop_front();
            return frm;
        }
        frame_cv_.wait(lock, [this](){return !(frame_buffer_.empty());}); // block until the buffer is not empty
        frm = frame_buffer_[0];
        frame_buffer_.pop_front();
        return frm;

        // std::shared_ptr<frame> frm;
        // {
        //     std::lock_guard<std::mutex> lock(frame_mutex_);
        //     if(frame_buffer_.size() < 1)
        //         return last_frame_;

        //     frm = frame_buffer_[0];
        //     frame_buffer_.pop_front();
        //     last_frame_ = frm;
        // }

        // return frm;
    }

    /**
     * @brief Get a video frame from this input stream
     * 
     */
    std::shared_ptr<AVFrame> decklink_input::get_video_frame()
    {
        std::shared_ptr<AVFrame> vframe;
        std::unique_lock<std::mutex> lock(vframe_mutex_);
        if(vframe_buffer_.size() > 0)
        {
            vframe = vframe_buffer_[0];
            vframe_buffer_.pop_front();
            return vframe;
        }
        vframe_cv_.wait(lock, [this](){return !(vframe_buffer_.empty());}); // block until the buffer is not empty
        vframe = vframe_buffer_[0];
        vframe_buffer_.pop_front();
        return vframe;
    }

    /**
     * @brief Get a audio frame from this input stream
     * 
     */
    std::shared_ptr<AVFrame> decklink_input::get_audio_frame()
    {
        std::shared_ptr<AVFrame> aframe;
        std::unique_lock<std::mutex> lock(aframe_mutex_);
        if(aframe_buffer_.size() > 0)
        {
            aframe = aframe_buffer_[0];
            aframe_buffer_.pop_front();
            return aframe;
        }
        aframe_cv_.wait(lock, [this](){return !(aframe_buffer_.empty());}); // block until the buffer is not empty
        aframe = aframe_buffer_[0];
        aframe_buffer_.pop_front();
        return aframe;
    }

     /**
     * @brief push the audio frame into the stream buffer,
     * if the buffer is full, discard the oldest frame
     */
    void decklink_input::set_audio_frame_slice(std::shared_ptr<buffer> asframe)
    {
        std::lock_guard<std::mutex> lock(asframe_mutex_);
        if(asframe_buffer_.size() >= asframe_capacity_)
        {
            auto f = asframe_buffer_[0];
            asframe_buffer_.pop_front(); // discard the oldest frame
            logger->trace("{}, The audio frame is discarded", __func__);
        }
        asframe_buffer_.push_back(asframe);
        asframe_cv_.notify_all();
    }

     /**
     * @brief Get a audio frame slice from this input stream
     * 
     */
    std::shared_ptr<buffer> decklink_input::get_audio_frame_slice()
    {
        std::shared_ptr<buffer> asframe;
        std::unique_lock<std::mutex> lock(asframe_mutex_);
        if(asframe_buffer_.size() > 0)
        {
            asframe = asframe_buffer_[0];
            asframe_buffer_.pop_front();
            return asframe;
        }
        asframe_cv_.wait(lock, [this](){return !(asframe_buffer_.empty());}); // block until the buffer is not empty
        asframe = asframe_buffer_[0];
        asframe_buffer_.pop_front();
        return asframe;
    }

    decklink_input_stat decklink_input::get_stat() {
        decklink_input_stat r {};
        r.frame_cnt = receive_frame_stat;
        r.invalid_frame_cnt = invalid_frame_stat;

        receive_frame_stat = 0;
        invalid_frame_stat = 0;
        return r;
    }
}