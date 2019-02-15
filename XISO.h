// XISO.h : main header file for the XISO DLL
//

#pragma once
#ifndef __XISO_H__
#define  __XISO_H__ 
#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

#define RE_LEFT_ENTRY_NODE		1
#define RE_RIGHT_ENTRY_NODE		2

#define EAF_FROMTREE			1
#define EAF_FROMISO				2

// iso OpenIsoFile flags
#define ISO_READ				0
#define ISO_WRITE				1


#define MICROSOFT_MEDIA_STRING "MICROSOFT*XBOX*MEDIA"

#define	ENTRYOFFSET				WORD

#define BYTESPERSECTOR			2048

#define XIDLL_STATUS_CHANGED WM_USER +5

// CXISOApp
// See XISO.cpp for the implementation of this class
//

typedef struct UINT_SMALLBIG_TAG {
	UINT SmallIndian;
	UINT BigIndian;
}UINT_SMALLBIG;

typedef struct UWORD_SMALLBIG_TAG {
	unsigned short SmallIndian;
	unsigned short BigIndian;
}UWORD_SMALLBIG;

typedef struct PRIMARY_VOLUME_DESCRIPTOR_TAG {
	char	VolumeDescriptorType;					// sould be 1
	char	StandardIdentifier[5];
	char	Version;
	char	Unused;
	char	SystemIdentifier[32];					// a-characters allowed
	char	VolumeIdentifier[32];					// d-characters allowed
	char	Unused2[8];
	UINT_SMALLBIG VolumeSpaceSize;						// number of logical block in the vloume
	char	Unused3[32];
	unsigned int volumeSetSize;
	UINT	VolumeSequenceNumber;					// 
	UWORD_SMALLBIG LogicalBlockSize;
	UINT_SMALLBIG PathTableSize;
	UINT	TypeLPathTable;							// location of type L path table - the block number
	UINT	OptionalTypeLPathTable;
	UINT	TypeMPathTable;
	UINT	OptionalTypeMPathTable;
	char	RootDirectoryRecord[34];
	char	VolumeSetIdentifier[128];
	char	PublisherIdentifier[128];
	char	DataPreparerIdentifier[128];
	char	ApplicationIdentifier[128];
	char	CopyrightFileIdentifier[37];
	char	AbstractFileIdentifier[37];
	char	bibliographicFileIdentifier[37];
	char	CreationDateAndTime[17];
	char	ModificationDataAndTime[17];
	char	ExpirationDataAndTime[17];
	char	EffectiveDataAndTime[17];
	char	FileStructureVersion;					// sould be 1
	char	Reserved;
	char	ApplicationUse[512];
	char	Reserved2 [653];						// must be filled with 0s

}PRIMARY_VOLUME_DESCRIPTOR;

typedef  struct VOLUME_DESCRIPTOR_TAG {
	char XboxPrefixString[0x14];
	unsigned int RootTableSector;
	unsigned int RootTableSize;
	FILETIME FileTime;
	char Unused [0x7c8];
	char XboxSuffixString[0x14];
}VOLUME_DESCRIPTOR;

typedef struct FILE_ENTRY_TAG {
	ENTRYOFFSET LeftEntryNode;
	ENTRYOFFSET RightEntryNode;
	unsigned int StartingSectorOfFile;
	unsigned int TotalFileSize;
	char FileAttribute;
	char LengthOfFileName;
	char *FileName;
}FILE_ENTRY;

typedef struct XisoTree_Tag {
	XisoTree_Tag *PrevTree;
	XisoTree_Tag *NextTree;
	XisoTree_Tag *ParentTree;
	HTREEITEM	Handle;
	FILE_ENTRY FileEntry;
	UINT NumOfChildernFiles;
	XisoTree_Tag *SubDirectory;
}XisoTree;

class CXISOApp : public CWinApp
{
private:


public:
	CXISOApp();
// Overrides
public:
	virtual BOOL InitInstance();
	 
	DECLARE_MESSAGE_MAP()
};


