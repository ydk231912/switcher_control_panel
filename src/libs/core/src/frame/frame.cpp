/**
 * @file frame.h
 * @author 
 * @brief frame data struct
 * @version 1.0
 * @date 2022-07-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "core/frame/frame.h"

namespace seeder::core
{
    /**
     * @brief Construct a new frame::frame object
     * allocate the frame memory
     */
   frame::frame()
   {
   }

   void frame::set_video_data(AVFrame* avframe)
   {
      this->format = avframe->format;
      this->width = avframe->width;
      this->height = avframe->height;
      this->interlaced_frame = avframe->interlaced_frame;
      this->top_field_first  = avframe->top_field_first;
      this->key_frame        = avframe->key_frame;
      this->pts = avframe->pts;

      for(int i = 0; i < AV_NUM_DATA_POINTERS; i++)
      {
         this->video_data[i] = avframe->data[i];
         this->linesize[i] = avframe->linesize[i];
      }
   }

   void frame::set_audio_data(AVFrame* avframe)
   {
      //TODO
      
   }

    /**
     * @brief Destroy the frame::frame, free the frame memory
     * 
     */
   frame::~frame()
   {
   }

}