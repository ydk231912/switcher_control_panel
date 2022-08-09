
#include "core/util/png_writer.h"
#include "core/util/lodepng.h"
#include "core/util/logger.h"

namespace seeder::core
{
    /**
     * @brief write frame data to png file
     * 
     * @param data RGBA 8 bit
     * @param width 
     * @param height 
     * @param path 
     */
    void write_png(uint8_t* data, int width, int height, std::string path)
    {
        unsigned error = lodepng_encode32_file(path.c_str(), data, 
        width, height);
        if(error)
        {
            logger->error("write data to png file failed");
        }
    }
}