extern "C" class XIso 
{
private:
	int x;
	CImageList TreeImageList;
	HICON Icons[3];
	HANDLE XIsoFileHandle;
	CFile XISOFile;
	CTreeCtrl* IsoDirsTree;
	CString IsoFileName;
	CString IsoPathName;
	CString IsoDestName;
	CProgressCtrl *FileProgressBar,*BatchProgressBar;
	char Buffer [BYTESPERSECTOR];
	HANDLE NewFile;
	bool CancledByUser;
	UINT NumOfSectors,ExtraBytesToRead;
	CString DestPathName;
	DWORD BytesWriten;



	//     PRIVATE ROUTINES
	void GotoSector (unsigned int Sector);
	void FreeBranch (XisoTree *xTree);
	void RecurciveEntryRead (XisoTree **TreeNode,ENTRYOFFSET EntryOffset,unsigned int TableSector,XisoTree *TreeRoot /*HTREEITEM RootItem*/);
	void ExtractAllUsingTree (XisoTree *RootTree,CString DestPath);
	UINT GetDirTableSize (XisoTree *xTree);
	void AddEntry (FILE_ENTRY FileEntry);
	UINT WriteSector (char *Buffer,UINT DestSector,unsigned short Size);
	UINT AddFileToIso (XisoTree *xTree,UINT TableSector,CString *FileName);
	unsigned short GetFileEntrySize (FILE_ENTRY *FileEntry);
public:
	PRIMARY_VOLUME_DESCRIPTOR PrimaryVolumeDescriptor;
	VOLUME_DESCRIPTOR VolumeDescriptor;
	FILE_ENTRY FileEntry;
	XisoTree *XIsoTree;
	CString DestPath;
	UINT NumOfFiles,NumOfFolders;
	CWnd *AppWndHand;
	CWinApp* WinApp;
	char FileName [256];
	HWND ActiveWindowHandle;

	_declspec (dllexport) XIso ();
	_declspec (dllexport) bool OpenXIsoFile (CString FileName,char Flags);
	_declspec (dllexport) void CloseXIsoFile ();
	_declspec (dllexport) void CancleProcess();
	_declspec (dllexport) bool IsSuccessfull ();
	_declspec (dllexport) CString OpenXisoFileDlg ();
	_declspec (dllexport) CString SaveXisoFileDlg ();
	_declspec (dllexport) bool GetPrimaryVolumeDiscriptor ();
	_declspec (dllexport) bool GetVolumeDescriptor ();
	_declspec (dllexport) bool ReadEntry ();			// read file entry from current file position and attach it to root of tree
	_declspec (dllexport) bool ReadEntry (FILE_ENTRY* FileEntry);
	_declspec (dllexport) bool ReadEntry (XisoTree **TreeNode,ENTRYOFFSET EntryOffset,UINT TableSector); // read file entry from specific file position and put it in the tree
	_declspec (dllexport) bool ReadNextEntry (UINT EntryNode,unsigned int TableSector);
	_declspec (dllexport) bool GotoEntry (ENTRYOFFSET EntryOffset,unsigned int TableSector);
	_declspec (dllexport) void AttachTreeView (CTreeCtrl* IsoDirsTreeCtrl);
	_declspec (dllexport) void AttachProgressBar (CProgressCtrl* BatchProgressBar,CProgressCtrl* FileProgressBar=NULL);
	_declspec (dllexport) void LoadFiles();
	_declspec (dllexport) void ExtractAllFiles (CString *DestPath,char Flags=EAF_FROMTREE);
	_declspec (dllexport) void ExtractFolder (XisoTree *xTree,CString *Destpath);
	_declspec (dllexport) void BuildTreeFromFolder (CString *SrcPath,XisoTree **xTree,XisoTree *ParentTree);
	_declspec (dllexport) FILE_ENTRY SearchFile (CString *SrcPathName);
	_declspec (dllexport) void BuildIso (CString *SrcPath);
	_declspec (dllexport) void BuildEntrys (CString *DestPath,XisoTree *xTree,UINT TableIndex);
	_declspec (dllexport) void ExtractFile (FILE_ENTRY* FileEntry,CString *DestPath);
	_declspec (dllexport) void FreeTree ();
	_declspec (dllexport) void ProcessAllMessages ();
	UINT CalcNumOfFilesInTreeSubs (XisoTree *xTree);
	//extern "C" _declspec (dllexport) void TransferTreeToCtrls
};

#endif 