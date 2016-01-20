/* Null-termination independent printf! */
#include <chr_string.h>
#include <chr_math.h>
#include <chr_intrin.h>

#include "stdarg.h"

using namespace integer;

//TODO(chronister): Support bases higher than 16?
//TODO(chronister): Localization?
char PrintfDigits[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };


struct integer_format
{
	uint8 Width;
	uint8 Base;
	bool32 LeftJustify;
	bool32 Lowercase;
	char PadCharacter;
	bool32 ForceSign;
};
integer_format DefaultIntegerFormat = { 0, 10, false, false, ' ', false };

// Unless otherwise specified, print functions return the amount of space in the buffer that would have been used if it
// could have been filled.

uint32 PrintUInteger(uint64 Num, integer_format Format, uint32 BufferCapacity, char* Buffer)
{
	char* Cursor = Buffer;

	uint8 NumWidth = (uint8)(LogN(Num, Format.Base) + 1);
	uint8 FullWidth = Max(NumWidth, Format.Width);
	uint8 PadWidth = (FullWidth > NumWidth) ? FullWidth - NumWidth : 0;

	if (Format.PadCharacter == '\0') Format.PadCharacter = ' ';
	
	if (!Format.LeftJustify && PadWidth > 0)
	{
		while (Cursor - Buffer < PadWidth
			&& Cursor - Buffer < (int)BufferCapacity)
		{
			*Cursor++ = Format.PadCharacter;
		}
	}
	char* NumCursor = (Cursor - 1) + NumWidth;
	
	while (NumCursor >= Cursor)
	{
		uint8 DigitValue = Num % Format.Base;
		Num /= Format.Base;

		if (NumCursor - Buffer < (int)BufferCapacity)
		{
			if (Format.Lowercase)  
				*NumCursor = CharToLower(PrintfDigits[DigitValue]);
			else
				*NumCursor = PrintfDigits[DigitValue];
		}
		--NumCursor;
	}
	Cursor = Cursor + NumWidth;
	NumCursor = Cursor;

	if (Format.LeftJustify && PadWidth > 0)
	{
		while (Cursor - NumCursor < PadWidth 
			&& Cursor - Buffer < (int)BufferCapacity)
		{
			*Cursor++ = Format.PadCharacter;
		}
	}
	return FullWidth;
}

uint32
PrintInteger(int64 Num, integer_format Format, uint32 BufferCapacity, char* Buffer)
{
	if (Format.ForceSign || Num < 0)
	{
		uint8 NumWidth = (uint8)(LogN(AbsoluteValue(Num), Format.Base) + 1);
		uint8 FullWidth = Max(NumWidth, Format.Width);
		uint8 PadWidth = (FullWidth > NumWidth) ? FullWidth - NumWidth : 0;
		uint32 Result = 0;

		if (Format.LeftJustify || PadWidth == 0)
		{
			// Take off a bit of padding to leave room for the sign at the front
			if (PadWidth > 0)
				Format.Width -= 1;
			
			// Because capacity is given as a uint, we have to be sure it doesn't underflow.
			uint32 Capacity = BufferCapacity - 1;
			if (Capacity > BufferCapacity) { Capacity = 0; }

			Result = PrintUInteger((uint32)AbsoluteValue(Num), Format, Capacity, Buffer + 1);
			// The sign will be the very first character.
			if (BufferCapacity >= 1)
				*Buffer = (Num < 0 ? '-' : '+');
		}
		else
		{
	Result = PrintUInteger((uint32)AbsoluteValue(Num), Format, BufferCapacity, Buffer);
			// We can replace one of the pad characters with the sign
			if ((int)BufferCapacity >= (PadWidth - 1))
				*(Buffer + PadWidth - 1) = (Num < 0 ? '-' : '+');
		}
		return Result + 1;
	}
	else
		// We can safely cast to uint64 because we checked above if it was negative.
		return PrintUInteger((uint64)Num, Format, BufferCapacity, Buffer);
}

