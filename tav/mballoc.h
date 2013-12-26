#ifndef MBALLOC_H
#define MBALLOC_H

#define MB_THREADSAFE//~10% оверхеда в самых быстрых режимах (TLS или AppControlled), в 2.5 раза медленнее в режиме mbmtmCS
#define MB_CALL __fastcall
//#define OPERATOR_NEW_OVERLOAD
//#define DETECT_MEMORY_LEAKS


extern void *MB_CALL MBAlloc(int size);
extern void *MB_CALL MBRealloc(void *p, int size);
extern int   MB_CALL MBMemSize(void *p);
extern void  MB_CALL MBFree(void *p);
extern void  MB_CALL MBFreeMainContext();//эту функцию нужно вызывать в основном потоке
extern void  MB_CALL MBFreeContext();//а эту - во всех дополнительных потоках

struct MBMemoryStats
{
	struct AB {int size, count;} allocatedBlocks[128];//информация по выделенным блокам (размер блока, кол-во блоков)
	int allocatedBlocksLength;//размер массива allocatedBlocks
	int totalAllocatedBlocks;//общее количество выделенных аллокатором блоков
	int partlyFreeMegaBlocks, freeMegaBlocks, filledMegaBlocks;//количество мегаблоков различных типов
	int totalMegaBlocks;//всего мегаблоков выделено = сумме пред. 3-х полей
	int totalMegaBlocksSize;//вся память, выделенная аллокатором
	int megaBlockSize;//полный размер одного мегаблока, включая заголовок
	int potentiallyFreeMegaBlocks;//потенциально свободные мегаблоки (те, которые находятся в списке частично свободных, но реально они полностью свободны)
	int netAllocatedBlocksSize;//размер выделенных приложением блоков без учета служебной информации аллокатора (заголовков блоков напр.)
	int grossAllocatedBlocksSize;//размер выделенных приложением блоков с учетом служебной информации аллокатора, отношение этого параметра к totalMegaBlocksSize можно рассматривать как общий КПД аллокатора по расходу памяти
	float fragmentationDegree;//приблизительная степень фрагментации памяти от 0 до 100% (чем больше, тем хуже)
	int sysAllocatedBlocks, sysAllocatedBlocksSize;//кол-во и суммарный размер неконтролируемых аллокатором (системных) блоков
};
extern void MB_CALL MBGetMemoryStats(MBMemoryStats &stats);//получает статистику по выделенной памяти только для текущего потока (учитываются только блоки размером до MAX_BLOCK_SIZE)

struct MBBlockGroup
{
	unsigned long callstack[4];
	int blocks, blocksSize;
};
extern int MB_CALL MBGetBlockGroups(MBBlockGroup *arr, int arrLength);//принимает указатель на буфер и кол-во элементов в нем; возвращает кол-во групп блоков, записанных в буфер
//снаружи можно сортировать блоки по размеру, брать первую сотню групп блоков, затем по адресу фреймов стека определять соотв-ю файл/строку, еще можно завести хэш для этого сопоставления, т.к. SymGetLineFromAddr не очень быстрая

#ifdef MB_THREADSAFE
enum MBMTMode
{
	mbmtmTLS = 0,
	mbmtmAppControlled,//нужно вручную оборачивать в крит. секции вызовы MBAlloc/MBFree
	mbmtmCS,
};
extern MBMTMode MB_CALL MBSwitchMTMode(MBMTMode mode = mbmtmTLS);
extern void MB_CALL MBForceTLSMode();
#endif

#endif
