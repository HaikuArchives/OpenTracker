/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software..

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include "FSClipboard.h"
#include <Clipboard.h>
#include <Alert.h>
#include <NodeMonitor.h>
#include "Commands.h"
#include "FSUtils.h"
#include "Tracker.h"

// prototypes
static void MakeNodeFromName(node_ref *node, char *name);
static inline void MakeRefName(char *refname, const node_ref *node);
static inline void MakeModeName(char *modename, const node_ref *node);
static inline void MakeModeNameFromRefName(char *modename, char *refname);
static inline bool CompareModeAndRefName(const char *modename, const char *refname);


//these are from PoseView.cpp
extern const char *kNoCopyToTrashStr;
extern const char *kNoCopyToRootStr;
extern const char *kOkToMoveStr;


static bool
FSClipboardCheckIntegrity()
{
	return true;
}


bool
FSClipboardHasRefs()
{
	bool result = false;

	if (be_clipboard->Lock()) {
		BMessage *clip = be_clipboard->Data();
		if (clip != NULL) {
#ifdef B_BEOS_VERSION_DANO
			const
#endif
			char *refname;
#ifdef B_BEOS_VERSION_DANO
			const
#endif
			char *modename;
			uint32 type;
			int32 count;
			if (clip->GetInfo(B_REF_TYPE, 0, &refname, &type, &count) == B_OK
				&& clip->GetInfo(B_INT32_TYPE, 0, &modename, &type, &count) == B_OK)
				result = CompareModeAndRefName(modename,refname);
		}
		be_clipboard->Unlock();
	}
	return result;
}


static void
MakeNodeFromName(node_ref *node, char *name)
{
	char *nodeString = strchr(name,'_');
	if (nodeString != NULL) {
		node->node = strtoll(nodeString + 1, (char **)NULL, 10);
		node->device = atoi(name + 1);
	}
}


static inline void
MakeRefName(char *refname, const node_ref *node)
{
	sprintf(refname,"r%ld_%Ld",node->device,node->node);
}


static inline void
MakeModeName(char *modename, const node_ref *node)
{
	sprintf(modename,"m%ld_%Ld",node->device,node->node);
}


static inline void
MakeModeName(char *name)
{
	name[0] = 'm';
}


static inline void
MakeModeNameFromRefName(char *modename, char *refname)
{
	strcpy(modename,refname);
	modename[0] = 'm';
}


static inline bool
CompareModeAndRefName(const char *modename, const char *refname)
{
	return !strncmp(refname + 1,modename + 1,strlen(refname + 1));
}


void
FSClipboardClear()
{
	if (!be_clipboard->Lock())
		return;

	be_clipboard->Clear();
	be_clipboard->Commit();
	be_clipboard->Unlock();

	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	BClipboardRefsWatcher *watcher = NULL;
	if (tracker)
		watcher = tracker->ClipboardRefsWatcher();

	if (watcher)
		watcher->Clear();
}


