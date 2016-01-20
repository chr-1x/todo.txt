#if !defined(CHR_ARRAY)

#include "chr_string.h" //TODO(chronister): Eliminate this dependency (maybe even reverse it)

void* (*ArrayAllocFunc)(size_t BytesToAlloc, bool32 ZeroTheMemory);
bool32 (*ArrayFreeFunc)(void* Memory);

#define AllocArray(T, count) (T*)ArrayAllocFunc((count) * sizeof(T), true)
#define AllocArrayNoClear(T, count) (T*)ArrayAllocFunc((count) * sizeof(T), false)

#define PushItemOntoArray(T, Capacity, NumItems, Array, Item) PushItemOntoArrayPointer(&(Capacity), &(NumItems), (void**)(&(Array)), sizeof(T), (void*)(&(Item)))
internal uint32
PushItemOntoArrayPointer(uint32* CapacityPtr, uint32* NumItemsPtr, void** ArrayPtr, uint32 ItemSize, void* Item)
{
    uint32 NumItems = *NumItemsPtr;

    if (NumItems >= *CapacityPtr)
    {
        void* OldArray = *ArrayPtr;
        uint32 NewSize = Max(NumItems + 0x10, NumItems * 2);
        *ArrayPtr = ArrayAllocFunc(ItemSize * (NewSize), false);
        CopyMemory(*ArrayPtr, OldArray, ItemSize * NumItems);
        ArrayFreeFunc(OldArray);
        *CapacityPtr = NewSize;
    }

    CopyMemory((char*)(*(ArrayPtr)) + NumItems*ItemSize, Item, ItemSize);
    (*NumItemsPtr)++;

    return *NumItemsPtr - 1;
}

#define RemoveItemFromArrayByIndex(T, NumItems, Array, Index) RemoveItemFromArrayPointerByIndex(&(NumItems), Array, sizeof(T), Index)
internal void
RemoveItemFromArrayPointerByIndex(uint32* NumItemsPtr, void* Array, uint32 ItemSize, uint32 ItemIndex)
{
    CopyMemory(((uint8*)Array) + (ItemSize * ItemIndex), ((uint8*)Array) + (ItemSize * (*NumItemsPtr - 1)), ItemSize);
    ZeroMemory(((uint8*)Array) + (ItemSize * (*NumItemsPtr - 1)), ItemSize);
    (*NumItemsPtr)--;
}

internal int64
IndexOfItemPointer(uint32 NumItems, void* Array, uint32 ItemSize, void* Item)
{
    return StringIndexOf(NumItems * ItemSize, (char*)Array, ItemSize, (char*)Item) / ItemSize;
}

#define RemoveItemFromArrayByValue(T, NumItems, Array, Item) RemoveItemFromArrayPointerByValue(&(NumItems), (void*)Array, sizeof(T), (void*)Item)
internal void
RemoveItemFromArrayPointerByValue(uint32* NumItemsPtr, void* Array, uint32 ItemSize, void* Item)
{
    int64 Index = IndexOfItemPointer(*NumItemsPtr, Array, ItemSize, Item);
    if (Index >= 0)
    {
        RemoveItemFromArrayPointerByIndex(NumItemsPtr, Array, ItemSize, (uint32)Index);
    }
}


//TODO(chronister): Hate myself
template <typename T>
struct array
{
    T* Values;
    uint32 Length;
    uint32 Capacity;
};

template <typename T>
array<T>
AllocateArray(uint32 Capacity)
{
    array<T> Result = {};
    Result.Values = AllocArray(T, Capacity);
    Result.Capacity = Capacity;
    return Result;
}

template <typename T>
bool32
ExpandArray(array<T>* Array, uint32 NewCapacity)
{
	uint32 OldCapacity = Array->Capacity;
	void* OldValues = Array->Values;
	Array->Capacity = NewCapacity;
	Array->Values = AllocArray(T, NewCapacity);
	CopyMemory(Array->Values, OldValues, Array->Length * sizeof(T));
	ArrayFreeFunc(OldValues);
	return true; // TODO(chronister): Error checking
}

