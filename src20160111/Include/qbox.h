/*******************************************************************************
*
* The content of this file or document is CONFIDENTIAL and PROPRIETARY
* to Mobilygen Corporation.  It is subject to the terms of a
* License Agreement between Licensee and Mobilygen Corporation.
* restricting among other things, the use, reproduction, distribution
* and transfer.  Each of the embodiments, including this information and
* any derivative work shall retain this copyright notice.
*
* Copyright 2009 Mobilygen Corporation.
* All rights reserved.
*
* QuArc is a registered trademark of Mobilygen Corporation.
*
* Version: SDK5r20450
*
*******************************************************************************/


#ifndef __QBOX_HH
#define __QBOX_HH

/* QBOX declarations */
// QBox: Quarc specific Box

#define QBOX_VERSION_NUM 1

//////////////////////////////////////////////////////////////////////////////
// version: 0
//
// SDL
// aligned(8) class QBox extends FullBox('qbox', version = 0, boxflags) {
//     unsigned short sample_stream_type;
//     unsigned short sample_stream_id;
//     unsigned long sample_flags;
//     unsigned long sample_cts;
//     unsigned char sample_data[];
// }
//
// equivalent to 
// typedef struct {
//     unsigned long box_size;
//     unsigned long box_type; // "qbox"
//     unsigned long box_flags; // (version << 24 | boxflags)
//     unsigned short sample_stream_type;
//     unsigned short sample_stream_id;
//     unsigned long sample_flags;
//     unsigned long sample_cts;
//     unsigned char sample_data[];
// } QBox;
//
// version 0 does not use large box
//
// box_flags
// 31 - 24         23 - 0
// version         boxflags
//
// boxflags
// 0x01 sample_data present after box header.
// otherwise sample_data contain four bytes address and four byte size info for
// the actual data.
// 0x02 this is the last sample
// 0x04 next qbox is word aligned
// 0x08 audio only
// 0x10 video only
// 0x20 stuffing packet (i.e. no meaningful data included)
//
// sample_stream_type:
// 0x01 AAC audio. sample_data contain audio frame or configuration info.
// 0x02 H.264 video. sample_data contain video frame or configuration info.
// It consists of 4 bytes length, NAL pairs and possible padding of 0s at the 
// end to make sample size word aligned.
// 0x05 H.264 video. sample_data contain video slice or configuration info.
// 0x06 MP1 audio. sample_data contain audio frame.
// 0x09 G.711 audio. sample_data contains one audio frame.
//
// sample_stream_id:
// 0, 1, ... or just the stream type
//
// sample_flags:
// 0x01 configuration info. sample_data contain configuration info.
// 0x02 cts present. 90 kHz cts present.
// 0x04 sync point. ex. I frame.
// 0x08 disposable. ex. B frame.
// 0x10 mute. Sample is mute/black.
// 0x20 cts base increment. By 2^32.
// 0x40 QBoxMeta present before configuration info or sample data.
// 0x80 sample contain end of sequence NALU.
// 0x100 sample contain end of stream NALU.
// 0x200 qmed
// 0xFF000000 padding mask, sample_data contain paddings "in front".
// sample_size include padding.
//
//////////////////////////////////////////////////////////////////////////////
#define int8_t   char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
#define int32_t  int
#define uint32_t unsigned int
#define int64_t  long long
#define uint64_t unsigned long long
#define ptr_t uint32_t

#define QBOX_FLAGS_SAMPLE_DATA_PRESENT 0x1
#define QBOX_FLAGS_LAST_SAMPLE 0x2
#define QBOX_FLAGS_PADDING_4 0x4
#define QBOX_FLAGS_AUDIO_ONLY 0x8
#define QBOX_FLAGS_VIDEO_ONLY 0x10
#define QBOX_FLAGS_STUFFING_PACKET 0x20

#define QBOX_SAMPLE_TYPE_AAC 0x1
#define QBOX_SAMPLE_TYPE_QAC 0x1
#define QBOX_SAMPLE_TYPE_H264 0x2
#define QBOX_SAMPLE_TYPE_QPCM 0x3
#define QBOX_SAMPLE_TYPE_DEBUG 0x4
#define QBOX_SAMPLE_TYPE_H264_SLICE 0x5
#define QBOX_SAMPLE_TYPE_QMA 0x6
#define QBOX_SAMPLE_TYPE_VIN_STATS_GLOBAL 0x7
#define QBOX_SAMPLE_TYPE_VIN_STATS_MB 0x8
#define QBOX_SAMPLE_TYPE_Q711 0x9
#define QBOX_SAMPLE_TYPE_Q722 0xa
#define QBOX_SAMPLE_TYPE_Q726 0xb
#define QBOX_SAMPLE_TYPE_Q728 0xc
#define QBOX_SAMPLE_TYPE_JPEG 0xd
#define QBOX_SAMPLE_TYPE_MPEG2_ELEMENTARY 0xe
#define QBOX_SAMPLE_TYPE_MAX 0xf

