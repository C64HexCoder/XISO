// XISO.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "XISO.h"
#include "io.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CXISOApp

BEGIN_MESSAGE_MAP(CXISOApp, CWinApp)

END_MESSAGE_MAP()


// CXISOApp construction

CXISOApp::CXISOApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CXISOApp object

CXISOApp theApp;


// CXISOApp initialization

BOOL CXISOApp::InitInstance()
{
	CWinApp::InitInstance();
	
	HMODULE ExeModule = ::GetModuleHandle (NULL);
	HMODULE DllModule = ::GetModuleHandle("XISO.DLL");

	return TRUE;
}


XIso::XIso (void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	XIsoTree = NULL;
	IsoDirsTree = NULL;
	BatchProgressBar = NULL;
	FileProgressBar = NULL;
	NumOfFiles = 0;
	NumOfFolders = 0;
	CancledByUser = false;

	Icons[0] = AfxGetApp()->LoadIcon(IDI_CLOSE_ICON);
	Icons[1] = AfxGetApp()->LoadIcon(IDI_OPEN_ICON);
	Icons[2] = AfxGetApp()->LoadIcon(IDI_ROOT_ICON);

	TreeImageList.Create(32,32,ILC_COLOR24|ILC_MASK,3,3);


	for (int n=0; n < 3; n++)
		TreeImageList.Add(Icons[n]);

}

bool XIso::OpenXIsoFile (CString FileName,char Flags)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ActiveWindowHandle = GetActiveWindow();
	switch (Flags) {
		default:
		case ISO_READ:
			XIsoFileHandle = CreateFile (FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,NULL,NULL);
			XISOFile.m_hFile = XIsoFileHandle;
			break;
		case ISO_WRITE:
			XIsoFileHandle = CreateFile (FileName,GENERIC_WRITE,NULL,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
			XISOFile.m_hFile = XIsoFileHandle;
			break;
	}
	if (XIsoFileHandle == INVALID_HANDLE_VALUE) {
		MessageBox (ActiveWindowHandle,"Error opening ISO file","Open file ERROR....",MB_OK);
		return false;
	}
	
	return true;
}

CString XIso::OpenXisoFileDlg ()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CFileDialog XIsoFileDlg (TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,"XISO XBOX iso files (*.ISO)|*.iso||",NULL,sizeof(OPENFILENAME ));
	
	if (XIsoFileDlg.DoModal() == IDOK)
	{
		CloseXIsoFile();
		IsoFileName = XIsoFileDlg.GetFileName();
		IsoPathName = XIsoFileDlg.GetPathName();
		OpenXIsoFile (IsoPathName,ISO_READ);
	}
	return IsoPathName;
}

CString XIso::SaveXisoFileDlg ()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CFileDialog XIsoFileDlg (FALSE,"*.iso",NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,"XISO XBOX iso files (*.ISO)|*.iso||",NULL,sizeof(OPENFILENAME ));

	if (XIsoFileDlg.DoModal() == IDOK)
	{
		CloseXIsoFile();
		IsoFileName = XIsoFileDlg.GetFileName();
		IsoPathName = XIsoFileDlg.GetPathName();
		OpenXIsoFile (IsoPathName,ISO_WRITE);
		}
	return IsoPathName;
}

bool XIso::GetPrimaryVolumeDiscriptor ()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());	

	XISOFile.Seek(16*2048,CFile::begin);
	if (XISOFile.Read(&PrimaryVolumeDescriptor,sizeof (PRIMARY_VOLUME_DESCRIPTOR)) == sizeof (PRIMARY_VOLUME_DESCRIPTOR))
	{
		if (strncmp (PrimaryVolumeDescriptor.StandardIdentifier,"CD001",5) != 0)
			return false;
	}
	else
		return false;
	
	return true;
}

bool XIso::GetVolumeDescriptor ()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	XISOFile.Seek(0x10000,CFile::begin );
	if (XISOFile.Read(&VolumeDescriptor,2048) == 2048) {
		if (strncmp (VolumeDescriptor.XboxPrefixString,MICROSOFT_MEDIA_STRING,20) != 0)
		return FALSE;
	}
	else
		return FALSE;
	
	return TRUE;
}

