/* Copyright (c) 2008, Третьяк А. В.
*
* Разрешается повторное распространение и использование как в виде исходного кода, так и в
* двоичной форме, с изменениями или без, при соблюдении следующих условий:
*
*     * При повторном распространении исходного кода должно оставаться указанное выше
*       уведомление об авторском праве, этот список условий и последующий отказ от гарантий.
*     * При повторном распространении двоичного кода должно сохраняться указанная выше
*       информация об авторском праве, этот список условий и последующий отказ от гарантий в
*       документации и/или в других материалах, поставляемых при распространении.
*     * При изменении исходного кода, измененная версия должна быть опубликована и доступна
*       в публичном доступе.
*
* ЭТА ПРОГРАММА ПРЕДОСТАВЛЕНА ВЛАДЕЛЬЦАМИ АВТОРСКИХ ПРАВ И/ИЛИ ДРУГИМИ СТОРОНАМИ
* "КАК ОНА ЕСТЬ" БЕЗ КАКОГО-ЛИБО ВИДА ГАРАНТИЙ, ВЫРАЖЕННЫХ ЯВНО ИЛИ ПОДРАЗУМЕВАЕМЫХ,
* ВКЛЮЧАЯ, НО НЕ ОГРАНИЧИВАЯСЬ ИМИ, ПОДРАЗУМЕВАЕМЫЕ ГАРАНТИИ КОММЕРЧЕСКОЙ ЦЕННОСТИ И
* ПРИГОДНОСТИ ДЛЯ КОНКРЕТНОЙ ЦЕЛИ. НИ В КОЕМ СЛУЧАЕ, ЕСЛИ НЕ ТРЕБУЕТСЯ СООТВЕТСТВУЮЩИМ
* ЗАКОНОМ, ИЛИ НЕ УСТАНОВЛЕНО В УСТНОЙ ФОРМЕ, НИ ОДИН ВЛАДЕЛЕЦ АВТОРСКИХ ПРАВ И НИ ОДНО
* ДРУГОЕ ЛИЦО, КОТОРОЕ МОЖЕТ ИЗМЕНЯТЬ И/ИЛИ ПОВТОРНО РАСПРОСТРАНЯТЬ ПРОГРАММУ, КАК БЫЛО
* СКАЗАНО ВЫШЕ, НЕ НЕСЁТ ОТВЕТСТВЕННОСТИ, ВКЛЮЧАЯ ЛЮБЫЕ ОБЩИЕ, СЛУЧАЙНЫЕ,
* СПЕЦИАЛЬНЫЕ ИЛИ ПОСЛЕДОВАВШИЕ УБЫТКИ, ВСЛЕДСТВИЕ ИСПОЛЬЗОВАНИЯ ИЛИ НЕВОЗМОЖНОСТИ
* ИСПОЛЬЗОВАНИЯ ПРОГРАММЫ (ВКЛЮЧАЯ, НО НЕ ОГРАНИЧИВАЯСЬ ПОТЕРЕЙ ДАННЫХ, ИЛИ ДАННЫМИ,
* СТАВШИМИ НЕПРАВИЛЬНЫМИ, ИЛИ ПОТЕРЯМИ ПРИНЕСЕННЫМИ ИЗ-ЗА ВАС ИЛИ ТРЕТЬИХ ЛИЦ, ИЛИ ОТКАЗОМ
* ПРОГРАММЫ РАБОТАТЬ СОВМЕСТНО С ДРУГИМИ ПРОГРАММАМИ), ДАЖЕ ЕСЛИ ТАКОЙ ВЛАДЕЛЕЦ ИЛИ
* ДРУГОЕ ЛИЦО БЫЛИ ИЗВЕЩЕНЫ О ВОЗМОЖНОСТИ ТАКИХ УБЫТКОВ. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>//это конечно нифига не кроссплатформенно, но я все равно не знаю аналога интринсика _BitScanReverse под nix
#include <crtdbg.h>
#include <stdlib.h>
#include <malloc.h>


#include "mballoc.h"

//Конфиг настраивается здесь, ибо все равно изменения в header-е ничего не дадут без перекомпиляции этого файла, к тому же изменения в mballoc.h приведут к перекомпиляции всех файлов, использующих аллокатор
#ifndef NDEBUG //получать стек реализованным способом можно только в дебаге
//#define DUMP_ALLOC_CALL_STACK //если дамп стека не нужен, просто закомментируйте эту строчку
#ifdef DUMP_ALLOC_CALL_STACK
#define GATHER_BLOCK_GROUPS_STATS //определяет, нужно ли поддерживать MBGetBlockGroups; не работает без DUMP_ALLOC_CALL_STACK
#endif
#endif

#define MBALLOC_ALIGNMENT 4//выравнивание выделяемых блоков (при использовании SSE можно поставить 16)

#define SUBPOWER_OF_TWO_BLOCK_SIZE_ACCURACY 2//поддерживаются значения от 0 до 2; когда 0, блоки выделяются размером только в степени двойки, когда =1, выделяются: 2^n и (2^n)*1.5, когда =2, выделяются: 2^n, (2^n)*1.25, (2^n)*1.5 и (2^n)*1.75 и т.д.
static const int MEGABLOCK_SIZE = 128*1024;//это число и ниже - не реально выделяемые размеры, а просто ориентировочные числа, лучше чтоб были степенями двойки
static const int MAX_BLOCK_SIZE =  16*1024;//при превышении этого размера блоки выделяются у системы



#define ASSERT(x) _ASSERT(x)//do ; while (false)
#define LENGTH(a) (sizeof(a)/sizeof(a[0]))
#ifdef MB_THREADSAFE
#ifdef _MSC_VER
#define DECL_THREAD __declspec(thread)
#elif defined __GNUC__
#define DECL_THREAD __thread
#else
#error Thread safity not supported on the current platform
#endif
#else
#define DECL_THREAD
#endif

#if MBALLOC_ALIGNMENT <= _CRT_PACKING//=8=MALLOC_ALIGNMENT
#define SysAlloc(sz) malloc(sz)//эти функции не обязаны быть потокобезопасными
#define SysFree(p) free(p)
#define SysMemSize(p) _msize(p)
#else
#define SysAlloc(sz) _aligned_malloc(sz, MBALLOC_ALIGNMENT)
#define SysFree(p) _aligned_free(p)
#define SysMemSize(p) _aligned_msize(p, MBALLOC_ALIGNMENT, 0)
#endif


static const int ptrSize = sizeof(void*);//размер указателя на текущей платформе


struct MegaBlock;
struct Context;

struct BlockHeader//это те самые 4 байта, которые лежат перед каждым выделенным блоком
{
	MegaBlock *megaBlock;//типа родитель
#ifndef NDEBUG
	//Доп. поля для отладки (напр. callstack аллокации, флаг isArray при выделении new[] для сопоставления с delete[]), которые будут в каждом блоке
#ifdef OPERATOR_NEW_OVERLOAD
	int isArray;//конечно, в простых блоках этот флаг не нужен, но маленькие блоки выделяются обычно через new, а через malloc - только большие блоки, где размер этого поля роли не сыграет
	enum {ARRAY_SIGN = 0xAAC0DEDD, NONARRAY_SIGN = 0xDeadAAAA};//используется не просто флаг, а специальная сигнатура, т.к. в массив объектов, имеющих деструктор, компилятор добавляет в начало блока int - кол-во элементов массива и указатель не будет указывать на BlockHeader, а будет смещен на 4 байта
#endif
#endif
#ifdef DUMP_ALLOC_CALL_STACK
	DWORD callstack[4];//адреса кода для 4-х уровней стека
#endif
};


struct FreeBlockHeader : BlockHeader//фишка в том, что когда блок свободен, его можно заюзать под какие-нить доп. данные, жаль только, их использование возможно лишь когда блок неаллоцирован
{
	FreeBlockHeader *nextFreeBlock;//указатель на следующий свободный блок
};


struct MegaBlock
{
	MegaBlock *prev, *next;//мегаблок может находиться в одном из трех списков, указатель prev используется во всех кроме freeMegaBlock, т.к. из этого списка мегаблок нельзя удалить из середины
	FreeBlockHeader *firstBlock;//первый свободный блок (в списке freeMegaBlock - не используется, в filledMegaBlock - должно быть NULL, и в partlyFreeMegaBlocks - не может быть NULL)
	int index;//индекс мегаблока в массиве partlyFreeMegaBlocks
	int numOfAllocatedBlocks;//когда достигает нуля, мегаблок помещается в список неразмеченных блоков
#ifdef MB_THREADSAFE
	Context *context;//контекст потока, в котором был выделен данный мегаблок, именно его нужно использовать при освобождении мегаблока
#endif
};
static const int MEGABLOCK_HEADER_SIZE = ((sizeof(MegaBlock) + sizeof(BlockHeader) + MBALLOC_ALIGNMENT-1) & ~(MBALLOC_ALIGNMENT-1)) - sizeof(BlockHeader);


struct PartlyFreeMegaBlock
{
	MegaBlock *first;//первый н** (собственна, указатель на первый блок в списке частично-свободных)
	MegaBlock *justAllocated;//только что аллоцированный мегаблок (далее идут поля связанные именно с ним)
	char *nextFreeBlock;
	int blocksLeft;//кол-во оставшихся блоков
};


DECL_THREAD struct Context
{
	PartlyFreeMegaBlock partlyFreeMegaBlocks[ptrSize*8*(1<<SUBPOWER_OF_TWO_BLOCK_SIZE_ACCURACY)];//расчет размера массива приблизительный (но с запасом), т.к. точно посчитать в compile-time трудно
	MegaBlock *freeMegaBlock;//указатель на первый блок в списке неразмеченных блоков
	MegaBlock *filledMegaBlock;//указатель на первый блок в списке заполненных блоков (этот список необходим лишь для корректного удаления таких блоков, т.к. они не фигурируют в других списках)
	bool forceTLSmode;
	int sysAllocatedBlocks, sysAllocatedBlocksSize;
#ifdef GATHER_BLOCK_GROUPS_STATS
	MBBlockGroup blockGroups[8*1024];//это очень простой хэш без возможности удаления с добавлением в первый пустой элемент
#endif
} context = {{0}, NULL, NULL, false, 0, 0};


#ifdef GATHER_BLOCK_GROUPS_STATS
void UpdateBlockGroup(BlockHeader *b, Context *ct, int size)
{
	int index = b->callstack[0];
	for (int i=1; i<LENGTH(b->callstack); i++) index ^= b->callstack[i];//считаем хэш

	for (;;index++)//да, здесь нет никакой защиты от зависания, но будем надеяться что blockGroups никогда не переполнится
	{
		index &= (LENGTH(ct->blockGroups)-1);//это и для начального преобразования хэша в индекс и для зацикливания

		DWORD *c = ct->blockGroups[index].callstack;
		if (memcmp(c, b->callstack, sizeof(b->callstack)) == 0) break;//нашли!
		if (c[0] == 0)//это пустой элемент, можем добавлять
		{
			memcpy(c, b->callstack, sizeof(b->callstack));
			break;
		}
	}

	ct->blockGroups[index].blocksSize += size;
	ct->blockGroups[index].blocks -= ((size >> 30) & 2) - 1;//эквивалентно size > 0 ? -1 : 1
}
#else
#define UpdateBlockGroup(b, ct, size)
#endif


#ifdef MB_THREADSAFE
namespace {
	Context *contextOverride = NULL;
	MBMTMode mtMode = mbmtmTLS;
	CRITICAL_SECTION critSection;
	bool critSectionInitialized = false;

	class CritSectionAutoLeave
	{
		bool enterCS;

	public:
		CritSectionAutoLeave(Context *&ct) : enterCS(false)
		{
			if (!ct->forceTLSmode)
				if (contextOverride)
				{
					ct = contextOverride;
					if (mtMode == mbmtmCS) EnterCriticalSection(&critSection), enterCS=true;
				}
		}

		~CritSectionAutoLeave()
		{
			if (enterCS) LeaveCriticalSection(&critSection);
		}
	};
}
#endif


int GetIndexFromSize(int size)//возвращает индекс по требуемому размеру блока
{
	//ASSERT(size > 0);
	int alignMask = MBALLOC_ALIGNMENT-1;
	//if (size & alignMask) alignMask = ptrSize-1;//чтобы при запросе 20 байт с MBALLOC_ALIGNMENT=16 выделилось 24, а не 32 байта, т.к. очевидно, что раз запрашиваемый размер не выровнен, то и сам блок можно не выравнивать
	size = (size + sizeof(BlockHeader) + alignMask) & ~alignMask;//размер блоков выделяется с учетом размера заголовка, т.к. иначе в мегаблоке на 128К поместится лишь 3 блока на 32К из-за заголовка блока, оверхед 25%
	int index;
	_BitScanReverse((DWORD*)&index, (size-1)|1);//|1 необходимо т.к. если передать bsr 0, то в index оказывается какая-то чушь, а |1 дает гарантию, что по крайней мере 0-й бит будет единичный
#if SUBPOWER_OF_TWO_BLOCK_SIZE_ACCURACY == 0
	return index + 1;
#else
	return (index<<SUBPOWER_OF_TWO_BLOCK_SIZE_ACCURACY)
		+ (((size-1) >> (index-SUBPOWER_OF_TWO_BLOCK_SIZE_ACCURACY)) & ((1<<SUBPOWER_OF_TWO_BLOCK_SIZE_ACCURACY)-1)) + 1;
#endif
}


int GetSizeFromIndex(int index)//возвращает размер блоков (вместе с заголовком) для заданного индекса
{
#if SUBPOWER_OF_TWO_BLOCK_SIZE_ACCURACY == 0
	return 1 << index;
#else
	int baseShift = (index>>SUBPOWER_OF_TWO_BLOCK_SIZE_ACCURACY);
	return (1 << baseShift) | ((index & ((1<<SUBPOWER_OF_TWO_BLOCK_SIZE_ACCURACY)-1)) << (baseShift-SUBPOWER_OF_TWO_BLOCK_SIZE_ACCURACY));
#endif
}


#ifdef DUMP_ALLOC_CALL_STACK
#pragma comment (lib, "dbghelp.lib")
#include <DbgHelp.h>
#include <stdio.h>

static void *MBAlloc_(int size, Context *ct);
void *MB_CALL MBAlloc(int size)
{
	Context *ct = &context;
#ifdef MB_THREADSAFE
	CritSectionAutoLeave al(ct);
#endif

	BlockHeader *b = (BlockHeader*)MBAlloc_(size, ct) - 1;

	//Т.к. StackWalk64 тормозит и глючит, пришлось написать свой сверхбыстрый граббер call-стека
	try {
	DWORD *p;
	_asm mov [p], ebp
	//p = (DWORD*)p[0];//skip 1 callstack frame
	for (int i=0; i<LENGTH(b->callstack); i++, p = (DWORD*)p[0])
		b->callstack[i] = p[1];
	} catch (...) {}//это предотвратит вылет при выходе за границу стека, но только если в настройках компиляции включить /EHa (Enable C++ Exceptions: Yes With SEH Exceptions)

	UpdateBlockGroup(b, ct, MBMemSize(b+1));

	return b + 1;
}

static void *MBAlloc_(int size, Context *ct)
{
#else
void *MB_CALL MBAlloc(int size)
{
	Context *ct = &context;
#ifdef MB_THREADSAFE
	CritSectionAutoLeave al(ct);//это нельзя перенести ниже, т.к. напр. sysAllocatedBlocks может меняться из разных потоков
#endif
#endif

	if (size > MAX_BLOCK_SIZE-sizeof(BlockHeader))//выделяем память у системы
	{
		BlockHeader *p = (BlockHeader*)SysAlloc(size+sizeof(BlockHeader));
		p->megaBlock = NULL;
		ct->sysAllocatedBlocks++;
		ct->sysAllocatedBlocksSize += size;
		return p + 1;
	}

	if (size < 1) size = 1;//fix for MBAlloc(0)/new [0]
	int index = GetIndexFromSize(size);

	PartlyFreeMegaBlock *pfmb = &ct->partlyFreeMegaBlocks[index];

	if (MegaBlock *mb = pfmb->first)//1. Смотрим свободные блоки в мегаблоках
	{
		mb->numOfAllocatedBlocks++;
		void *r = (BlockHeader*)mb->firstBlock + 1;
		mb->firstBlock = mb->firstBlock->nextFreeBlock;
		if (mb->firstBlock == NULL)//в мегаблоке не осталось места - помещаем его в список заполненных блоков
		{
			ASSERT(mb->numOfAllocatedBlocks == MEGABLOCK_SIZE/GetSizeFromIndex(index));

			if (mb->next) mb->next->prev = mb->prev;//удаляем из списка частично-свободных мегаблоков
			pfmb->first = mb->next;//проверка на mb->prev не нужна, т.к. mb - первый мегаблок в списке и mb->prev == NULL

			mb->prev = NULL;//помещаем в список заполненных
			mb->next = ct->filledMegaBlock;
			if (ct->filledMegaBlock) ct->filledMegaBlock->prev = mb;
			ct->filledMegaBlock = mb;
		}
		return r;
	}

	if (MegaBlock *mb = pfmb->justAllocated)//2. Только что выделенные (еще не до конца освоенные) мегаблоки
	{
		//mb->numOfAllocatedBlocks++;//это не нужно, т.к. только что выделенный блок считается заполненным
		BlockHeader *r = (BlockHeader*)pfmb->nextFreeBlock;
		pfmb->nextFreeBlock += GetSizeFromIndex(index);
		if (--pfmb->blocksLeft == 0)//место закончилось
		{
			pfmb->justAllocated = NULL;
		}
		r->megaBlock = mb;
		return r + 1;
	}

	MegaBlock *mb;
	if (ct->freeMegaBlock)//3. Смотрим в неразмеченных блоках
	{
		mb = ct->freeMegaBlock;
		ct->freeMegaBlock = mb->next;
	}
	else//4. Ничего не остается, кроме как запрашивать память у системы
	{
		mb = (MegaBlock*)SysAlloc(MEGABLOCK_SIZE+MEGABLOCK_HEADER_SIZE);
	}

	int blockSize = GetSizeFromIndex(index);
	mb->firstBlock = NULL;
	mb->index = index;
	mb->numOfAllocatedBlocks = MEGABLOCK_SIZE/blockSize;
#ifdef MB_THREADSAFE
	mb->context = ct;
#endif

	//Помещаем мегаблок в список заполненных мегаблоков
	mb->prev = NULL;
	mb->next = ct->filledMegaBlock;
	if (ct->filledMegaBlock) ct->filledMegaBlock->prev = mb;
	ct->filledMegaBlock = mb;

	pfmb->justAllocated = mb;
	pfmb->blocksLeft = mb->numOfAllocatedBlocks - 1;//форматируем мегаблок на новый размер

	BlockHeader *r = (BlockHeader*)((char*)mb + MEGABLOCK_HEADER_SIZE);//данные (т.е. блоки) начинаются после заголовка мегаблока с учетом выравнивания
	pfmb->nextFreeBlock = (char*)r + blockSize;
	r->megaBlock = mb;

	return r + 1;
}


void MB_CALL MBFree(void *p)
{
	ASSERT(p);//ибо нефиг!

	BlockHeader *b = (BlockHeader*)p - 1;

	Context *ct = &context;
#ifdef MB_THREADSAFE
	if (MegaBlock *mb = b->megaBlock) ct = mb->context;
	CritSectionAutoLeave al(ct);
#endif

	UpdateBlockGroup(b, ct, -MBMemSize(p));

	if (MegaBlock *mb = b->megaBlock)
	{
#ifndef NDEBUG
		memset(p, 0xAA, GetSizeFromIndex(mb->index) - sizeof(BlockHeader));//заполняем память мусором, только если это не системная аллокация, т.к. free в дебаге все равно заполнит память 0xDD
#ifdef MB_THREADSAFE
		if (mtMode == mbmtmTLS || context.forceTLSmode)
			ASSERT(mb->context == &context);//в режиме TLS нельзя освобождать память, выделенную в другом потоке
#endif
#endif

		if (--mb->numOfAllocatedBlocks > 0)//есть еще.. порох в пороховницах!... мм, т.е. выделенные блоки
		{
			if (mb->firstBlock == NULL)//мегаблок находится в списке полностью заполненных блоков
			{
				//Удаляем мегаблок из списка заполненных блоков
				if (mb->next) mb->next->prev = mb->prev;
				if (mb->prev) mb->prev->next = mb->next; else ct->filledMegaBlock = mb->next;

				//Помещаем в список частично-свободных мегаблоков
				PartlyFreeMegaBlock *pfmb = &ct->partlyFreeMegaBlocks[mb->index];
				mb->prev = NULL;
				mb->next = pfmb->first;
				if (pfmb->first) pfmb->first->prev = mb;
				pfmb->first = mb;
			}

			((FreeBlockHeader*)b)->nextFreeBlock = mb->firstBlock;
			mb->firstBlock = (FreeBlockHeader*)b;
		}
		else//мегаблок свободен - помещаем его в список неразмеченных блоков
		{
			//Сначала удаляем мегаблок из списка частично-свободных мегаблоков (partlyFreeMegaBlocks)
			/*if (mb->firstBlock == NULL)//мегаблок находится в списке полностью заполненных блоков, а не частично-свободных (если это раскомментировать, а также строчку mb->numOfAllocatedBlocks++ и поменять mb->numOfAllocatedBlocks = MEGABLOCK_SIZE/blockSize на mb->numOfAllocatedBlocks = 1, то при освобождении последнего блока в только что выделенном мегаблоке, мегаблок будет освобожден)
			{
				if (mb->next) mb->next->prev = mb->prev;
				if (mb->prev) mb->prev->next = mb->next; else filledMegaBlock = mb->next;
			}
			else
			{*/
			if (mb->next) mb->next->prev = mb->prev;
			if (mb->prev) mb->prev->next = mb->next; else ct->partlyFreeMegaBlocks[mb->index].first = mb->next;
			//}

			mb->next = ct->freeMegaBlock;
			//mb->prev = NULL;//это не нужно, т.к. в списке freeMegaBlock поле prev у мегаблоков не используется
			//ct->freeMegaBlock->prev = mb;
			ct->freeMegaBlock = mb;
		}
	}
	else
	{
		ct->sysAllocatedBlocks--;
		ct->sysAllocatedBlocksSize -= MBMemSize(p);
		SysFree(b);
	}
}