struct float_format
{
	uint16 Width;
	uint16 Precision;
	uint8 Base;
	bool32 UseScientificNotation;
	bool32 ForceSign;
	bool32 LeftJustify;
	char RadixCharacter;
	char PadCharacter;
};
float_format DefaultFloatFormat = { 0, 6, 10, false, false, false, '.', ' '};

uint32
PrintFloat(float64 Num, float_format Format, uint32 BufferCapacity, char* Buffer)
{
	float64 WholePart = fabs(trunc(Num));
	float64 FractionalPart = fabs(Num - WholePart);

	uint16 WholeWidth = (uint8)trunc(real::LogN(WholePart, (float64)Format.Base)) + 1;
	uint16 RadixWidth = Format.Precision;
	uint16 NumWidth = WholeWidth + RadixWidth + 1; // For the decimal point
	if (Format.ForceSign || Num < 0) { NumWidth += 1; }
	
	uint16 FullWidth = Max(NumWidth, Format.Width);
	uint16 ActualWidth = (uint8)Min(FullWidth, BufferCapacity);
	uint16 PadWidth = (FullWidth > NumWidth) ? FullWidth - NumWidth : 0;

	char* Cursor = Buffer;

	if (Format.PadCharacter == '\0') Format.PadCharacter = ' ';
	if (Format.RadixCharacter == '\0') Format.RadixCharacter = '.';

	if (!Format.LeftJustify && PadWidth > 0)
	{
		while (Cursor - Buffer < PadWidth
			&& Cursor - Buffer < BufferCapacity)
		{
			*Cursor++ = Format.PadCharacter;
		}
	}

	if (Num < 0 || Format.ForceSign)
	{
		*Cursor++ = (Num < 0 ? '-' : '+');
	}

	//Precondition: Cursor at highest digit of num
#if 0
	for (int i = WholeWidth - 1; i >= 0; --i, ++Cursor)
	{
		float64 Magnitude = pow(10., (float64)i);
		uint8 DigitValue = (uint8)trunc(fmod(Num / Magnitude, 10.));
		if (Cursor - Buffer < BufferCapacity)
			*Cursor = Digits[DigitValue];
	}
#else
	Cursor += WholeWidth - 1;
	for (int i = WholeWidth - 1; i >= 0; --i, --Cursor)
	{
		uint8 DigitValue = (uint8)trunc(fmod(WholePart, 10.));
		WholePart /= 10.;
		if (Cursor - Buffer < BufferCapacity)
			*Cursor = PrintfDigits[DigitValue];
	}
	Cursor += WholeWidth + 1;
#endif
	//Postcondition: Cursor after lowest digit of num
	
	if (Cursor - Buffer <= BufferCapacity)
		*Cursor = Format.RadixCharacter;
	for (char* NumCursor = Cursor + 1;
	    NumCursor - Cursor <= RadixWidth;
		++NumCursor)
	{
		float64 PlaceMul = pow(10., (float64)(NumCursor - Cursor));
		uint8 DigitValue = (uint8)trunc(fmod(FractionalPart * PlaceMul, 10.));
		if (NumCursor - Buffer < BufferCapacity)
			*NumCursor = PrintfDigits[DigitValue];
	}
	Cursor += RadixWidth;
	
	char* PadCursor = ++Cursor;
	if (Format.LeftJustify && PadWidth > 0)
	{
		while (PadCursor - Cursor < PadWidth
			&& PadCursor - Buffer < BufferCapacity)
		{
			*PadCursor++ = Format.PadCharacter;
		}
	}

	return FullWidth;
}

/* Defines for dealing with floats! */
/* The MSB is the sign bit */
#define F64_SIGNBIT_MASK	0x8000000000000000
/* The next 11 MSBs are the exponent bits */
#define F64_EXPONENT_MASK	0x7FF0000000000000
/* The next 52 bits are the mantissa bits */
#define F64_MANTISSA_MASK	0x000FFFFFFFFFFFFF
/* The exponent is biased by a certain value */
#define F64_EXPONENT_BIAS	1023
/* It is helpful to be able to shift the exponent to access its value */
#define F64_EXPONENT_SHIFT	52
/* The whole part of the mantissa is assumed to be 1 in normalized form. 
 * This is inconvenient to add, so we'll define which bit it corresponds to for convenience. */