void XIso::GotoSector (unsigned int Sector) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());


	unsigned __int64 SectorOffset = (unsigned __int64) Sector * 2048;
	XISOFile.Seek(SectorOffset,CFile::begin );
}

bool XIso::ReadEntry (FILE_ENTRY* FileEntry)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (XISOFile.Read(FileEntry,14)==14) {
		FileEntry->FileName = new char[FileEntry->LengthOfFileName ];
		XISOFile.Read(FileEntry->FileName ,FileEntry->LengthOfFileName );
		
		return TRUE;

	}

	return FALSE;
}

bool XIso::ReadEntry ()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	XIsoTree = new XisoTree;

	if (XISOFile.Read(&XIsoTree->FileEntry,14)==14) {
		XIsoTree->FileEntry.FileName = new char[XIsoTree->FileEntry.LengthOfFileName ];
		XISOFile.Read(XIsoTree->FileEntry.FileName ,XIsoTree->FileEntry.LengthOfFileName );
		
		return TRUE;

	}

	return FALSE;
}


bool XIso::ReadEntry (XisoTree **TreeNode,ENTRYOFFSET EntryOffset,UINT TableSector)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	*TreeNode = new XisoTree;
	(*TreeNode)->NextTree = (*TreeNode)->PrevTree = (*TreeNode)->SubDirectory = NULL;
	(*TreeNode)->NumOfChildernFiles = 0;

	if (XISOFile.Read(&(*TreeNode)->FileEntry,14)==14) {
		(*TreeNode)->FileEntry.FileName = new char[(*TreeNode)->FileEntry.LengthOfFileName +1];
		XISOFile.Read((*TreeNode)->FileEntry.FileName ,(*TreeNode)->FileEntry.LengthOfFileName );
		(*TreeNode)->FileEntry.FileName[(*TreeNode)->FileEntry.LengthOfFileName] = 0x00;

		return TRUE;

	}
	
	TRACE ("Error reading FILE_ENTRY\n");
	ASSERT (FALSE);
	return FALSE;
}

bool XIso::ReadNextEntry (UINT EntryNode,unsigned int TableSector)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	switch (EntryNode)
	{
		case 1: GotoEntry (FileEntry.LeftEntryNode,TableSector );
				if (ReadEntry ()) return TRUE;
				break;

		case 2: GotoEntry (FileEntry.RightEntryNode,TableSector);
				if (ReadEntry()) return TRUE;
				break;
				
	}
	return FALSE;
}

bool XIso::GotoEntry (ENTRYOFFSET EntryOffset,unsigned int TableSector)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());


	UINT EntryOffsetInBytes = EntryOffset*4;
	XISOFile.Seek((unsigned __int64) TableSector*2048,CFile::begin);
	XISOFile.Seek(EntryOffsetInBytes,CFile::current);

	return true;

}

void XIso::AttachTreeView (CTreeCtrl* IsoDirsTreeCtrl)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	IsoDirsTree = IsoDirsTreeCtrl;
}

void XIso::LoadFiles()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	NumOfFiles = NumOfFolders = 0;

	XIsoTree = new XisoTree;
	XIsoTree->FileEntry.FileName = new char [IsoFileName.GetLength()+1];
	strcpy (XIsoTree->FileEntry.FileName,IsoFileName.GetString()); 
	XIsoTree->NextTree = NULL;
	XIsoTree->PrevTree = NULL;
	XIsoTree->NumOfChildernFiles = 0;	

	/*HTREEITEM Root*/ XIsoTree->Handle  = IsoDirsTree->InsertItem(IsoFileName,2,2);
	RecurciveEntryRead(&XIsoTree->SubDirectory ,0,VolumeDescriptor.RootTableSector,XIsoTree);
	IsoDirsTree->SetItem (XIsoTree->Handle,TVIF_PARAM,0,0,0,0,0,(LPARAM) XIsoTree );
	
}

