#include <fcntl.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
#include <libavutil/pixdesc.h>
#include <va/va.h>
#include <va/va_drm.h>
#include <va/va_drmcommon.h>
}


int main(int argc, char *argv[]) {
  const char *filename = "../planet.mp4";
  FILE *fout = fopen("out.raw", "wb");

  // ---------------------------------
  // find video stream information
  // set up decode context
  // ---------------------------------
  AVInputFormat *input_format = NULL;
  AVFormatContext *input_ctx = NULL;
  avformat_open_input(&input_ctx, filename, input_format, NULL);
  const AVCodec *codec = nullptr;

  int video_stream =
      av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);

  AVCodecParameters *codecpar = input_ctx->streams[video_stream]->codecpar;

  AVCodecContext *decoder_ctx = NULL;
  AVBufferRef *hw_device_ctx = NULL;
  decoder_ctx = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(decoder_ctx, codecpar);


  // ---------------------------------
  // Manually initialize decode context 
  // so that va_display is available later
  // for the vaapi->dmabuf conversion
  // ---------------------------------
  decoder_ctx->pix_fmt = AV_PIX_FMT_VAAPI; 
  const char *device = "/dev/dri/renderD128"; 

  VADisplay va_display = 0;
  int drm_fd = open(device, O_RDWR);
  va_display = vaGetDisplayDRM(drm_fd);
  int major, minor;
  vaInitialize(va_display, &major, &minor);

  hw_device_ctx = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VAAPI);

  AVHWDeviceContext *hwctx = (AVHWDeviceContext *)hw_device_ctx->data;
  AVVAAPIDeviceContext *vactx = (AVVAAPIDeviceContext *)hwctx->hwctx;
  vactx->display = va_display;
  av_hwdevice_ctx_init(hw_device_ctx);
  decoder_ctx->hw_device_ctx = hw_device_ctx;

  avcodec_open2(decoder_ctx, codec, NULL);


  // ---------------------------------
  //          MAIN LOOP
  // ---------------------------------
  int frame_num = 0;

  for (;;) {

    // Read packet with compressed video frame
    AVPacket *avpacket = av_packet_alloc();

    if (av_read_frame(input_ctx, avpacket) < 0) {
      av_packet_free(&avpacket); // End of stream or error. Send NULL to
      // avcodec_send_packet once to flush decoder
    } else if (avpacket->stream_index != video_stream) {
      av_packet_free(&avpacket); // Non-video (ex, audio) packet
      continue;
    }

    // Send packet to decoder
    avcodec_send_packet(decoder_ctx, avpacket);

    //----------------------------------------------
    // Receive frame(s) from decoder
    //----------------------------------------------
    while (true) {
      auto av_frame = av_frame_alloc();
      int decode_err = avcodec_receive_frame(decoder_ctx, av_frame);

      if (decode_err == AVERROR(EAGAIN) || decode_err == AVERROR_EOF) {
        break;
      }

      printf("Frame %d ", frame_num++);

      if (av_frame->format != AV_PIX_FMT_VAAPI)
        throw std::runtime_error("Unsupported av_frame format");

      VASurfaceID va_surface =
          (VASurfaceID)(size_t)
              av_frame->data[3]; // As defined by AV_PIX_FMT_VAAPI

      printf("va_surface %d\n", va_surface);

      //------------------------------------------------
      // dump NV12 output (debug only)
      //------------------------------------------------
      VAImage image;
      unsigned char *Y, *UV;

      void *buffer;
      vaDeriveImage(va_display, va_surface, &image);
      vaMapBuffer(va_display, image.buf, &buffer);

      /* NV12 */
      Y = (unsigned char *)buffer + image.offsets[0];
      UV = (unsigned char *)buffer + image.offsets[1];

      for (int r = 0; r < image.height; r++) {
        fwrite(Y, image.width, 1, fout);
        Y += image.pitches[0];
      }
      for (int r = 0; r < image.height / 2; r++) {
        fwrite(UV, image.width, 1, fout);
        UV += image.pitches[1];
      }
      vaUnmapBuffer(va_display, image.buf);
      vaDestroyImage(va_display, image.image_id);

      av_frame_free(&av_frame);
    }

    if (avpacket)
      av_packet_free(&avpacket);

    else
      break; // End of stream
  }

  avformat_close_input(&input_ctx);
  avcodec_free_context(&decoder_ctx);
  fclose(fout);
  vaTerminate(va_display);
  close(drm_fd);
  return 0;
}
