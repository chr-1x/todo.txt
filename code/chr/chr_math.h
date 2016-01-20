#if !defined(CHR_MATH)

#include "limits.h"
#include "math.h"

#define MS_PER_SEC(S) ((S) * 1000)
#define MS_PER_MIN(M) (MS_PER_SEC(M) * 60)
#define MS_PER_HOUR(H) (MS_PER_MIN(H) * 60)

#define S_PER_MIN(M) ((M) * 60)
#define S_PER_HOUR(H) (S_PER_MIN(H) * 60)
#define S_PER_DAY(D) (S_PER_HOUR(D) * 24)

#if 0
#define AbsoluteValue(Expr) \
	(((Expr) < 0) ? -Expr : Expr)
#endif

inline uint32
SafeTruncateUInt64(uint64 Value)
{
    //TODO(chronister): Defines for max values
    Assert(Value <= UINT32_MAX);
    return((uint32)Value);
}

/* =====================
      Math Operations 
   ===================== */

//TODO(chronister): Generalize these to all number types!

//TODO(chronister): Overflow?
//TODO(chronister): Intrinsics?
namespace integer
{
	int64
	Pow(int32 Value, int32 Power)
	{
		int64 Result = 1;
		while(Power-- > 0)
		{
			Result *= Value;
		}
		return Result;
	}

	uint64
	LogN(uint64 Value, uint64 Base)
	{
		if (Value < Base) { return 0; }
		uint64 Result = 0;
		while (Value > Base - 1)
		{
			Value /= Base;
			Result++;
		}
		return Result;
	}

	uint64 Log10(uint64 Value) { return LogN(Value, 10); }
	uint64 Log2(uint64 Value) { return LogN(Value, 2); }
}

namespace real
{
	float64
	LogN(float64 Value, float64 Base)
	{
		return log(Value) / log(Base);
	}
}


#define CHR_MATH
#endif
