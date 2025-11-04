#pragma once
#include <stdexcept>
#include <string>
#include <zstd.h>

class ZstdCtx
{
public:
    enum class Mode
    {
        Compress,
        Decompress
    };

private:
    Mode mode;
    union
    {
        ZSTD_CCtx* cctx;
        ZSTD_DCtx* dctx;
    };

public:
    explicit ZstdCtx(Mode m, int compressionLevel = 6, int nbWorkers = 2) : mode(m)
    {
        if (mode == Mode::Compress)
        {
            cctx = ZSTD_createCCtx();
            if (!cctx)
                throw std::runtime_error("Failed to create ZSTD_CCtx");

            // 🔧 Compression tuning
            ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, compressionLevel); // level 6 = good balance
            ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, nbWorkers);               // 2 threads = best perf/watt
            ZSTD_CCtx_setParameter(cctx, ZSTD_c_windowLog, 23);                      // 8 MB window — fine for mixed files
            ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);                    // integrity check per frame
            ZSTD_CCtx_setParameter(cctx, ZSTD_c_contentSizeFlag, 1);                 // store uncompressed size
            ZSTD_CCtx_setParameter(cctx, ZSTD_c_dictIDFlag, 0);                      // no dict id
        }
        else
        {
            dctx = ZSTD_createDCtx();
            if (!dctx)
                throw std::runtime_error("Failed to create ZSTD_DCtx");

// ⚙️ Decompression tuning (safe across all Zstd versions)
#if defined(ZSTD_d_stableOutBuffer)
            ZSTD_DCtx_setParameter(dctx, ZSTD_d_stableOutBuffer, 1); // allow safe reuse of buffers
#endif
#if defined(ZSTD_d_format)
            ZSTD_DCtx_setParameter(dctx, ZSTD_d_format, ZSTD_f_zstd1); // enforce zstd frame format
#endif
#if defined(ZSTD_d_forceIgnoreChecksum)
            ZSTD_DCtx_setParameter(dctx, ZSTD_d_forceIgnoreChecksum, 0); // verify checksum
#endif
        }
    }

    ~ZstdCtx()
    {
        if (mode == Mode::Compress)
            ZSTD_freeCCtx(cctx);
        else
            ZSTD_freeDCtx(dctx);
    }

    ZSTD_CCtx* compressor() const
    {
        if (mode != Mode::Compress)
            throw std::logic_error("Not a compression context");
        return cctx;
    }

    ZSTD_DCtx* decompressor() const
    {
        if (mode != Mode::Decompress)
            throw std::logic_error("Not a decompression context");
        return dctx;
    }
};