void XIso::AttachProgressBar (CProgressCtrl* BatchProgressBar,CProgressCtrl* FileProgressBar)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	this->BatchProgressBar = BatchProgressBar;
	this->FileProgressBar = FileProgressBar;
}

void XIso::RecurciveEntryRead (XisoTree **TreeNode,ENTRYOFFSET EntryOffset,unsigned int TableSector,XisoTree *TreeRoot /*HTREEITEM RootItem*/)
{
	

	XisoTree* RootTreeNode = *TreeNode;
	

	GotoEntry (EntryOffset,TableSector );

	ReadEntry (TreeNode,EntryOffset,TableSector);

	if ((*TreeNode)->FileEntry.LeftEntryNode == 0 && (*TreeNode)->FileEntry.RightEntryNode == 0 && (*TreeNode)->FileEntry.StartingSectorOfFile == 0 /*(*TreeNode)->FileEntry.FileAttribute != 0x10*/)
		return;

	// Insert DIRECTORY item to the TreeCtrl
	if (((*TreeNode)->FileEntry.FileAttribute & 0x10) == 0x10) {
		(*TreeNode)->Handle  = IsoDirsTree->InsertItem(TVIF_TEXT|TVIF_PARAM|TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE,
			CString((*TreeNode)->FileEntry.FileName,(*TreeNode)->FileEntry.LengthOfFileName ),
			0,1,0,0,(LPARAM) (*TreeNode),TreeRoot->Handle /*RootItem*/,0); 
			NumOfFolders ++;
	}
	else	{
			NumOfFiles ++;
			TreeRoot->NumOfChildernFiles++;
	}
	
	(*TreeNode)->ParentTree = TreeRoot;

	
	if ((*TreeNode)->FileEntry.LeftEntryNode != 0) {
		RecurciveEntryRead (&(*TreeNode)->PrevTree,(*TreeNode)->FileEntry.LeftEntryNode,TableSector,TreeRoot/*RootItem*/ );
	}

	if ((*TreeNode)->FileEntry.RightEntryNode != 0){
		RecurciveEntryRead (&(*TreeNode)->NextTree,(*TreeNode)->FileEntry.RightEntryNode,TableSector,TreeRoot /*RootItem*/  );
	}

	if (((*TreeNode)->FileEntry.FileAttribute & 0x10) == 0x10 && (*TreeNode)->FileEntry.StartingSectorOfFile){
		RecurciveEntryRead (&(*TreeNode)->SubDirectory,0,(*TreeNode)->FileEntry.StartingSectorOfFile,*TreeNode);
	}
	

}