int MB_CALL MBMemSize(void *p)
{
	ASSERT(p);
	BlockHeader *b = (BlockHeader*)p - 1;
	return (b->megaBlock ? GetSizeFromIndex(b->megaBlock->index) : SysMemSize(b)) - sizeof(BlockHeader);
}


void *MB_CALL MBRealloc(void *p, int size)
{
	if (p)
	{
		int blockSize = MBMemSize(p);
		if (size <= blockSize) return p;//делать ничего не надо
		void *pp = MBAlloc(size);
		memcpy(pp, p, blockSize);
		MBFree(p);
		return pp;
	}
	else return MBAlloc(size);
}


void MB_CALL MBFreeContext()//Функция очищает TL-контекст, ассоциированный с данным потоком независимо от текущего MT-режима
{
#ifdef DETECT_MEMORY_LEAKS
	if (context.filledMegaBlock) goto leak_detected;//в этом списке никогда не окажется пустых блоков - это либо заполненные, либо только что аллоцированные не пустые блоки

	for (int i=0; i<LENGTH(context.partlyFreeMegaBlocks); i++)
	{
		PartlyFreeMegaBlock &pfmb = context.partlyFreeMegaBlocks[i];
		if (pfmb.first)
			if (pfmb.first == pfmb.justAllocated && pfmb.first->next == NULL)//допустима только одна ситуация - блок в списке один, он только что выделен и при этом все его аллоцированные блоки были освобождены
			{
				if (pfmb.first->numOfAllocatedBlocks != pfmb.blocksLeft) goto leak_detected;
			}
			else goto leak_detected;
	}

	goto no_leaks;
leak_detected:
	MessageBoxA(NULL, "Memory leak detected!", "MB Allocator", MB_OK|MB_SYSTEMMODAL);

#ifdef DUMP_ALLOC_CALL_STACK
	SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES);
	static HANDLE hProcess = GetCurrentProcess();
	SymInitialize(hProcess, NULL, TRUE);
	OutputDebugStringA("<-------------- !!! MEMORY LEAKS (MBALLOC) !!! -------------->\n");

	struct Dump
	{
		static void dumpLeakedBlock(char *block)
		{
			BlockHeader *b = (BlockHeader*)block;

			for (int i=0; i<LENGTH(b->callstack); i++)
			{
				//callstack есть, нужно только зарезолвить файл и строку, соответствующую адресу кода для данного уровня стека
				IMAGEHLP_LINE Line = {0};
				Line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

				DWORD LineDisplacement = 0;
				if (!SymGetLineFromAddr(hProcess, b->callstack[i]-1, &LineDisplacement, &Line)) break;//-1 нужно, т.к. сохраненный адрес - адрес возврата, т.е. адрес, следующий сразу за инструкцией call
				if (Line.FileName)
				{
					char str[500];
					//Line.FileName = strrchr(Line.FileName,'\\')+1;//оставляем только имя файла, усекая путь
					sprintf_s(str, sizeof(str), "%s(%i) : stack frame %i\n", Line.FileName, Line.LineNumber, i);
					OutputDebugStringA(str);
				}
			}

			OutputDebugStringA("--------------------------------------------------------------\n");
		}
	};

	for (MegaBlock *mb=context.filledMegaBlock; mb; mb=mb->next)
	{
		int blockSize = GetSizeFromIndex(mb->index);
		PartlyFreeMegaBlock &pfmb = context.partlyFreeMegaBlocks[mb->index];
		char *block = (char*)(mb+1), *end;
		if (mb == pfmb.justAllocated) end = pfmb.nextFreeBlock; else end = block + mb->numOfAllocatedBlocks*blockSize;

		for (; block<end; block+=blockSize) Dump::dumpLeakedBlock(block);
	}

	for (int b=0; b<LENGTH(context.partlyFreeMegaBlocks); b++)
	{
		PartlyFreeMegaBlock &pfmb = context.partlyFreeMegaBlocks[b];
		for (MegaBlock *mb=pfmb.first; mb; mb=mb->next)
		{
			int blockSize = GetSizeFromIndex(mb->index);
			int blocks = MEGABLOCK_SIZE/blockSize;//кол-во блоков в данном мегаблоке
			if (mb == pfmb.justAllocated)
				if (mb->numOfAllocatedBlocks == pfmb.blocksLeft) continue;//выполнение этого условия означает, что в данном мегаблоке нет выделенных блоков
				else blocks -= pfmb.blocksLeft;

			bool blockFree[MEGABLOCK_SIZE/(sizeof(BlockHeader)+ptrSize)];
			memset(blockFree, 0, blocks);

			char *blockBasis = (char*)(mb+1);
			for (FreeBlockHeader *fbh = mb->firstBlock; fbh; fbh = fbh->nextFreeBlock)
			{
				int i = ((char*)fbh-blockBasis) / blockSize;
				ASSERT(unsigned(i) < unsigned(blocks));
				blockFree[i] = true;//помечаем освобожденные блоки
			}

			for (int i=0; i<blocks; i++)//оставшиеся блоки - и есть утечка
				if (!blockFree[i]) Dump::dumpLeakedBlock(blockBasis+i*blockSize);
		}
	}

	SymCleanup(hProcess);
	OutputDebugStringA("<-------------- !!! END    LEAKS (MBALLOC) !!! -------------->\n");
