#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
}

int main() {
    const AVCodec *codec = nullptr;
    void *iter = nullptr;
    std::cout << "Available Audio Encoders:" << std::endl;
    while ((codec = av_codec_iterate(&iter))) {
        if (av_codec_is_encoder(codec) && codec->type == AVMEDIA_TYPE_AUDIO) {
            std::cout << " - " << codec->name << " (" << codec->long_name << ")" << std::endl;
        }
    }
    return 0;
}
