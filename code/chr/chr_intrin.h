#if !defined(CHR_INTRINSICS_H)

//TODO(handmade): Convert all of these to platform-efficient versions and remove math.h

#include "chr.h"
#include "math.h"

#if COMPILER_MSVC
    #include "intrin.h"
    #include "immintrin.h"
#endif

inline int32
SignBit(int32 Value)
{
    int32 Result = (Value >= 0) ? 1 : -1;
    return Result;
}

inline float32
SquareRoot(float32 Real32)
{
    float32 Result = sqrtf(Real32);
    return Result;
}

inline float
AbsoluteValue(float32 Num)
{
    float32 Result = (float32)fabs(Num);
    return(Result);
}

inline int64
AbsoluteValue(int64 Num)
{
   int64 Result = (((Num) < 0) ? -Num : Num);
   return Result;
}

inline int32
AbsoluteValue(int32 Num)
{
   int32 Result = (((Num) < 0) ? -Num : Num);
   return Result;
}

inline uint32
RotateLeft(uint32 Value, int32 Amount)
{
#if COMPILER_MSVC
	uint32 Result = _rotl(Value, Amount);
#else
	//TODO(handmade): Actually port this to other compiler platforms
	Amount &= 31;
	uint32 Result = ((Value << Amount) | Value >> (32 - Amount));
#endif

	return Result;
}

inline uint32
RotateRight(uint32 Value, int32 Amount)
{
#if COMPILER_MSVC
	uint32 Result = _rotr(Value, Amount);
#else
	//TODO(handmade): Actually port this to other compiler platforms
	Amount &= 31;
	uint32 Result = ((Value >> Amount) | Value << (32 - Amount));
#endif

	return Result;
}

inline int32
RoundReal32ToInt32(float32 Real32)
{
    int32 Result = (int32)roundf(Real32);
    return(Result);
}

inline uint32
RoundReal32ToUInt32(float32 Real32)
{
    uint32 Result = (uint32)roundf(Real32);
    return(Result);
}

inline int32
FloorReal32ToInt32(float32 Real32)
{
    int32 Result = (int32)floorf(Real32);
    return Result;
}

inline int32
CeilReal32ToInt32(float32 Real32)
{
    int32 Result = (int32)ceilf(Real32);
    return Result;
}

inline int32
TruncateReal32ToInt32(float32 Real32)
{
    int32 Result = (int32)(Real32);
    return(Result);
}

inline float32
Sin(float32 Angle)
{
	float32 Result = sinf(Angle);
	return(Result);
}

inline float32
Cos(float32 Angle)
{
	float32 Result = cosf(Angle);
	return(Result);
}
inline float32
ATan2(float32 Y, float32 X)
{
	float32 Result = (float32)atan2(Y, X);
	return(Result);
}

struct bit_scan_result
{
    bool32 Found;
    uint32 Index;
};
inline bit_scan_result
FindLeastSignificantSetBit(uint32 Value)
{
    bit_scan_result Result = {};

#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long*)&Result.Index, Value);
#else
    for (uint32 Test = 0;
        Test < 32;
        ++Test)
    {
        if (Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif

    return(Result);
}

uint32 SwapByteOrder(uint32 A)
{
#if COMPILER_MSVC
        return _byteswap_ulong(A);
#else
        return ((A & 0xFF000000) >> 24) |
               ((A & 0x00FF0000) >> 8) |
               ((A & 0x0000FF00) << 8) |
               ((A & 0x000000FF) << 24);
#endif
}

uint64 SwapByteOrder(uint64 A)
{
#if COMPILER_MSVC
        return _byteswap_uint64(A);
#else
        return ((A & 0xFF00000000000000) >> 56) |
               ((A & 0x00FF000000000000) >> 40) |
               ((A & 0x0000FF0000000000) >> 24) |
               ((A & 0x000000FF00000000) >> 8) |
               ((A & 0x00000000FF000000) << 8) |
               ((A & 0x0000000000FF0000) << 24) |
               ((A & 0x000000000000FF00) << 40) |
               ((A & 0x00000000000000FF) << 56);
#endif
}

#define CHR_INTRINSICS_H
#endif