void XIso::BuildIso (CString *SrcPath)
{
	char Sector[BYTESPERSECTOR];
	CancledByUser = FALSE;
	if (BatchProgressBar) {
		BatchProgressBar->SetRange32(0,NumOfFiles);
		BatchProgressBar->SetStep(1);
	}
	
	ZeroMemory (Sector,BYTESPERSECTOR);
	ZeroMemory (&VolumeDescriptor,sizeof (VolumeDescriptor));
	UINT SectorsLeft;
	FillMemory (Buffer,BYTESPERSECTOR,0xFF);

	ZeroMemory (&PrimaryVolumeDescriptor,sizeof (PrimaryVolumeDescriptor));
	PrimaryVolumeDescriptor.LogicalBlockSize.SmallIndian = 0x0800;
	PrimaryVolumeDescriptor.LogicalBlockSize.BigIndian = 0x0008;
	PrimaryVolumeDescriptor.VolumeDescriptorType = 0x01;
	strncpy (PrimaryVolumeDescriptor.StandardIdentifier,"CD001",5);
	PrimaryVolumeDescriptor.Version = 0x01;
	

	for (int i=0; i< 16; i++)
		XISOFile.Write(&Sector,BYTESPERSECTOR);

	XISOFile.Write(&PrimaryVolumeDescriptor,2048);
	PrimaryVolumeDescriptor.VolumeDescriptorType = (char) 0xFF;
	XISOFile.Write(&PrimaryVolumeDescriptor,2048);

	// First wee need to add code for saving the PVD

	for (int i=0; i< 14; i++)
		XISOFile.Write(&Sector,BYTESPERSECTOR);

	strncpy (VolumeDescriptor.XboxPrefixString,MICROSOFT_MEDIA_STRING,20);
	strncpy (VolumeDescriptor.XboxSuffixString,MICROSOFT_MEDIA_STRING,20);

	VolumeDescriptor.RootTableSector = 0x108; // 264
	VolumeDescriptor.RootTableSize = (UINT) GetDirTableSize (XIsoTree->SubDirectory );
	//memcpy (Sector,&VolumeDescriptor,sizeof (VolumeDescriptor));


	XISOFile.Write(&VolumeDescriptor,BYTESPERSECTOR);

	for (int i=0; i<231;i++)
		XISOFile.Write(&Sector,BYTESPERSECTOR);

	// make first DIRECTOR_ENTRY
	//XISOFile.Write(Buffer,BYTESPERSECTOR);
	BuildEntrys (SrcPath,XIsoTree->SubDirectory,264);
	
	if (!CancledByUser) {
		SectorsLeft = (0x10000 - ((UINT) XISOFile.GetLength() % 0x10000))/2048;

	
		XISOFile.Seek(0,CFile::end);
		ZeroMemory (Buffer,BYTESPERSECTOR);

		for (unsigned int i=0; i< SectorsLeft; i++)
			XISOFile.Write(Buffer,BYTESPERSECTOR);

		//	XISOFile.Write(Buffer,BytesLeft);
	}
	

	
}

UINT XIso::WriteSector (char *Buffer,UINT DestSector,UWORD Size)
{
	char Sector[BYTESPERSECTOR];
	ZeroMemory (Sector,BYTESPERSECTOR);


	if (Size == BYTESPERSECTOR) 
	{
		int x = (int) XISOFile.GetPosition();	
		XISOFile.Write(Buffer,BYTESPERSECTOR);
	}
	
	return DestSector++;
}

UINT XIso::GetDirTableSize (XisoTree *xTree)
{
	UINT sum =0, EntrySize;

	while (xTree)
	{
		EntrySize = 14+xTree->FileEntry.LengthOfFileName;
		
		if ((2048-(sum % 2048)) < EntrySize)
			EntrySize += (2048-(sum % 2048));
		else if ((sum+EntrySize) % 4)
			EntrySize += 4-((sum+EntrySize) % 4);

		sum += EntrySize ;
		xTree = xTree->NextTree;
	}

	return sum;
}

unsigned short XIso::GetFileEntrySize (FILE_ENTRY *FileEntry)
{
	UWORD sum = 0;
	return sum = 14+FileEntry->LengthOfFileName;
}