uint32
FSClipboardAddPoses(const node_ref *directory,PoseList *list,uint32 moveMode,bool clearClipboard)
{
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	BClipboardRefsWatcher *watcher = NULL;
	if (tracker)
		watcher = tracker->ClipboardRefsWatcher();

	uint32	refsAdded = 0;
	int32	listSize = list->CountItems();

	if (listSize && be_clipboard->Lock()) {
		if (clearClipboard) {
			be_clipboard->Clear();
			if (watcher)
				watcher->Clear();
		}

		BMessage *clip = be_clipboard->Data();
		if (clip != NULL) {
			char refname[64];
			char modename[64];
			for (int32 index = 0; index < listSize; index++) {
				BPose *pose = (BPose*)list->ItemAt(index);
				Model *model = pose->TargetModel();
				const node_ref *node = model->NodeRef();
				MakeRefName(refname,node);
				MakeModeNameFromRefName(modename,refname);
				
				if (clearClipboard) {
					if (clip->AddInt32(modename,(int32)moveMode) == B_OK)
						if (clip->AddRef(refname,model->EntryRef()) == B_OK) {
							pose->SetClipboardMode(moveMode);
							refsAdded++;
							if (watcher)
								watcher->AddNode((node_ref *)node);
						} else
							clip->RemoveName(modename);
				} else {
					// replace old mode if entry already exists in clipboard
					if (clip->ReplaceInt32(modename,(int32)moveMode) == B_OK) {
						if (clip->ReplaceRef(refname,model->EntryRef()) == B_OK) {
							pose->SetClipboardMode(moveMode);
							refsAdded++;
						} else {
							clip->RemoveName(modename);
							if (watcher)
								watcher->RemoveNode((node_ref *)node);
						}
					// add if it doesn't exist
					} else {
						if ((clip->AddRef(refname,model->EntryRef()) == B_OK) && (clip->AddInt32(modename,(int32)moveMode)) == B_OK) {
							pose->SetClipboardMode(moveMode);
							refsAdded++;
							if (watcher)
								watcher->AddNode((node_ref *)node);
						} else {
							clip->RemoveName(modename);
							clip->RemoveName(refname);
							if (watcher)
								watcher->RemoveNode((node_ref *)node);
						}
					}
				}
			}
			be_clipboard->Commit();
		}	
		be_clipboard->Unlock();
	}

	if (watcher)
		watcher->UpdatePoseViews(clearClipboard,directory);

	return refsAdded;
}