#endif
no_leaks:
#endif

	for (int i=0; i<LENGTH(context.partlyFreeMegaBlocks); i++)
		for (MegaBlock *mb=context.partlyFreeMegaBlocks[i].first; mb;)
		{
			MegaBlock *next = mb->next;
			SysFree(mb);
			mb = next;
		}
	memset(context.partlyFreeMegaBlocks, 0, sizeof(context.partlyFreeMegaBlocks));

	for (MegaBlock *mb=context.freeMegaBlock; mb;)
	{
		MegaBlock *next = mb->next;
		SysFree(mb);
		mb = next;
	}
	context.freeMegaBlock = NULL;

	for (MegaBlock *mb=context.filledMegaBlock; mb;)
	{
		MegaBlock *next = mb->next;
		SysFree(mb);
		mb = next;
	}
	context.filledMegaBlock = NULL;
}


void MB_CALL MBFreeMainContext()
{
	MBFreeContext();

#ifdef MB_THREADSAFE
	if (critSectionInitialized)
		DeleteCriticalSection(&critSection), critSectionInitialized = false;
#endif
}


void MB_CALL MBGetMemoryStats(MBMemoryStats &stats)
{
	memset(&stats, 0, sizeof(stats));

	int startIndex = GetIndexFromSize(0);
	stats.allocatedBlocksLength = GetIndexFromSize(MAX_BLOCK_SIZE-sizeof(BlockHeader)) - startIndex + 1;
	ASSERT(stats.allocatedBlocksLength <= LENGTH(stats.allocatedBlocks));

	for (int i=0; i<stats.allocatedBlocksLength; i++)
	{
		stats.allocatedBlocks[i].size = GetSizeFromIndex(i+startIndex) - sizeof(BlockHeader);

		PartlyFreeMegaBlock &pfmb = context.partlyFreeMegaBlocks[i+startIndex];
		for (MegaBlock *mb=pfmb.first; mb; mb=mb->next)
		{
			stats.partlyFreeMegaBlocks++;

			stats.allocatedBlocks[i].count += mb->numOfAllocatedBlocks;
			if (mb == pfmb.justAllocated)
			{
				if (mb->numOfAllocatedBlocks == pfmb.blocksLeft) stats.potentiallyFreeMegaBlocks++;
				stats.allocatedBlocks[i].count -= pfmb.blocksLeft;
			}
		}
	}

	for (MegaBlock *mb=context.freeMegaBlock; mb; mb=mb->next) stats.freeMegaBlocks++;

	for (MegaBlock *mb=context.filledMegaBlock; mb; mb=mb->next)
	{
		stats.filledMegaBlocks++;

		ASSERT(mb->numOfAllocatedBlocks == MEGABLOCK_SIZE/GetSizeFromIndex(mb->index));//в заполненном магаблоке это условие всегда должно выполняться
		int i = mb->index - startIndex;
		ASSERT(unsigned(i) < unsigned(stats.allocatedBlocksLength));
		stats.allocatedBlocks[i].count += mb->numOfAllocatedBlocks;
		PartlyFreeMegaBlock &pfmb = context.partlyFreeMegaBlocks[mb->index];
		if (mb == pfmb.justAllocated) stats.allocatedBlocks[i].count -= pfmb.blocksLeft;
	}

	stats.totalMegaBlocks = stats.partlyFreeMegaBlocks + stats.freeMegaBlocks + stats.filledMegaBlocks;
	stats.totalMegaBlocksSize = stats.totalMegaBlocks * (stats.megaBlockSize = (MEGABLOCK_SIZE+MEGABLOCK_HEADER_SIZE));

	//Наконец считаем нетто и брутто
	for (int i=0; i<stats.allocatedBlocksLength; i++)
	{
		stats.netAllocatedBlocksSize += stats.allocatedBlocks[i].size * stats.allocatedBlocks[i].count;
		stats.grossAllocatedBlocksSize += (stats.allocatedBlocks[i].size+sizeof(BlockHeader)) * stats.allocatedBlocks[i].count;
		stats.totalAllocatedBlocks += stats.allocatedBlocks[i].count;//также здесь считаем общее кол-во блоков
	}

	stats.fragmentationDegree = 100.f - float(stats.grossAllocatedBlocksSize*100./((stats.partlyFreeMegaBlocks+stats.filledMegaBlocks)*MEGABLOCK_SIZE));

	stats.sysAllocatedBlocks     = context.sysAllocatedBlocks;
	stats.sysAllocatedBlocksSize = context.sysAllocatedBlocksSize;
}