void XIso::BuildEntrys (CString *DestPath,XisoTree *xTree,UINT TableIndex)
{
	CFile FileToX;
	static UINT SectorIndex = 264,NumOfSectorsForDir = 1;
	UWORD EntrySize,NextEntrySize;
	unsigned __int64 Index;

	char Pad = 0,EntryPad;
	//static const PadBytes[3] = {0xff,0xff,0xff};

	Index = 0;
		
/*	if (xTree->NextTree == NULL && xTree->PrevTree == NULL && xTree->SubDirectory  == NULL)
		return;*/
	SectorIndex += NumOfSectorsForDir;
	GotoSector (TableIndex);


				// save file to xiso
//	XISOFile.Write(Buffer,BYTESPERSECTOR);
	while (xTree != NULL) {
		
		if (CancledByUser)
			return;

	
		xTree->FileEntry.LeftEntryNode =  NULL;
	
			// Create another folder entry in xiso
		
		
		if ((xTree->FileEntry.FileAttribute & 0x10) == 0x10) {
			if (xTree->SubDirectory)	{		// subdir not empty
				xTree->FileEntry.TotalFileSize = (UINT) GetDirTableSize (xTree->SubDirectory);
				
				NumOfSectorsForDir = xTree->FileEntry.TotalFileSize / 2048;
				if (xTree->FileEntry.TotalFileSize % 2048) NumOfSectorsForDir ++;

				if (NumOfSectorsForDir > 1) {
					GotoSector (SectorIndex);
					for (unsigned int i=0; i< NumOfSectorsForDir; i++)
												XISOFile.Write(Buffer,2048);
				
					GotoSector (TableIndex);
					XISOFile.Seek(Index,CFile::current);

				}

				xTree->FileEntry.StartingSectorOfFile  = SectorIndex;
			}
			else {
				xTree->FileEntry.TotalFileSize = 0x00;
				xTree->FileEntry.StartingSectorOfFile  = 0x00;

			}
		}
		else	// if not a folder (a file) then link it to the start sector
			xTree->FileEntry.StartingSectorOfFile  = SectorIndex;

		//AddEntry (xTree->FileEntry);
		NextEntrySize = 0;
		EntrySize = GetFileEntrySize(&xTree->FileEntry );
		if (xTree->NextTree) 
			NextEntrySize = GetFileEntrySize (&xTree->NextTree->FileEntry);
		EntryPad = (EntryPad = (char)(Index +EntrySize) % 4) ? 4-EntryPad : EntryPad;
		//EntryPad ? EntryPad = 4-EntryPad : EntryPad;

		if ((EntrySize + EntryPad+ NextEntrySize) > (2048-(Index % 2048))) {
         			Pad = (char) (2048-EntrySize-(Index % 2048));
		}
		else 
			Pad = EntryPad;
			
		
		if(xTree->NextTree)
			xTree->FileEntry.RightEntryNode = (WORD) (Index+EntrySize+Pad)/4;
		else
			xTree->FileEntry.RightEntryNode = NULL;

		XISOFile.Write(&xTree->FileEntry ,14);
		Index +=14;
		XISOFile.Write(xTree->FileEntry.FileName,xTree->FileEntry.LengthOfFileName);
		Index+= xTree->FileEntry.LengthOfFileName;

	
		XISOFile.Write(&Buffer,Pad);
		//XISOFile.Seek(Pad,CFile::current);
		Index+=Pad;
		
		if ((xTree->FileEntry.FileAttribute & 0x10) == 0x10) {
			if (xTree->SubDirectory ) {
				BuildEntrys(&(*DestPath+"\\"+xTree->FileEntry.FileName),xTree->SubDirectory,SectorIndex);
				GotoSector (TableIndex);
				XISOFile.Seek(Index,CFile::current);
			}
		}
		else {
			SendMessage (ActiveWindowHandle,XIDLL_STATUS_CHANGED,(WPARAM) (*DestPath+"\\"+xTree->FileEntry.FileName).GetString(),1);
			GotoSector (SectorIndex);
			SectorIndex += AddFileToIso (xTree,TableIndex,&(*DestPath+"\\"+xTree->FileEntry.FileName) );	
			GotoSector (TableIndex);
			XISOFile.Seek(Index,CFile::current);
			
			if (BatchProgressBar)
				BatchProgressBar->StepIt();

		}
			// finished file/folder and ready to go to the next one ...
		
		
		xTree = xTree->NextTree;
	}
	GotoSector (TableIndex);
	XISOFile.Seek(Index,CFile::current);
	XISOFile.Write(Buffer,(UINT) (2048-(Index % 2048)));

}
/*void XIso::BuildEntrys (CString *DestPath,XisoTree *xTree,UWORD SectorIndex) {

if (xTree->PrevTree == NULL && xTree->NextTree == NULL && Prev

}*/