#define QBOX_SAMPLE_FLAGS_CONFIGURATION_INFO 0x01
#define QBOX_SAMPLE_FLAGS_CTS_PRESENT 0x02
#define QBOX_SAMPLE_FLAGS_SYNC_POINT 0x04
#define QBOX_SAMPLE_FLAGS_DISPOSABLE 0x08
#define QBOX_SAMPLE_FLAGS_MUTE 0x10
#define QBOX_SAMPLE_FLAGS_BASE_CTS_INCREMENT 0x20
#define QBOX_SAMPLE_FLAGS_META_INFO 0x40
#define QBOX_SAMPLE_FLAGS_END_OF_SEQUENCE 0x80
#define QBOX_SAMPLE_FLAGS_END_OF_STREAM 0x100
#define QBOX_SAMPLE_FLAGS_QMED_PRESENT 0x200
#define QBOX_SAMPLE_FLAGS_PKT_HEADER_LOSS   0x400
#define QBOX_SAMPLE_FLAGS_PKT_LOSS          0x800
#define QBOX_SAMPLE_FLAGS_PADDING_MASK 0xFF000000

#define QBOX_VERSION(box_flags)                                         \
    ((box_flags) >> 24)
#define QBOX_BOXFLAGS(box_flags)                                        \
    (((box_flags) << 8) >> 8)
#define QBOX_FLAGS(v, f)                                                \
    (((v) << 24) | (f))
#define QBOX_SAMPLE_PADDING(sample_flags)                               \
    (((sample_flags) & QBOX_SAMPLE_FLAGS_PADDING_MASK) >> 24)
#define QBOX_SAMPLE_FLAGS_PADDING(sample_flags, padding)                \
    ((sample_flags) | ((padding) << 24))

typedef struct
{
    unsigned int v : 8;
    unsigned int f : 24;
} QBoxFlag;

typedef struct
{
    unsigned int samplerate;
    unsigned int samplesize;
    unsigned int channels;
} QBoxMetaA;

typedef struct
{
    unsigned int width;
    unsigned int height;
    unsigned int gop;
    unsigned int frameticks;
} QBoxMetaV;

typedef union
{
    QBoxMetaA a;
    QBoxMetaV v;
} QBoxMeta;

typedef struct
{
    unsigned int addr;
    unsigned int size;
} QBoxSample;

//////////////////////////////////////////////////////////////////////////////
// version: 1
//
// 64 bits cts support

typedef struct
{
    unsigned int ctslow;
    unsigned int addr;
    unsigned int size;
} QBoxSample1;

#ifdef    __cplusplus
/* - since qbox is to be used in streaming, data are stored in big endian.
   the calss is endinness aware.
   - cast QBox to memory to read/write its member.
*/
class QBox
{
  public:
    unsigned int HeaderSize();
    unsigned int BoxSize();
    unsigned int BoxType();
    unsigned int BoxFlags();
    unsigned char Version();
    unsigned int Flags();
    unsigned short SampleStreamType();
    unsigned short SampleStreamId();
    unsigned int SampleFlags();
    unsigned int SampleCTS();
    uint64_t SampleCTS64();
    unsigned int SampleAddr();
    unsigned int SampleSize();
    void BoxSize(unsigned int size);
    void BoxType(unsigned int type);
    void BoxFlags(unsigned int flags);
    void Version(unsigned char v);
    void Flags(unsigned int f);
    void SampleStreamType(unsigned short type);
    void SampleStreamId(unsigned short id);
    void SampleFlags(unsigned int flags);
    void SampleCTS(unsigned int cts);
    void SampleCTS64(uint64_t cts);
    void SampleAddr(unsigned int addr);
    void SampleSize(unsigned int size);

    void Dump();

  protected:
    unsigned int mBoxSize;
    unsigned int mBoxType; // "qbox"
    union {
        unsigned int value; // (version << 24 | boxflags)
        QBoxFlag field;
    } mBoxFlags;
    unsigned short mSampleStreamType;
    unsigned short mSampleStreamId;
    unsigned int mSampleFlags;
    unsigned int mSampleCTS;
    union {
        unsigned char* data;
        QBoxSample info;
        QBoxSample1 info1;
    } mSample;
};