int MB_CALL MBGetBlockGroups(MBBlockGroup *arr, int arrLength)
{
	arr; arrLength;//unwarning
	int r = 0;
#ifdef GATHER_BLOCK_GROUPS_STATS
	Context *ct = &context;
#ifdef MB_THREADSAFE
	CritSectionAutoLeave al(ct);
#endif
	for (int i=0; i<LENGTH(ct->blockGroups); i++)
		if (ct->blockGroups[i].blocks)//возвращаем только те группы, в которых есть блоки
		{
			memcpy(arr+r++, &ct->blockGroups[i], sizeof(MBBlockGroup));
			if (r == arrLength) break;
		}
#endif
	return r;
}


#ifdef MB_THREADSAFE
/*По умолчанию MultiThreading включен в режиме TLS, но если нужна возможность удалять блоки из разных потоков, то код,
  использующий MBAlloc (напр. работа с объектами, вызывающими MBAlloc неявно через перегруженный new) нужно вручную
  обернуть в крит. секцию, а до выполнения этого кода, в ОСНОВНОМ потоке необходимо вызвать
  MBSwitchMTMode(mbmtmAppControlled), по окончании работы потоков с этим кодом, нужно вызвать MBSwitchMTMode(mbmtmTLS).
  Если могут существовать какие-то потоки, в которых вызовы MBAlloc/MBFree не обернуты в крит. секцию, то
  стоит воспользоваться самым простым и надежным режимом mbmtmCS, который нужно включить на время работы этих потоков.
  ПРИМЕЧАНИЕ: В режимах mbmtmCS и mbmtmAppControlled память может быть корректно удалена из любого потока, даже если
  до вызова MBSwitchMTMode(mbmtmCS) уже существовали потоки, в которых произошло выделение памяти, и которые продолжают
  существовать после этого вызова. Однако при возвращении в режим mbmtmTLS, память выделенная в другом режиме не может
  быть освобождена из других потоков, кроме основного!
*/
MBMTMode MB_CALL MBSwitchMTMode(MBMTMode mode)
{
	switch (mode)
	{
	case mbmtmTLS:
		contextOverride = NULL;
		break;
	case mbmtmAppControlled:
		contextOverride = &context;//переопределяем TL-контекст на контекст из основного потока (очевидно, функция должна вызываться из основного потока)
		break;
	case mbmtmCS:
		contextOverride = &context;
		if (!critSectionInitialized)
		{
			InitializeCriticalSectionAndSpinCount(&critSection, 5000);
			critSectionInitialized = true;
		}
		break;
	}

	MBMTMode prevMTMode = mtMode;
	mtMode = mode;
	return prevMTMode;
}