#define F64_MANTISSA_LEAD	0x0010000000000000

//TODO(chronister): Finish this!
#if 0
uint32
PrintFloatExponential(float64 Num, float_format Format, uint32 BufferCapacity, char* Buffer)
{
	uint64* RawPtr = (uint64*)(&Num);
	uint64 RawNum = *RawPtr;

	uint64 SignBit = RawNum & F64_SIGNBIT_MASK;
	uint64 RawExponent = (RawNum & F64_EXPONENT_MASK) >> F64_EXPONENT_SHIFT;
	// We'll add 1 because the number is stored with the assumption that the mantissa is something like:
	// 1.111010010101...
	// But we want to access it as:
	// 0.111101001010...
	// At 1 exponent higher than it was.
	int16 Exponent = (int16)(RawExponent - F64_EXPONENT_BIAS) + 1;
	uint64 Mantissa = RawNum & F64_MANTISSA_MASK;
	if (Mantissa != 0) Mantissa += F64_MANTISSA_LEAD;

	char* Cursor = Buffer;
	if (SignBit || Format.ForceSign)
	{
		if (BufferCapacity >= 1)
			*Cursor++ = (SignBit ? '-' : '+');
	}

	if (RawExponent == 0)
	{
		// Exponent is all 0s. Two possible cases.
		// First, it's zero. It'll be taken care of by default.
		// Second, it's a denormalized number, so we'll have to change the way we deal with it.
		if (Mantissa != 0)
		{
			// It's denormalized. The exponent is assumed to be -BIAS + 1
			Exponent = -F64_EXPONENT_BIAS + 1;
			// Also, because it's denormalized, instead of 1.10101011011..., it's 0.10101011011...
			// This is actually nice, because we wanted to do this anyway. We assumed above we'd have to add the
			// implicit bit, but we don't now, so we'll subtract it again.
			Mantissa -= F64_MANTISSA_LEAD;
		}
	}

	if ((RawExponent & (F64_EXPONENT_MASK >> F64_EXPONENT_SHIFT)) == 0)
	{
		// Exponent is all 1s. Three possible cases.
		// First, it's Infinity. If so, the mantissa will be all 0s.
		if (Mantissa == 0)
		{
			// Maintain the sign, since there is an important distinction between +Infinity and -Infinity
			int BufferSpace = (BufferCapacity > 0 ? BufferCapacity - 1 : 0);
			CopyString(8, "Infinity", BufferSpace, Buffer + 1);
			return 9;
		}
		// Second and third, it's an SNAN or a QNAN. 
		// We don't care about exceptions so we'll treat them both the same.
		else
		{
			// NaN doesn't actually have a sign, so we'll overwrite that piece of the string if applicable.
			int BufferSpace = (BufferCapacity > 0 ? BufferCapacity - 1 : 0);
			CopyString(3, "NaN", BufferSpace, Buffer);
			return 3;
		}
	}

	// We've dealt with the nonconforming cases, so hopefully at this point we have a valid floating point number.
	// Scientific notation starts with the mantissa.
	uint16 Digits = Format.Precision;
}
#endif

enum char_format_case
{
	CASE_NO_CHANGE,
	CASE_UPPER,
	CASE_LOWER
};

struct char_format
{
	char_format_case Case;
	uint8 SequencePrintBase;
	bool32 UseFriendlyEscapes;
	bool32 UseSequenceEscapes;
	bool32 AlwaysUseSequenceEscapes;
};
char_format DefaultCharFormat = { CASE_NO_CHANGE, 0, false, false, false };

char PrintableChars[] = " `~!@#$%^&*()_+-={}[]\\|;:'\"<,>.?/";
bool32
CharIsPrintable(char C)
{
	if (CharIsUppercase(C) || CharIsLowercase(C)) { return true; }
	int Index = StringIndexOf(ArrayCount(PrintableChars), PrintableChars, 1, &C);
	return (Index >= 0 && Index < ArrayCount(PrintableChars) - 1);
}

