#include <jni.h>
#include <string>
#include <cstdlib>
#include "gif_lib.h"
#include <android/log.h>
#include <android/bitmap.h>

#define LOG_TAG "GifPlayer >>> "
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define argb(a, r, g, b) (((a) & 0xff) << 24) | (((b) & 0xff) << 16) | (((g) & 0xff) << 8) | ((r) & 0xff)

typedef struct GifBean {
    // 当前帧数
    int currentFrame;
    // 总帧数
    int totalFrame;
    // 延迟时间数据
    int *delays;
} GifBean;

/**
 * 绘制一帧图片
 * @param gifFileType
 * @param gifBean
 * @param info
 * @param pixels
 */
void drawFrame(GifFileType *gifFileType, GifBean *gifBean, AndroidBitmapInfo info, void *pixels) {
    // 当前帧的图片
    SavedImage savedImage = gifFileType->SavedImages[gifBean->currentFrame];
    // 图片的首地址
    int *px = static_cast<int *>(pixels);
    int pointPixel;
    GifImageDesc frameInfo = savedImage.ImageDesc;
    // 压缩数据 LZW
    GifByteType gifByteType;
    // RGB工具
    ColorMapObject *colorMapObject = frameInfo.ColorMap;
    // Bitmap往下偏移
    px = reinterpret_cast<int *>((char *) px + info.stride + frameInfo.Top);
    // 每一行的首地址
    int *line;
    for (int y = frameInfo.Top; y < frameInfo.Top + frameInfo.Height; ++y) {
        line = px;
        for (int x = frameInfo.Left; x < frameInfo.Left + frameInfo.Width; ++x) {
            // 拿到每一个坐标的索引
            pointPixel = (y - frameInfo.Top) * frameInfo.Width + (x - frameInfo.Left);

            gifByteType = savedImage.RasterBits[pointPixel];
            GifColorType gifColorType = colorMapObject->Colors[gifByteType];
            line[x] = argb(255, gifColorType.Red, gifColorType.Green, gifColorType.Blue);
        }
        px = reinterpret_cast<int *>((char *) px + info.stride);
    }

}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_gif_sample_GifHandler_loadPath(JNIEnv *env, jobject instance, jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);

    int err;
    GifFileType *gifFileType = DGifOpenFileName(path, &err);
    DGifSlurp(gifFileType);
    // 给GIFBean分配内存
    GifBean *gifBean = static_cast<GifBean *>(malloc(sizeof(GifBean)));
    // 清空内存地址
    memset(gifBean, 0, sizeof(GifBean));
    gifFileType->UserData = gifBean;

    // 初始化数组
    gifBean->delays = static_cast<int *>(malloc(sizeof(int) * gifFileType->ImageCount));
    memset(gifBean->delays, 0, sizeof(int) * gifFileType->ImageCount);

    gifFileType->UserData = gifBean;
    gifBean->currentFrame = 0;
    gifBean->totalFrame = gifFileType->ImageCount;
    ExtensionBlock *ext;
    for (int i = 0; i < gifFileType->ImageCount; ++i) {
        SavedImage frame = gifFileType->SavedImages[i];
        for (int j = 0; j < frame.ExtensionBlockCount; ++j) {
            if (frame.ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE) {
                ext = &frame.ExtensionBlocks[j];
                break;
            }
        }
        if (ext) {
            int frameDelay = 10 * (ext->Bytes[1] | (ext->Bytes[2] << 8));
            LOGE("时间：%d", frameDelay);
            gifBean->delays[i] = frameDelay;
        }
    }
    LOGE("gif长度大小：%d", gifFileType->ImageCount);
    env->ReleaseStringUTFChars(path_, path);
    return reinterpret_cast<jlong>(gifFileType);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_gif_sample_GifHandler_getWidth__J(JNIEnv *env, jobject instance, jlong ndkGif) {

    GifFileType *gifFileType = reinterpret_cast<GifFileType *>(ndkGif);
    LOGE("宽度：%d", gifFileType->SWidth);
    return gifFileType->SWidth;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_gif_sample_GifHandler_getHeight__J(JNIEnv *env, jobject instance, jlong ndkGif) {

    GifFileType *gifFileType = reinterpret_cast<GifFileType *>(ndkGif);
    LOGE("高度：%d", gifFileType->SHeight);
    return gifFileType->SHeight;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_gif_sample_GifHandler_updateFrame__JLandroid_graphics_Bitmap_2(JNIEnv *env,
                                                                        jobject instance,
                                                                        jlong ndkGif,
                                                                        jobject bitmap) {

    GifFileType *gifFileType = reinterpret_cast<GifFileType *>(ndkGif);
    GifBean *gifBean = static_cast<GifBean *>(gifFileType->UserData);

    AndroidBitmapInfo info;
    // 像素数组
    AndroidBitmap_getInfo(env, bitmap, &info);

    void *pixels;
    AndroidBitmap_lockPixels(env, bitmap, &pixels);
    drawFrame(gifFileType, gifBean, info, pixels);
    gifBean->currentFrame += 1;
    if (gifBean->currentFrame >= gifBean->totalFrame - 1) {
        gifBean->currentFrame = 0;
        LOGE("重新播放：%d", gifBean->currentFrame);
    }
    AndroidBitmap_unlockPixels(env, bitmap);
    return gifBean->delays[gifBean->currentFrame];
}