inline unsigned int str2int(const char* s) {
    return (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3];
}
inline void int2str(unsigned int i, char* s) {
    s[0] = (char)((i >> 24) & 0xFF); s[1] = (char)((i >> 16) & 0xFF); 
    s[2] = (char)((i >> 8) & 0xFF); s[3] = (char)(i & 0xFF); s[4] = 0;
}
// This is for version 0 only, obselete
inline unsigned int HeaderSize(unsigned int flags)
{
    return (flags & QBOX_FLAGS_SAMPLE_DATA_PRESENT) ? 
        sizeof(QBox) - sizeof(QBoxSample1) : sizeof(QBox)- sizeof(unsigned int);
}
inline unsigned int HeaderSize(unsigned int flags, unsigned int version)
{
    if (version == 0)
        return (flags & QBOX_FLAGS_SAMPLE_DATA_PRESENT) ? 
            sizeof(QBox) - sizeof(QBoxSample1) : sizeof(QBox) - sizeof(unsigned int);
    else
        return (flags & QBOX_FLAGS_SAMPLE_DATA_PRESENT) ? 
            sizeof(QBox) - sizeof(QBoxSample) : sizeof(QBox);
}

#ifdef _WIN32
#define LITTLE_ENDIAN 0
#define BIG_ENDIAN    1
#define __BYTE_ORDER LITTLE_ENDIAN
#else
#include <endian.h>
#endif

#define BE8(a) (a)
#if __BYTE_ORDER == BIG_ENDIAN
#define BE16(a) (a)
#define BE24(a) (a)
#define BE32(a) (a)
#define BE64(a) (a)
#else
#define BE16(a)                                                             \
    ((((a)>>8)&0xFF) |                                                      \
    (((a)<<8)&0xFF00))
#define BE24(a)                                                             \
    ((((a)>>16)&0xFF) |                                                     \
    ((a)&0xFF00) |                                                          \
    (((a)<<16)&0xFF0000))
#define BE32(a)                                                             \
    ((((a)>>24)&0xFF) |                                                     \
    (((a)>>8)&0xFF00) |                                                     \
    (((a)<<8)&0xFF0000) |                                                   \
    ((((a)<<24))&0xFF000000))
#define BE64(a)                                                             \
    (BE32(((a)>>32)&0xFFFFFFFF) |                                           \
    ((BE32((a)&0xFFFFFFFF)<<32)&0xFFFFFFFF00000000))
#endif