uint32
PrintCharacter(char C, char_format Format, uint32 BufferCapacity, char* Buffer)
{
	if (CharIsLowercase(C) && !Format.AlwaysUseSequenceEscapes)
	{
		if (BufferCapacity >= 1)
		{
			if (Format.Case == CASE_UPPER) { *Buffer = CharToUpper(C); }
			else { *Buffer = C; }
		}
		return 1;
	}
	if (CharIsUppercase(C) && !Format.AlwaysUseSequenceEscapes)
	{
		if (BufferCapacity >= 1)
		{
			if (Format.Case == CASE_LOWER) { *Buffer = CharToLower(C); }
			else { *Buffer = C; }
		}
		return 1;
	}
	if (Format.AlwaysUseSequenceEscapes || Format.UseSequenceEscapes) { 
		if (CharIsPrintable(C) && !Format.AlwaysUseSequenceEscapes)
		{
			if (BufferCapacity >= 1)
				*Buffer = C;
			return 1;
		}


#define CharacterPrintSpecialCase(C, EscapeSeq, Representation) \
		if (Format.UseFriendlyEscapes \
		&& !Format.AlwaysUseSequenceEscapes \
		&& C == EscapeSeq) { \
			CopyString(sizeof(Representation)-1, Representation, BufferCapacity, Buffer, false); \
			return sizeof(Representation)-1; \
		}

		CharacterPrintSpecialCase(C, '\n', "\\n");
		CharacterPrintSpecialCase(C, '\t', "\\t");
		CharacterPrintSpecialCase(C, '\r', "\\r");
		CharacterPrintSpecialCase(C, '\f', "\\f");
		
		if (Format.SequencePrintBase == 0) Format.SequencePrintBase = 10;
		char* Cursor = Buffer;
		*Cursor++ = '\\';
		uint TotalLen = 1;
		if (Format.SequencePrintBase == 8 && BufferCapacity >= 2) 
		{
			*Cursor++ = '0';
			++TotalLen;
		}
		else if (Format.SequencePrintBase == 16 && BufferCapacity >= 2) 
		{
			*Cursor++ = 'x';
			++TotalLen;
		}
		uint8 NumDigits = (uint8)integer::LogN(C, Format.SequencePrintBase) + 1;
		TotalLen += NumDigits;
		char* NumCursor = Cursor + (NumDigits - 1);
		while (NumCursor >= Cursor)
		{
			uint8 DigitValue = C % Format.SequencePrintBase;
			C /= Format.SequencePrintBase;

			if (NumCursor - Buffer < (int)BufferCapacity)
			{
				*NumCursor = PrintfDigits[DigitValue];
			}
			--NumCursor;
		}
		return TotalLen;
	}
	else
	{
		if (BufferCapacity >= 1)
			*Buffer = C;
		return 1;
	}
}

struct string_format
{
	char_format CharFormat;
};
string_format DefaultStringFormat = { CASE_NO_CHANGE };

uint32
PrintCString(cstring Str, string_format Format, uint32 BufferCapacity, char* Buffer)
{
	uint i;
	uint32 Len = StringLength(Str);
	uint32 Effective = Min(Len, BufferCapacity);
	for(i = 0; i < Effective; ++i)
	{
		if (Format.CharFormat.Case == CASE_UPPER)
			*Buffer++ = CharToUpper(*Str++);
		else if (Format.CharFormat.Case == CASE_UPPER)
			*Buffer++ = CharToLower(*Str++);
		else 
			*Buffer++ = *Str++;
	}
	return Len;
}

enum format_type
{
	FMT_NONE = 0,
	FMT_POINTER,
	FMT_INT,
	FMT_UINT,
	FMT_CHAR,
	FMT_STRING,
	FMT_FLOAT
};
char* FormatTypeSpecifiers[] = { "", "p", "di", "uxXo", "c", "s", "fFeEaA" };