template <typename T>
T*
ArrayPush(array<T>* Array, T Item)
{
    if (Array->Length >= Array->Capacity || Array->Values == null)
    {
		ExpandArray(Array, Max(Array->Capacity + 10, Array->Capacity * 2));
    }
	++Array->Length;
    Array->Values[Array->Length - 1] = Item;
    return &Array->Values[Array->Length - 1];
}

template<typename T>
void //?
ArrayPushAll(array<T>* Array, array<T> Items)
{
	foreach(T, Item, Items.Length, Items.Values)
	{
		ArrayPush<T>(Array, *Item);
	}
}

template<typename T>
void //?
ArrayPushAllReferences(array<T>* Array, array<T*> Items)
{
	foreach(T*, Item, Items.Length, Items.Values)
	{
		ArrayPush<T>(Array, **Item);
	}
}

template<typename T>
void //?
ArrayPushAllValues(array<T*>* Array, array<T> Items)
{
	foreach(T, Item, Items.Length, Items.Values)
	{
		ArrayPush<T*>(Array, Item);
	}
}

template<typename T>
uint32
ArrayInsert(array<T>* Array, T Item, uint32 TargetIndex)
{
	if (TargetIndex > Array->Length) { TargetIndex = Array->Length; }

	if (Array->Length >= Array->Capacity || Array->Values == null)
	{
		ExpandArray(Array, Max(Array->Capacity + 10, Array->Capacity * 2));
	}
	
	for (int i = Array->Length; i > (int)TargetIndex; --i)
	{
		Array->Values[i] = Array->Values[i - 1];
	}
	Array->Values[TargetIndex] = Item;
	return TargetIndex; 
}

template<typename T>
T
PopArray(array<T>* Array)
{
	T Item = Array->Values[Array->Length - 1];
	Array->Values[--Array->Length] = {};
	return Item;
}

template <typename T>
internal int64
ArraySearch(array<T> Array, bool32 (*Predicate)(T*, void*), void* Param = null, uint32 LeftBound = 0)
{
    if (Array.Capacity == 0 || Array.Values == null || Array.Length == 0) { return -1; }
    for(uint32 i = LeftBound;
        i < Array.Length;
        ++i)
    {
        T Item = Array.Values[i];
        if (Predicate(&Item, Param))
        {
            return i;
        }
    }
    return -1;
}

template <typename T>
internal int64
ArraySearchReverse(array<T> Array, bool32 (*Predicate)(T*, void*), void* Param = null, int RightBound = -1)
{
    if (Array.Capacity == 0 || Array.Values == null || Array.Length == 0) { return -1; }
	if (RightBound == -1) { RightBound = Array.Length - 1; }

    for(uint32 i = (uint32)RightBound;
        i >= 0;
        --i)
    {
        T Item = Array.Values[i];
        if (Predicate(&Item, Param))
        {
            return i;
        }
    }
    return -1;
}

template <typename T>
internal uint32
ArrayOccurrences(array<T> Array, bool32 (*Predicate)(T*, void*), void* Param = null)
{
    if (Array.Capacity == 0 || Array.Values == null || Array.Length == 0) { return 0; }
	uint32 Total = 0;
    for(uint32 i = 0;
        i < Array.Length;
        ++i)
    {
        T Item = Array.Values[i];
        if (Predicate(&Item, Param))
        {
            ++Total;
        }
    }
    return Total;
}

template <typename T>
internal array<T*>
ArrayFilter(array<T> Array, bool32 (*Predicate)(T*, void*), void* Param = null)
{
	array<T*> Result = {};
    for(uint32 i = 0;
        i < Array.Length;
        ++i)
    {
        T* Item = &Array.Values[i];
        if (Predicate(Item, Param))
        {
            ArrayPush<T*>(&Result, Item);
        }
    }
    return Result;
}

template <typename T>
T*
Get(array<T> Array, uint32 Index)
{
	if (Index < 0 || Index >= Array.Length) { return null; }
    return &Array.Values[Index];
}

