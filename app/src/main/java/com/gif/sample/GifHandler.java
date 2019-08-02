package com.gif.sample;

import android.graphics.Bitmap;

/**
 * @author: lixin
 * Date: 2019-08-02
 * Description:
 */
public class GifHandler {
    private long gifAddr;

    static {
        System.loadLibrary("native-lib");
    }

    public GifHandler(String path) {
        this.gifAddr = loadPath(path);
    }

    public int getWidth() {
        return getWidth(gifAddr);
    }

    public int getHeight() {
        return getHeight(gifAddr);
    }

    public int updateFrame(Bitmap bitmap) {
        return updateFrame(gifAddr, bitmap);
    }

    private native long loadPath(String path);

    private native int getWidth(long ndkGif);

    private native int getHeight(long ndkGif);

    public native int updateFrame(long ndkGif, Bitmap bitmap);
}