inline unsigned int QBox::HeaderSize()
{
    if (Version() == 0)
        return (Flags() & QBOX_FLAGS_SAMPLE_DATA_PRESENT) ? 
            sizeof(QBox) - sizeof(QBoxSample1) : sizeof(QBox) - sizeof(unsigned int);
    else
        return (Flags() & QBOX_FLAGS_SAMPLE_DATA_PRESENT) ? 
            sizeof(QBox) - sizeof(QBoxSample) : sizeof(QBox);
}
inline unsigned int QBox::BoxSize()
{
    return BE32(mBoxSize);
}
inline unsigned int QBox::BoxType()
{
    return BE32(mBoxType);
}
inline unsigned int QBox::BoxFlags()
{
    return BE32(mBoxFlags.value);
}
inline unsigned char QBox::Version()
{
    return BE8(mBoxFlags.field.v);
}
inline unsigned int QBox::Flags()
{
    return BE24(mBoxFlags.field.f);
}
inline unsigned short QBox::SampleStreamType()
{
    return BE16(mSampleStreamType);
}
inline unsigned short QBox::SampleStreamId()
{
    return BE16(mSampleStreamId);
}
inline unsigned int QBox::SampleFlags()
{
    return BE32(mSampleFlags);
}
inline unsigned int QBox::SampleCTS()
{
    if (Version() == 0)
        return BE32(mSampleCTS);
    else
        return BE32(mSample.info1.ctslow);
}
inline uint64_t QBox::SampleCTS64()
{
    if (Version() == 0)
        return BE32(mSampleCTS);
    else
        return ((uint64_t)BE32(mSampleCTS) << 32) | BE32(mSample.info1.ctslow);
}
inline unsigned int QBox::SampleAddr()
{
    if (Version() == 0)
        return (Flags() & QBOX_FLAGS_SAMPLE_DATA_PRESENT) ? 
            (unsigned long)mSample.data : BE32(mSample.info.addr); 
    else
        return (Flags() & QBOX_FLAGS_SAMPLE_DATA_PRESENT) ? 
            (unsigned long)(mSample.data + sizeof(mSample.data) ) : BE32(mSample.info1.addr); 
}
inline unsigned int QBox::SampleSize()
{
    if (Version() == 0)
        return (Flags() & QBOX_FLAGS_SAMPLE_DATA_PRESENT) ?
            BoxSize() - sizeof(QBox) + sizeof(QBoxSample1) : BE32(mSample.info.size);
    else
        return (Flags() & QBOX_FLAGS_SAMPLE_DATA_PRESENT) ? 
            BoxSize() - sizeof(QBox) + sizeof(QBoxSample) : BE32(mSample.info1.size);
}
inline void QBox::BoxSize(unsigned int size)
{
    mBoxSize = BE32(size);
}
inline void QBox::BoxType(unsigned int type)
{
    mBoxType = BE32(type);
}
inline void QBox::BoxFlags(unsigned int flags)
{
    mBoxFlags.value = BE32(flags);
    if (!(BoxFlags() & QBOX_FLAGS_SAMPLE_DATA_PRESENT)) {
        BoxSize(sizeof(QBox));
    }
}
inline void QBox::Version(unsigned char v)
{
    mBoxFlags.field.v = BE8(v);
}
inline void QBox::Flags(unsigned int f)
{
    mBoxFlags.field.f = BE24(f);
    if (!(BoxFlags() & QBOX_FLAGS_SAMPLE_DATA_PRESENT)) {
        BoxSize(sizeof(QBox));
    }
}
inline void QBox::SampleStreamType(unsigned short type)
{
    mSampleStreamType = BE16(type);
}
inline void QBox::SampleStreamId(unsigned short id)
{
    mSampleStreamId = BE16(id);
}
inline void QBox::SampleFlags(unsigned int flags)
{
    mSampleFlags = BE32(flags);
}
inline void QBox::SampleCTS(unsigned int cts)
{
    if (Version() == 0)
        mSampleCTS = BE32(cts);
    else {
        mSampleCTS = 0;
        mSample.info1.ctslow = BE32(cts);
    }
}
inline void QBox::SampleCTS64(uint64_t cts)
{
    if (Version() == 0)
        mSampleCTS = BE32((unsigned int)(cts & 0xFFFFFFFF));
    else {
        mSampleCTS = BE32((unsigned int)(cts >> 32));
        mSample.info1.ctslow = BE32((unsigned int)(cts & 0xFFFFFFFF));
    }
}
inline void QBox::SampleAddr(unsigned int addr)
{
    if (Flags() & QBOX_FLAGS_SAMPLE_DATA_PRESENT) {
        fprintf(stderr, "sample should store in addr 0x%x\n", (unsigned int)SampleAddr());
    }
    else {
        if (Version() == 0)
            mSample.info.addr = BE32(addr);
        else
            mSample.info1.addr = BE32(addr);
    }
}
inline void QBox::SampleSize(unsigned int size)
{
    if (Flags() & QBOX_FLAGS_SAMPLE_DATA_PRESENT) {
        if (Version() == 0)
            BoxSize(sizeof(QBox) - sizeof(QBoxSample1) + size);
        else
            BoxSize(sizeof(QBox) - sizeof(QBoxSample) + size);
    }
    else {
        if (Version() == 0)
            mSample.info.size = BE32(size);
        else
            mSample.info1.size = BE32(size);
    }
}
inline void QBox::Dump()
{
#if 1
    char type[5] = {0};
    int2str(BoxType(), type);
    fprintf(stdout, "Box: Size %d, Header Size %d, Type %s, Flags 0x%x(v %2d, f 0x%x) \nSample: Type 0x%x, Stream Id 0x%x, Flags 0x%x, CTS 0x%x-%08x, Addr 0x%x, Size %d\n", (int)BoxSize(), (int)HeaderSize(), type, (unsigned int)BoxFlags(), (int)Version(), (unsigned int)Flags(), (unsigned int)SampleStreamType(), (unsigned int)SampleStreamId(), (unsigned int)SampleFlags(), (unsigned int)(SampleCTS64() >> 32), (unsigned int)(SampleCTS64() & 0xFFFFFFFF), (unsigned int)SampleAddr(), (int)SampleSize());
#endif
}

#endif /* cplusplus */

#endif  /* __QBOX_HH */
/*******************************************************************************
* vi: set shiftwidth=4 tabstop=8 softtabstop=4 expandtab nosmarttab:
*******************************************************************************/