UINT XIso::AddFileToIso (XisoTree *xTree,UINT TableSector,CString *FileName)
{
	
	UINT SectorsAdded=0,NumOfSectors = xTree->FileEntry.TotalFileSize / BYTESPERSECTOR;
	unsigned int NumberOfBytes = xTree->FileEntry.TotalFileSize % BYTESPERSECTOR;	
	char Buffer[2048];
	CFile SrcFile (*FileName,CFile::modeRead);
	

	if (FileProgressBar) {
		FileProgressBar->SetRange32(0,NumOfSectors);
		FileProgressBar->SetStep(1);

	}

	for (unsigned int i=0; i< NumOfSectors; i++)
	{
		if (CancledByUser)
						goto exit;

		SrcFile.Read(Buffer,BYTESPERSECTOR);
		XISOFile.Write(Buffer,BYTESPERSECTOR);
		SectorsAdded ++;
	
		if (FileProgressBar) 
			FileProgressBar->SetPos(i+1);
		
		ProcessAllMessages ();
	}
	
	if (NumberOfBytes) {
		ZeroMemory (Buffer,BYTESPERSECTOR);
		SrcFile.Read(Buffer,NumberOfBytes);
		XISOFile.Write(Buffer,BYTESPERSECTOR);
		SectorsAdded++;

	}

exit:
	SrcFile.Close();

	return SectorsAdded;
}

void XIso::AddEntry (FILE_ENTRY FileEntry)
{
	XISOFile.Write(&FileEntry,14);
	XISOFile.Write(FileEntry.FileName ,FileEntry.LengthOfFileName);
}

void XIso::BuildTreeFromFolder(CString *SrcPath,XisoTree **xTree,XisoTree *ParentTree)
{
	WIN32_FIND_DATA FindData;
	HANDLE FindFileH;

	if (ParentTree == NULL) {
		//HeapH = GetProcessHeap ();
		ParentTree = (*xTree) = new XisoTree;
		(*xTree)->NextTree = (*xTree)->PrevTree = (*xTree)->ParentTree = (*xTree)->SubDirectory = NULL;
		(*xTree)->Handle = NULL;
		(*xTree)->NumOfChildernFiles = 0;
		(*xTree)->FileEntry.FileName = new char [SrcPath->GetLength()+1];
		strcpy ((*xTree)->FileEntry.FileName,SrcPath->GetString());
		(*xTree)->FileEntry.LengthOfFileName = SrcPath->GetLength();
		(*xTree)->FileEntry.FileAttribute = 0x10;
		NumOfFolders = NumOfFiles = 0;
		(*xTree)->Handle = IsoDirsTree->InsertItem(TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT,(*xTree)->FileEntry.FileName,2,2,0,0,(LPARAM) *xTree,TVI_ROOT,NULL);
	
		xTree = &(*xTree)->SubDirectory;
	}

	if ((FindFileH = FindFirstFile (*SrcPath+"\\*.*",&FindData)) != INVALID_HANDLE_VALUE)
	{
		do {	
			if (FindData.cFileName [0] != '.'){
				*xTree = new ( XisoTree); 
				//*xTree = HeapAlloc (HeapH,HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,sizeof (XisoTree));
				(*xTree)->NextTree = (*xTree)->PrevTree = (*xTree)->SubDirectory = NULL;
				(*xTree)->Handle = NULL;
				(*xTree)->NumOfChildernFiles = 0;
				(*xTree)->FileEntry.FileAttribute = (char) FindData.dwFileAttributes; 
				(*xTree)->ParentTree = ParentTree;

				(*xTree)->FileEntry.FileName = new char [strlen (FindData.cFileName)+1];
				strcpy ((*xTree)->FileEntry.FileName,FindData.cFileName);
				(*xTree)->FileEntry.LengthOfFileName = (char) strlen (FindData.cFileName);
				(*xTree)->FileEntry.TotalFileSize = (FindData.nFileSizeHigh << 16) | FindData.nFileSizeLow; 
				(*xTree)->ParentTree->NumOfChildernFiles++;

				if (FindData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
					if (IsoDirsTree) {
							(*xTree)->Handle = IsoDirsTree->InsertItem(TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT,(*xTree)->FileEntry.FileName,0,1,0,0,(LPARAM) *xTree,ParentTree->Handle,NULL);

					}
					BuildTreeFromFolder (&(*SrcPath+"\\"+FindData.cFileName),&(*xTree)->SubDirectory,*xTree);
					NumOfFolders++;
				}
				else
					NumOfFiles ++;

				xTree = &(*xTree)->NextTree;
			}
		} while (FindNextFile (FindFileH,&FindData) != 0);	
		FindClose (FindFileH);
	}
}