#if 0
void
FormatIntoStringVariadic(astring* Dest, cstring FormatStr, va_list args)
{
	va_list args2;
	va_copy(args2, args);

	bool32 InFormatSpecifier = false;
	char* TokenStart = FormatStr;
	char* TokenEnd = FormatStr;
	char* C;
	for (C = FormatStr; *C != 0; ++C)
	{
		if (*C == '%') 
		{
			InFormatSpecifier = true;
			TokenStart = C;
			if (TokenEnd < TokenStart)
			{
				bstring Addendum = {};
				Addendum.Value = TokenEnd;
				Addendum.Length = (uint32)(TokenStart - TokenEnd);
				AppendToString(Dest, Addendum);
			}
		}
		else if (InFormatSpecifier)
		{
			format_type Type = FMT_NONE;
			for(uint i = 0;
				i < ArrayCount(FormatTypeSpecifiers);
				++i)
			{
				char* FormatTypeList = FormatTypeSpecifiers[i];
				if (StringContains(StringLength(FormatTypeList), FormatTypeList, 1, C))
				{
					Type = (format_type)i;
					break;
				}
			}

			if (Type != FMT_NONE)
			{
				InFormatSpecifier = false;
				TokenEnd = C + 1;
				int Capacity = Dest->Capacity - Dest->Length;
				if (Capacity < 0) { Capacity = 0; }

				switch(Type)
				{
					case FMT_POINTER:
					{
						integer_format Format = {};
						Format.Base = 16;
						Format.Width = sizeof(intptr_t) * 2;
						Format.PadCharacter = '0';
						intptr_t Num = va_arg(args2, intptr_t);
						Dest->Length += PrintUInteger(Num, Format, Capacity, Dest->Value + Dest->Length);
					} break;

					case FMT_INT:
					{
						integer_format Format = DefaultIntegerFormat;
						int Num = va_arg(args2, int);
						Dest->Length += PrintInteger(Num, Format, Capacity, Dest->Value + Dest->Length);
					} break;

					case FMT_UINT:
					{
						integer_format Format = DefaultIntegerFormat;
						uint Num = va_arg(args2, uint);
						Dest->Length += PrintUInteger(Num, Format, Capacity, Dest->Value + Dest->Length);
					} break;

					case FMT_CHAR:
					{
						char_format Format = DefaultCharFormat;
						char Char = va_arg(args2, char);
						Dest->Length += PrintCharacter(Char, Format, Capacity, Dest->Value + Dest->Length);
					} break;

					case FMT_STRING:
					{
						string_format Format = DefaultStringFormat;
						char* CStr = va_arg(args2, char*);
						Dest->Length += PrintCString(CStr, Format, Capacity, Dest->Value + Dest->Length);
					} break;

					case FMT_FLOAT:
					{
						float_format Format = DefaultFloatFormat;
						float64 Num = va_arg(args2, float64);
						Dest->Length += PrintFloat(Num, Format, Capacity, Dest->Value + Dest->Length);
					} break;
				}
			}
		}
	}

	if (C > TokenEnd)
	{
		bstring Addendum = {};
		Addendum.Value = TokenEnd;
		Addendum.Length = (uint32)(C - TokenEnd);
		AppendToString(Dest, Addendum);
	}

	va_end(args2);
}

void
FormatIntoString(astring* Dest, cstring Format, ...)
{
    va_list args;
    va_start(args, Format);
    FormatIntoStringVariadic(Dest, Format, args);
    va_end(args);
}
#else 
#if 1
#include "stdio.h"
void
FormatIntoStringVariadic(astring* Dest, cstring FormatStr, va_list args)
{
	Dest->Length = vsnprintf(Dest->Value, Dest->Capacity, FormatStr, args);
}

void
FormatIntoString(astring* Dest, cstring Format, ...)
{
    va_list args;
    va_start(args, Format);
    FormatIntoStringVariadic(Dest, Format, args);
    va_end(args);
}
	
#else


#define MAGIC_NUMBER 0x4B98

struct print_argument
{
	int64 data;
	uint32 (*fmt)(int, integer_format, uint32, uint32);
};


struct integer_print
{
	print_type Type;
	int64 Number;
	integer_format Format;
};
integer_print fInt(int64 Number, integer_format Format) 
{
	integer_print Result;
	Result.Number = Number;
	Result.Format = Format;
	return Result;
}