bool
FSClipboardPaste(Model *model)
{
	if (!FSClipboardHasRefs())
		return false;

	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	BClipboardRefsWatcher *watcher = NULL;
	if (tracker)
		watcher = tracker->ClipboardRefsWatcher();

	int32 refsPasted = 0;

	BEntry *destMoveEntry = new BEntry();
	model->GetEntry(destMoveEntry);
	BEntry *destCopyEntry = new BEntry();
	model->GetEntry(destCopyEntry);
	
	node_ref *destNodeRef = (node_ref*)model->NodeRef();

	bool destIsTrash = FSIsTrashDir(destMoveEntry);

	bool okToMove = true;
	int32 moveCount = 0;
	int32 copyCount = 0;

	BObjectList<entry_ref> *srcMoveList = NULL;
	BObjectList<entry_ref> *srcCopyList = NULL;

	if ((be_clipboard->Lock())) {
		BMessage *clip = be_clipboard->Data();
		if (clip != NULL) {
			srcMoveList = new BObjectList<entry_ref>(0, true);
			srcCopyList = new BObjectList<entry_ref>(0, true);

			BEntry checkEntry;
			char modename[64];
			int32 moveMode = 0;

			int32 index = 0;
			char *refname;
			type_code type;
			int32 count;
			while (clip->GetInfo(B_REF_TYPE, index,
#ifdef B_BEOS_VERSION_DANO
				(const char **)
#endif
				&refname, &type, &count) == B_OK) {
				entry_ref *ref = new entry_ref();
				if ((clip->FindRef(refname,ref) == B_OK) && (destNodeRef->node != ref->directory)) {
					MakeModeNameFromRefName(modename,refname);
					if (clip->FindInt32(modename,&moveMode) == B_OK) {
						checkEntry.SetTo(ref);
						if (checkEntry.Exists()) {
							if ((uint32)moveMode == kMoveSelectionTo) {
								srcMoveList->AddItem(ref);
								// now change "Cut" mode to "Copy" mode, to let the user
								// cut only once, next paste will just copy poses (from
								// the new location)
								clip->ReplaceInt32(modename,kCopySelectionTo);
								moveCount++;
							} else if ((uint32)moveMode == kCopySelectionTo) {
								srcCopyList->AddItem(ref);
								copyCount++;
							}
							refsPasted++;
						} else {
							clip->RemoveName(refname);
							clip->RemoveName(modename);
							if (watcher) {
								node_ref node;
								MakeNodeFromName(&node,modename);
								watcher->RemoveNode(&node);
							}
							delete ref;
						}
					} else
						delete ref;
				} else
					delete ref;

				index++;
			}
			be_clipboard->Commit();
		}
		be_clipboard->Unlock();
	}

	// can't copy/paste to root('/') directory
	if (model->IsRoot()) {
		(new BAlert("", kNoCopyToRootStr, "Cancel", NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		okToMove = false;
	}

	// can't copy items into the trash
	if (copyCount && destIsTrash) {
		(new BAlert("", kNoCopyToTrashStr, "Cancel", NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		okToMove = false;
	}
	
	if (okToMove) {
		if (moveCount)
			FSMoveToFolder(srcMoveList, destMoveEntry, kMoveSelectionTo);
		else
			delete srcMoveList;

		if (copyCount)
			FSMoveToFolder(srcCopyList, destCopyEntry, kCopySelectionTo);
		else
			delete srcCopyList;

		return true;
	}

	delete destMoveEntry;
	delete destCopyEntry;
	
	return false;
}


/**	Seek node in clipboard, if found return it's moveMode
 *	else return 0
 */

uint32
FSClipboardFindNodeMode(Model *model, bool updateRefIfNeeded)
{
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	BClipboardRefsWatcher *watcher = NULL;
	if (tracker)
		watcher = tracker->ClipboardRefsWatcher();

	int32 moveMode = 0;

	if (be_clipboard->Lock()) {
		BMessage *clip = be_clipboard->Data();
		if (clip != NULL) {
			const node_ref *node = model->NodeRef();
			char modename[64];
			MakeModeName(modename,node);
			if ((clip->FindInt32(modename,&moveMode) == B_OK)) {
				const entry_ref *ref = model->EntryRef();
				entry_ref clipref;
				char refname[64];
				MakeRefName(refname,node);
				if ((clip->FindRef(refname,&clipref) == B_OK)) {
					if (clipref != *ref) {
						if (updateRefIfNeeded) {
							clip->ReplaceRef(refname,ref);
							be_clipboard->Commit();
						} else {
							clip->RemoveName(refname);
							clip->RemoveName(modename);
							be_clipboard->Commit();
							if (watcher)
								watcher->RemoveNode((node_ref *)node);
							moveMode = 0;
						}
					}
				} else {
					clip->RemoveName(modename);
					be_clipboard->Commit();
					if (watcher)
						watcher->RemoveNode((node_ref *)node);
					moveMode = 0;
				}
			}
		}
		be_clipboard->Unlock();
	}
	
	return (uint32)moveMode;
}


//	#pragma mark -


BClipboardRefsWatcher::BClipboardRefsWatcher()
	:	BLooper("ClipboardRefsWatcher", B_LOW_PRIORITY, 4096),
	fNotifyList(10, false)
{
	watch_node(NULL,B_WATCH_MOUNT,this);
	fRefsInClipboard = FSClipboardHasRefs();
	be_clipboard->StartWatching(this);
}


BClipboardRefsWatcher::~BClipboardRefsWatcher()
{
	stop_watching(this);
	be_clipboard->StopWatching(this);
}


void
BClipboardRefsWatcher::AddToNotifyList(BPoseView *view)
{
	if (Lock()) {
		if (!fNotifyList.HasItem(view))
			fNotifyList.AddItem(view);
		Unlock();
	}
}


void
BClipboardRefsWatcher::RemoveFromNotifyList(BPoseView *view)
{
	if (Lock()) {
		fNotifyList.RemoveItem(view);
		Unlock();
	}
}


void
BClipboardRefsWatcher::AddNode(const node_ref *node)
{
	TTracker::WatchNode(node,B_WATCH_NAME,this);
	fRefsInClipboard = true;
}


void
BClipboardRefsWatcher::RemoveNode(node_ref *node, bool removeFromClipboard)
{
	watch_node(node,B_STOP_WATCHING,this);

	if (!removeFromClipboard)
		return;

	if (be_clipboard->Lock()) {
		BMessage *clip = be_clipboard->Data();
		if (clip != NULL) {
			char name[64];
			MakeRefName(name,node);
			clip->RemoveName(name);
			MakeModeName(name);
			clip->RemoveName(name);

			be_clipboard->Commit();
		}
		be_clipboard->Unlock();
	}
}


void
BClipboardRefsWatcher::RemoveNodesByDevice(dev_t device)
{
	if (!be_clipboard->Lock())
		return;

	BMessage *clip = be_clipboard->Data();
	if (clip != NULL) {
		char deviceName[6];
		sprintf(deviceName,"r%ld_",device);

		uint32 index = 0;
		char *refName;
		type_code type;
		int32 count;
		while (clip->GetInfo(B_REF_TYPE, index,
#ifdef B_BEOS_VERSION_DANO
			(const char **)
#endif
			&refName, &type, &count) == B_OK) {
			if (!strncmp(deviceName,refName,strlen(deviceName))) {
				clip->RemoveName(refName);
				MakeModeName(refName);
				clip->RemoveName(refName);

				node_ref node;
				MakeNodeFromName(&node,refName);
				watch_node(&node,B_STOP_WATCHING,this);
			}
			index++;
		}
		be_clipboard->Commit();
	}
	be_clipboard->Unlock();
}


void
BClipboardRefsWatcher::UpdateNode(node_ref *node, entry_ref *ref)
{
	if (!be_clipboard->Lock())
		return;

	BMessage *clip = be_clipboard->Data();
	if (clip != NULL) {
		char name[64];
		MakeRefName(name,node);
		if ((clip->ReplaceRef(name,ref)) != B_OK) {
			clip->RemoveName(name);
			MakeModeName(name);
			clip->RemoveName(name);
			
			RemoveNode(node);
		}
		be_clipboard->Commit();
	}
	be_clipboard->Unlock();
}


void
BClipboardRefsWatcher::Clear()
{
	stop_watching(this);
	watch_node(NULL,B_WATCH_MOUNT,this);

	BMessage message(kClipboardPosesChanged);
	message.AddBool("clearClipboard",true);

	if (Lock()) {
		int32 items = fNotifyList.CountItems();
		for (int32 i = 0;i < items;i++) {
			BMessenger((BView*)fNotifyList.ItemAt(i)).SendMessage(&message);
		}
		Unlock();
	}
}


void
BClipboardRefsWatcher::UpdatePoseViews(bool clearClipboard, const node_ref *node)
{
	BMessage message(kClipboardPosesChanged);
	message.AddInt32("device",node->device);
	message.AddInt64("directory",node->node);
	message.AddBool("clearClipboard",clearClipboard);

	if (Lock()) {
		int32 items = fNotifyList.CountItems();
		for (int32 i = 0;i < items;i++) {
			BMessenger((BView*)fNotifyList.ItemAt(i)).SendMessage(&message);
		}
		Unlock();
	}
}


void
BClipboardRefsWatcher::MessageReceived(BMessage *message)
{
	if (message->what == B_CLIPBOARD_CHANGED && fRefsInClipboard) {
		if (!(fRefsInClipboard = FSClipboardHasRefs())) {
			Clear();
		}
		return;
	} else if (message->what != B_NODE_MONITOR) {
		_inherited::MessageReceived(message);
		return;
	}
	
	switch (message->FindInt32("opcode")) {	
		case B_ENTRY_MOVED:
		{
			ino_t toDir;
			ino_t fromDir;
			node_ref node;
			const char *name = NULL;
			message->FindInt64("from directory", &fromDir);
			message->FindInt64("to directory", &toDir);
			message->FindInt64("node", &node.node);
			message->FindInt32("device", &node.device);
			message->FindString("name", &name);
			entry_ref ref(node.device,toDir,name);
			UpdateNode(&node,&ref);
			break;
		}			
			
		case B_DEVICE_UNMOUNTED:
		{
			dev_t device;
			message->FindInt32("device",&device);
			RemoveNodesByDevice(device);
			break;
		}

		case B_ENTRY_REMOVED:
		{
			node_ref node;
			message->FindInt64("node", &node.node);
			message->FindInt32("device", &node.device);
			RemoveNode(&node,true);
			break;
		}
	}
}