template <typename T>
T*
GetLast(array<T> Array)
{
    return Get(Array, Array.Length - 1);
}

template <typename T>
array<T*>
ArrayIndirected(array<T> DirectArray)
{
	array<T*> Result = AllocateArray<T*>(DirectArray.Length);
	for (uint i = 0; i < DirectArray.Length; ++i)
	{
		Result.Values[i] = DirectArray.Values + i;
	}
	return Result;
}

template <typename T>
uint32
ArrayRandomIndex(array<T> Array)
{
    return rand() % Array.Length;
}

template<typename T>
T*
ArrayRandomItem(array<T> Array)
{
    return &Array.Values[RandomArrayIndex(Array)];
}

template <typename T>
void
ArrayClear(array<T> Array)
{
    for (uint32 i = 0; i < Array.Capacity; ++i)
    {
        Array.Values[i] = {};
    }
}

template <typename T>
void
FreeArray(array<T>* Array)
{
    ArrayFreeFunc(Array->Values);
    Array->Length = Array->Capacity = 0;
}

template <typename T>
void
RemoveItemByIndex(array<T>* Array, uint32 Index)
{
    if (Index < Array->Length)
    {
        if (Array->Length > 1)
            Array->Values[Index] = Array->Values[--Array->Length];
        else
            Array->Values[Index] = {};
    }
}

template <typename T>
bool32 PredicateItemsIdentical(T* ArrayItem, void* Comparison)
{
    return CompareMemory(ArrayItem, Comparison, sizeof(T));
}

template <typename T>
int64
ArrayIndexOf(array<T> Array, T Item, uint32 Start = 0)
{
    return ArraySearch<T>(Array, PredicateItemsIdentical<T>, &Item, Start);
}

template <typename T>
void
RemoveItemByValue(array<T>* Array, T Item, uint32 StartSearch = 0)
{
	int64 ItemIndex = ArrayIndexOf(*Array, Item, StartSearch);
	if (ItemIndex > 0) 
	{
		RemoveItemByIndex(Array, (uint32)ItemIndex);
	}
}

template <typename T>
void
RemoveMatchingItem(array<T>* Array, bool32 (*Predicate)(T*, void*), void* PredicateArg, uint32 StartSearch = 0)
{
	int64 ItemIndex = ArraySearch(*Array, Predicate, PredicateArg, StartSearch);
	if (ItemIndex > 0) 
	{
		RemoveItemByIndex(Array, (uint32)ItemIndex);
	}
}

// Note: This will only really work with maybe strings, and structs with
// uint8-size members. Won't work with little-endian since a number with nothing
// in the LSByte but a bunch in the MSByte would not be sorted above a number
// with a bunch in the LSByte but none in the MSbyte.
// As such, it's better in general to write a sort specifically for your case.
// This is a nice default maybe.
template <typename T>
int32 CompareRawByteOrder(T Item1, T Item2)
{
	uint8* Ptr1 = (uint8*)(&Item1);
	uint8* Ptr2 = (uint8*)(&Item2);
	for (uint32 i = 0;
		i < (sizeof(T) / sizeof(uint8));
		++i)
	{
		if (Ptr1[i] > Ptr2[i]) { return 1; }
		if (Ptr1[i] < Ptr2[i]) { return -1; }
	}
	return 0;
}

template <typename T>
internal void
ArraySortBubble(array<T>* Array, int32 (*Compare)(T, T))
{
	if (Compare == null) { Compare = &CompareRawByteOrder; }
    int32 Swaps = 0;
    uint32 N = Array->Length;
    uint32 NewN = 0;
    do
    {
        NewN = 0;
        for (uint32 i = 1;
            i < N;
            ++i)
        {
            if (Compare(Array->Values[i - 1], Array->Values[i]) >= 0)
            {
                Swap(Array->Values[i], Array->Values[i - 1], T);
                NewN = i;
            }
        }
        N = NewN;
    } while(N > 0);
}

#define CHR_ARRAY
#endif