/*Переопределяет глобальный режим для текущего потока, устанавливая его в TLS навсегда.
  Нужно вызывать эту функцию в начале каждого потока, для которого не предполагается изменять режим, т.е. для таких потоков
  которые не могут работать с общими данными. Это нужно для избежания конфликтов аллокации после переключения режимов (особенно,
  при возвращении в режим mbmtmTLS), чтобы переключение режимов не влияло на такие потоки.
*/
void MB_CALL MBForceTLSMode()
{
	context.forceTLSmode = true;
}
#endif


#ifdef OPERATOR_NEW_OVERLOAD
#include <new>
#ifndef NDEBUG
#define OPERATOR_NEW_BODY(is_array) BlockHeader *b = (BlockHeader*)MBAlloc(size) - 1; b->isArray = is_array ? BlockHeader::ARRAY_SIGN : BlockHeader::NONARRAY_SIGN; return b + 1;
#else
#define OPERATOR_NEW_BODY(is_array) return MBAlloc(size);
#endif
void* _cdecl operator new(size_t size)							{OPERATOR_NEW_BODY(false)}
void* _cdecl operator new(size_t size, const std::nothrow_t&)	{OPERATOR_NEW_BODY(false)}
void* _cdecl operator new[](size_t size)						{OPERATOR_NEW_BODY(true)}
void* _cdecl operator new[](size_t size, const std::nothrow_t&)	{OPERATOR_NEW_BODY(true)}

void _cdecl operator delete(void* p)							{if (p) {ASSERT(((BlockHeader*)p-1)->isArray == BlockHeader::NONARRAY_SIGN); MBFree(p);}}
void _cdecl operator delete(void* p, const std::nothrow_t&)		{if (p) {ASSERT(((BlockHeader*)p-1)->isArray == BlockHeader::NONARRAY_SIGN); MBFree(p);}}
void _cdecl operator delete[](void* p)							{if (p) {ASSERT(((BlockHeader*)p-1)->isArray == BlockHeader::   ARRAY_SIGN); MBFree(p);}}
void _cdecl operator delete[](void* p, const std::nothrow_t&)	{if (p) {ASSERT(((BlockHeader*)p-1)->isArray == BlockHeader::   ARRAY_SIGN); MBFree(p);}}
#endif