struct float_print
{
	print_type Type;
	float64 Number;
	float_format Format;
};
float_print fFloat(float64 Number, float_format Format)
{
	float_print Result;
	Result.Number = Number;
	Result.Format = Format;
	return Result;
}

// TODO(chronister): va_arg needs to know the size of the thing it's popping off the argument list.
// So, unless all the format structs are the same size, we can't pass them by value without getting real funky.
// Option 1: Format funcs above allocate on the heap, main string format func FreeString, pass ptrs
// Option 2: Make all format structs the same size (janky)
// 			Use a union?????
// Option 3: Add a magic number parameter to each struct and read bytes off the argument stack until you get to it (way
// janky)
// Option 4: ???????????????
// Option 5: Make the macro prepend the size of the next item so we know how big of a thing to pop off
// Applicable to all of the above: still need to have a format function ptr involved so that we can support custom
// types! That might help..?
void
FormatIntoStringVariadic(astring* Dest, cstring Format, va_list args)
{
	va_list args2;
	va_copy(args2, args);

	uint FormatsUsed[20] = {};
	char* FormatIndices[20] = {};
	uint FormatsUsedCursor = 1;

	char* C;
	for (C = Format; *C != 0; ++C)
	{
		if (*C == '%')
		{
			if (IsNumber(*(C + 1)))
			{
				char* NumEnd = C + 1;
				while (IsNumber(NumEnd)) ++NumEnd;

				parse_int_result Parse = ParseInteger(NumEnd - (C + 1), C + 1);
				if (Parse.Valid)
				{
					FormatIndices[FormatsUsedCursor] = C;
					FormatsUsed[FormatsUsedCursor++] = Parse.Value;
				}
			}
		}
	}

	uint HighestFormat = 0;
	for (i = 0; i < FormatsUsedCursor; ++i)
		if (FormatsUsed[i] > HighestFormat)
			HighestFormat = FormatsUsed[i];

	generic_print* Formats

}

#endif
#endif


#if 1
uint32
GetFormattedLengthVariadic(cstring Format, va_list args)
{
	astring FakeAString;
	char Fake = 0;
	FakeAString.Value = &Fake;
	FakeAString.Capacity = 1;
	FakeAString.Length = 0;
    FormatIntoStringVariadic(&FakeAString, Format, args);
	return FakeAString.Length;
}

uint32
GetFormattedLength(cstring Format, ...)
{
    va_list args;
    va_start(args, Format);
    uint32 Result = GetFormattedLengthVariadic(Format, args);
    va_end(args);

	return Result;
}

astring
FormatStringVariadic(cstring Format, va_list args)
{
    astring Result = {};

    va_list args2;
    va_copy(args2, args);
    Result.Length = GetFormattedLengthVariadic(Format, args2);
    va_end(args2);

    va_copy(args2, args);
    Result.Capacity = Result.Length + 1;
    Result.Value = (cstring)StringAllocFunc(Result.Capacity, false);
    FormatIntoStringVariadic(&Result, Format, args2);
    va_end(args2);

    return Result;
}

astring
FormatString(cstring Format, ...)
{
    astring Result = {};
    va_list args;
    va_start(args, Format);
    Result = FormatStringVariadic(Format, args);
    va_end(args);
    return Result;
}

void
AppendFormatIntoString(astring* Dest, cstring Format, ...)
{
	// Temp is a proxy for Dest that offsets by the length so that it does become an append.
	astring Temp = ASTR(Dest->Value + Dest->Length, Dest->Capacity - Dest->Length, 0); 
    va_list args;
    va_start(args, Format);

    FormatIntoStringVariadic(&Temp, Format, args);
	Dest->Length = Min(Dest->Length + Temp.Length, Dest->Capacity - 1);
    va_end(args);
}

void
AppendFullFormatIntoString(astring* Dest, cstring Format, ...)
{
    va_list args;
    va_start(args, Format);
    astring Temp = FormatStringVariadic(Format, args);
    va_end(args);

    AppendFullToString(Dest, A2BSTR(Temp));
    FreeString(&Temp);
}
#endif