void XIso::ExtractAllFiles(CString *DestPath,char Flags)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	this->DestPath = *DestPath;

	if (XISOFile.m_hFile) { 
		switch (Flags) {
			default:
			case EAF_FROMTREE:
				CancledByUser = FALSE;
				DestPathName.LockBuffer();
				BatchProgressBar->SetRange32(0,NumOfFiles);
				BatchProgressBar->SetStep (1);
				BatchProgressBar->SetPos(0);
				ExtractAllUsingTree (XIsoTree->SubDirectory,*DestPath);
				break;
			case EAF_FROMISO:
				break;
		}
	}
	else
		MessageBox (NULL,"Select source (XISO) file first","XISO DLL ERROR",MB_OK);
}

void XIso::ExtractFolder (XisoTree *xTree,CString *DestPath)
{

	XisoTree *Parent = xTree->ParentTree ;

	CancledByUser = false;
	UINT NumOfFiles = CalcNumOfFilesInTreeSubs (xTree);
	NumOfFiles += Parent->NumOfChildernFiles;

	BatchProgressBar->SetRange32 (0,NumOfFiles);
	BatchProgressBar->SetStep(1);

	ExtractAllUsingTree (xTree,*DestPath);
}

UINT XIso::CalcNumOfFilesInTreeSubs (XisoTree *xTree)
{
	UINT SumOfFiles=0;
	
	if (xTree->PrevTree )
		SumOfFiles += CalcNumOfFilesInTreeSubs (xTree->PrevTree);
		
	if (xTree->NextTree)
		SumOfFiles += CalcNumOfFilesInTreeSubs (xTree->NextTree);

	if (xTree->SubDirectory)
	{
		SumOfFiles+= xTree->NumOfChildernFiles;
		SumOfFiles+= CalcNumOfFilesInTreeSubs (xTree->SubDirectory);
	}

	return SumOfFiles;

}
void XIso::ExtractAllUsingTree (XisoTree *RootTree,CString DestPath)
{
	
	if (CancledByUser || !RootTree)
			return;

	GotoSector (RootTree->FileEntry.StartingSectorOfFile);

	// in case the File is NOT a folder 
	if (!(RootTree->FileEntry.FileAttribute & 0x10)) { 
		if (FileProgressBar) {
			FileProgressBar->SetRange32(0,RootTree->FileEntry.TotalFileSize);
			FileProgressBar->SetStep(2048);
			FileProgressBar->SetPos(0);
		}

		NumOfSectors = RootTree->FileEntry.TotalFileSize  / BYTESPERSECTOR;
		ExtraBytesToRead = RootTree->FileEntry.TotalFileSize  - (NumOfSectors * BYTESPERSECTOR);
		strncpy (FileName,RootTree->FileEntry.FileName,RootTree->FileEntry.LengthOfFileName);
		FileName[RootTree->FileEntry.LengthOfFileName] = 0x00;

		DestPathName = DestPath +"\\"+FileName;
		NewFile = CreateFile (DestPathName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		SendMessage (ActiveWindowHandle,XIDLL_STATUS_CHANGED,(WPARAM) DestPathName.GetString(),0);

		for (unsigned int i=0; i< NumOfSectors; i++) {
			
			if (CancledByUser)
				break;

			XISOFile.Read(Buffer,BYTESPERSECTOR);
			WriteFile (NewFile,Buffer,BYTESPERSECTOR,&BytesWriten,NULL);
			if (FileProgressBar)
				FileProgressBar->StepIt();
		
			ProcessAllMessages();							
		}

		FileProgressBar->SetPos(RootTree->FileEntry.TotalFileSize);
		XISOFile.Read(Buffer,ExtraBytesToRead);
		WriteFile (NewFile,Buffer,ExtraBytesToRead,&BytesWriten,NULL);
		CloseHandle(NewFile);
		BatchProgressBar->StepIt();			// later check if exists
	}		
	
	if (RootTree->PrevTree )
			ExtractAllUsingTree (RootTree->PrevTree,DestPath);

	if (RootTree->NextTree ) 
			ExtractAllUsingTree (RootTree->NextTree,DestPath );

	if ((RootTree->FileEntry.FileAttribute & 0x10) == 0x10) {
		
		DestPathName = DestPath+"\\"+CString (RootTree->FileEntry.FileName,RootTree->FileEntry.LengthOfFileName);	
		
		CreateDirectory (DestPathName,NULL);
		ExtractAllUsingTree (RootTree->SubDirectory,DestPathName);	

	}
}

FILE_ENTRY XIso::SearchFile(CString *SrcPathName)
{
	FILE_ENTRY FileEntry;
	return FileEntry;
}

void XIso::ExtractFile(FILE_ENTRY* FileEntry,CString *DestPath)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	CancledByUser = FALSE;

	if ((FileEntry->FileAttribute & 0x20) == 0x20) {
		if (FileProgressBar) {
			FileProgressBar->SetRange32(0,FileEntry->TotalFileSize);
			FileProgressBar->SetStep(2048);
			FileProgressBar->SetPos(0);
		}
		GotoSector (FileEntry->StartingSectorOfFile);
		NumOfSectors = FileEntry->TotalFileSize  / BYTESPERSECTOR;
		ExtraBytesToRead = FileEntry->TotalFileSize  - (NumOfSectors * BYTESPERSECTOR);
		DestPathName = *DestPath +"\\"+CString (FileEntry->FileName,FileEntry->LengthOfFileName );		
		NewFile = CreateFile (DestPathName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

		SendMessage (ActiveWindowHandle,XIDLL_STATUS_CHANGED,(WPARAM) DestPathName.GetString(),0);
		for (unsigned int i=0; i< NumOfSectors; i++) {
			if (CancledByUser)
				break;

			XISOFile.Read(Buffer,BYTESPERSECTOR);
			WriteFile (NewFile,Buffer,BYTESPERSECTOR,&BytesWriten,NULL);
			if (FileProgressBar)
				FileProgressBar->StepIt();
		
			ProcessAllMessages();							
		}
		FileProgressBar->SetPos(FileEntry->TotalFileSize);

		XISOFile.Read(Buffer,ExtraBytesToRead);
		WriteFile (NewFile,Buffer,ExtraBytesToRead,&BytesWriten,NULL);
		CloseHandle(NewFile);
	}		
	

}

void XIso::FreeTree ()
{
	FreeBranch (XIsoTree);
	XIsoTree = NULL;
}

void XIso::FreeBranch (XisoTree *xTree)
{
	if (xTree == NULL)
		return;

	ASSERT (xTree->FileEntry.FileName  != "Prague_Dungeon2_ALT.wma");
	TRACE ("INSIDE %x",xTree);

	if (xTree->PrevTree ) 
		FreeBranch (xTree->PrevTree);

	if (xTree->NextTree) 
		FreeBranch (xTree->NextTree);

	if (xTree->SubDirectory) 
		FreeBranch (xTree->SubDirectory);
	

	delete xTree->FileEntry.FileName;
	xTree->FileEntry.FileName = NULL;
	delete xTree;
	xTree = NULL;
}

void XIso::ProcessAllMessages ()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	MSG msg;
	while ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) 
	{ 
		if ( !WinApp->PumpMessage( ) ) 
		{ 
			::PostQuitMessage(0 ); 
			break; 
		} 
	} 

}

void XIso::CancleProcess()
{
	CancledByUser = TRUE;
}

bool XIso::IsSuccessfull ()
{
	return !CancledByUser;
}

void XIso::CloseXIsoFile ()
{
	if (XISOFile.m_hFile != INVALID_HANDLE_VALUE)
				XISOFile.Close();
}