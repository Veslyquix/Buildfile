#include "ModularSave.h"

void* MS_GetSaveAddressBySlot(unsigned slot) {
	if (slot > 7) // 	SAVE_BLOCK_UNK6       = 6,
		return NULL;

	return (void*)(0xE000000) + gSaveBlockDecl[slot].offset;
}

const struct SaveChunkDecl* MS_FindGameSaveChunk(unsigned chunkId) {
	for (const struct SaveChunkDecl* chunk = gGameSaveChunks; chunk->offset != 0xFFFF; ++chunk)
		if (chunk->identifier == chunkId)
			return chunk;

	return NULL;
}

const struct SaveChunkDecl* MS_FindSuspendSaveChunk(unsigned chunkId) {
	for (const struct SaveChunkDecl* chunk = gSuspendSaveChunks; chunk->offset != 0xFFFF; ++chunk)
		if (chunk->identifier == chunkId)
			return chunk;

	return NULL;
}

void MS_LoadChapterStateFromGameSave(unsigned slot, struct ChapterState* target) {
	void* const source = GetSaveSourceAddress(slot);
	const struct SaveChunkDecl* const chunk = MS_FindGameSaveChunk(gMS_ChapterStateChunkId);

	ReadSramFast(source + chunk->offset, target, chunk->size);
}

void MS_LoadChapterStateFromSuspendSave(unsigned slot, struct ChapterState* target) {
	void* const source = GetSaveSourceAddress(slot);
	const struct SaveChunkDecl* const chunk = MS_FindSuspendSaveChunk(gMS_ChapterStateChunkId);

	ReadSramFast(source + chunk->offset, target, chunk->size);
}

u32 MS_GetClaimFlagsFromGameSave(unsigned slot) {
	u32 buf;

	void* const source = GetSaveSourceAddress(slot);
	const struct SaveChunkDecl* const chunk = MS_FindGameSaveChunk(gMS_ClaimFlagsChunkId);

	ReadSramFast(source + chunk->offset, &buf, 4);

	return buf;
}

// TODO: use eventual proper WMData struct definition, or something
void MS_LoadWMDataFromGameSave(unsigned slot, void* target) {
	void* const source = GetSaveSourceAddress(slot);
	const struct SaveChunkDecl* const chunk = MS_FindGameSaveChunk(gMS_WMDataChunkId);

	LoadWMStuff(source + chunk->offset, target);
}

int MS_CheckEid8AFromGameSave(unsigned slot) {
	void* const source = GetSaveSourceAddress(slot);
	const struct SaveChunkDecl* const chunk = MS_FindGameSaveChunk(gMS_PermanentEidsChunkId);

	// TODO: fix this mess
	ReadSramFast(source + chunk->offset, gGenericBuffer, chunk->size);
	return ((u8(*)(unsigned eid, void* buf))(0x08083D34+1))(0x8A, gGenericBuffer);
}

//extern int UnpackUnitsFromBox(int slot);
//extern void PackUnitsIntoBox(int slot);
//extern void ClearPCBoxUnitsBuffer(void);
//extern void RelocateUnitsPastThreshold(int); 

void MS_CopyGameSave(int sourceSlot, int targetSlot) {
	void* const source = GetSaveSourceAddress(sourceSlot);
	void* const target = GetSaveTargetAddress(targetSlot);

	unsigned size = gSaveBlockTypeSizeLookup[SAVE_TYPE_GAME];

	ReadSramFast(source, gGenericBuffer, size);
	WriteAndVerifySramFast(gGenericBuffer, target, size);
	
	//UnpackUnitsFromBox(sourceSlot);
	//PackUnitsIntoBox(targetSlot); 

	struct SaveBlockMetadata sbm;

	sbm.magic1 = SBM_MAGIC1_GAME;
	sbm.type   = SAVE_TYPE_GAME;

	SaveMetadata_Save(&sbm, targetSlot);
}


void MS_SaveGame(unsigned slot) {
	void* const base = GetSaveTargetAddress(slot);

	// Clear suspend
	ClearSaveBlock(SAVE_BLOCK_SUSPEND);

	// Update save slot index
	gChapterData.saveSlotIndex = slot;

	// Actual save!
	for (const struct SaveChunkDecl* chunk = gGameSaveChunks; chunk->offset != 0xFFFF; ++chunk)
		if (chunk->save)
			chunk->save(base + chunk->offset, chunk->size);



	//PackUnitsIntoBox(slot);
	//ClearPCBoxUnitsBuffer();
	//int index = UnpackUnitsFromBox(slot);
	//RelocateUnitsPastThreshold(index); 
	//
	//PackUnitsIntoBox(slot); 
	
	
	// Setup block metadata

	struct SaveBlockMetadata sbm;

	sbm.magic1 = SBM_MAGIC1_GAME;
	sbm.type   = SAVE_TYPE_GAME;

	SaveMetadata_Save(&sbm, slot);

	// Update save slot in global metadata
	UpdateLastUsedGameSaveSlot(slot);
}

void MS_LoadGame(unsigned slot) {
	void* const base = GetSaveSourceAddress(slot);

	if (!(gChapterData.chapterStateBits & 0x40))
		// Clear suspend if not loading for link arena
		ClearSaveBlock(SAVE_BLOCK_SUSPEND);

	// Actual load!
	for (const struct SaveChunkDecl* chunk = gGameSaveChunks; chunk->offset != 0xFFFF; ++chunk)
		if (chunk->load)
			chunk->load(base + chunk->offset, chunk->size);


	//UnpackUnitsFromBox(slot);
	
	
	// Update save slot in global metadata
	UpdateLastUsedGameSaveSlot(slot);
}

void MS_SaveSuspend(unsigned slot) {
	if (gChapterData.chapterStateBits & 8)
		return;

	if (!IsSramWorking())
		return;

	void* const base = GetSaveTargetAddress(slot);

	// Actual save!
	for (const struct SaveChunkDecl* chunk = gSuspendSaveChunks; chunk->offset != 0xFFFF; ++chunk)
		if (chunk->save)
			chunk->save(base + chunk->offset, chunk->size);

	// Setup block metadata

	struct SaveBlockMetadata sbm;

	sbm.magic1 = SBM_MAGIC1_GAME;
	sbm.type   = SAVE_TYPE_SUSPEND;

	SaveMetadata_Save(&sbm, slot);

	gGameState.boolHasJustResumed = FALSE;
}

void MS_LoadSuspend(unsigned slot) {
	void* const base = GetSaveSourceAddress(slot);

	for (const struct SaveChunkDecl* chunk = gSuspendSaveChunks; chunk->offset != 0xFFFF; ++chunk)
		if (chunk->load)
			chunk->load(base + chunk->offset, chunk->size);
}
