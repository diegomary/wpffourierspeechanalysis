// This is the main DLL file.

#include "stdafx.h"
#include "corelexicon.h"
#include "Dictionnary.h"

using namespace corelexicon;
 LexHelper::LexHelper(){
	m_freq_ext=2500;
	is_playing_phonema=false;
	m_min_recthreshold=80; // We start admitting that no samples with intensity < MAXSHORT /50 can be recorded
	// Remember CoInitializeEx always between brackets and prefer it to the simple CoInitialize
	CoInitializeEx(NULL,COINIT_MULTITHREADED|COINIT_SPEED_OVER_MEMORY); // Initialization in multithreading
	// Warning : the COINIT_SPEED_OVER_MEMORY flag is used to improve performance providing a good amount of memory
	//CoInitialize creates a single-threaded apartment (STA), whereas CoInitializeEx with the
	//COINIT_MULTITHREADED flag produces a multi-threaded apartment.
	//The CoInitialize/CoInitializeEx call does not initialize the COM library globally, 
	//only the calling thread's use of it, so it's important to remember that this initialization
	//should be done on a per-thread basis. This is typically done early in a thread's work function (ThreadProc). 	
	// MFC Initialization
	AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0);	
	pVoice = NULL;
	HRESULT hr = pVoice.CoCreateInstance( CLSID_SpVoice );
	}

 LexHelper::~LexHelper(){
		pVoice.Release();
		pCurVoiceToken->Release();
		::CoUninitialize();
	
	}


bool LexHelper::CreateNewVoiceWORDS(String ^ m_voicename,String ^ m_voice_description, String ^ language,
						  String^ m_CLSID, String ^ m_pathvoicefile, String ^ gender, String ^ age)	
{
	
pin_ptr<String ^> p = &m_voicename;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voicenameU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voicename)).ToPointer();//Unicode char set
//***************************************************************************************************	
pin_ptr<String ^> pp = &m_voice_description;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voice_descriptionU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voice_description)).ToPointer();//Unicode char set	
//***************************************************************************************************		
pin_ptr<String ^> ppp = &language;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR languageU =(LPCWSTR)(Marshal::StringToHGlobalUni(language)).ToPointer();//Unicode char set	
//***************************************************************************************************	
pin_ptr<String ^> pppp = &m_CLSID;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_CLSIDU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_CLSID)).ToPointer();//Unicode char set	
//***************************************************************************************************
pin_ptr<String ^> ppppp = &m_pathvoicefile;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_pathvoicefileU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_pathvoicefile)).ToPointer();//Unicode char set	
//***************************************************************************************************
pin_ptr<String ^> pppppp = &gender;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR genderU =(LPCWSTR)(Marshal::StringToHGlobalUni(gender)).ToPointer();//Unicode char set	
//***************************************************************************************************
pin_ptr<String ^> ppppppp = &age;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR ageU =(LPCWSTR)(Marshal::StringToHGlobalUni(age)).ToPointer();//Unicode char set	
//***************************************************************************************************

// Now we can proceed to build the voice token		
int langidbase10 = Convert::ToInt32(language,16); // This base conversion is very useful
HRESULT	hr;
	CLSID pguid=ConvertStringtoCLSID(m_CLSID);
	CComPtr<ISpObjectToken> cpToken;
	CComPtr<ISpDataKey> cpDataKeyAttribs;
	hr = SpCreateNewTokenEx(SPCAT_VOICES, m_voice_descriptionU,
						&pguid, 
						m_voicenameU, 
						langidbase10, 
						m_voicenameU, 
						&cpToken,
						&cpDataKeyAttribs);	          
	hr = cpDataKeyAttribs->SetStringValue(L"Gender", L"Male");
	hr = cpDataKeyAttribs->SetStringValue(L"Description", m_voice_descriptionU);
	hr = cpDataKeyAttribs->SetStringValue(L"Language", languageU);
	hr = cpDataKeyAttribs->SetStringValue(L"Name", m_voicenameU);
	hr = cpDataKeyAttribs->SetStringValue(L"Age", ageU);
    hr = cpDataKeyAttribs->SetStringValue(L"Gender", genderU);
	hr = cpDataKeyAttribs->SetStringValue(L"Vendor", L"DM&Mary");
	hr = cpToken->SetStringValue(L"VoiceData", m_pathvoicefileU);
// The following fragment creates the preview Key in the registry and sets a defaul phrase
   //wchar_t *defaultphrase = L"Il treno regionale 20 80 delle ore 17 ferma in tutte le stazioni %s"; 	
   wchar_t *defaultphrase = L"Frase che si legge nell'anteprima"; 
// The %s add the Name of the voice  
   wchar_t * kname = L"Preview";   
   HKEY hk; 
   DWORD dwDisp; 
   TCHAR szBuf[MAX_PATH]; 
   size_t cchSize = MAX_PATH;
   // Create the event source as a subkey of the log. 
   HRESULT hr1 = StringCchPrintf(szBuf, cchSize, 
   L"SOFTWARE\\MICROSOFT\\Speech\\Voices\\%s",
   kname); 
   if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szBuf, 
   0, NULL, REG_OPTION_NON_VOLATILE,
   KEY_WRITE, NULL, &hk, &dwDisp)) 
   {
   printf("Warning Could not create the registry key.Default phrase for this voice won't be available"); 
   }
   if (RegSetValueEx(hk,             // subkey handle 
   L"410",// value name that is the Italian Language LCID
   0,// must be zero 
   REG_SZ,// value type 
   (LPBYTE) defaultphrase,          // pointer to value data 
   (DWORD) (lstrlen(defaultphrase)+1)*sizeof(TCHAR))) // data size
   {
   printf("Could not set the default phrase in the preview key");RegCloseKey(hk); 
   }
// End Default phrase creation in the registry  ********************************************************
	if(SUCCEEDED(hr)) return true; else return false;
	// This return value is the CLSID of our blank voice and must be used when developing the 
	// engine for this newly created voice
}

bool LexHelper::CreateNewVoiceCONCAT(String ^ m_voicename,String ^ m_voice_description, String ^ language,
						  String^ m_CLSID, String ^ m_pathvoicewords,String ^ m_pathvoicephones,  String ^ gender, String ^ age)	
{
	
pin_ptr<String ^> p = &m_voicename;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voicenameU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voicename)).ToPointer();//Unicode char set
//***************************************************************************************************	
pin_ptr<String ^> pp = &m_voice_description;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voice_descriptionU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voice_description)).ToPointer();//Unicode char set	
//***************************************************************************************************		
pin_ptr<String ^> ppp = &language;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR languageU =(LPCWSTR)(Marshal::StringToHGlobalUni(language)).ToPointer();//Unicode char set	
//***************************************************************************************************	
pin_ptr<String ^> pppp = &m_CLSID;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_CLSIDU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_CLSID)).ToPointer();//Unicode char set	
//***************************************************************************************************
pin_ptr<String ^> ppppp = &m_pathvoicewords;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_pathvoicewordsU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_pathvoicewords)).ToPointer();//Unicode char set	
//***************************************************************************************************
pin_ptr<String ^> ppppp1 = &m_pathvoicephones;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_pathvoicephonesU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_pathvoicephones)).ToPointer();//Unicode char set	
//***************************************************************************************************
pin_ptr<String ^> pppppp = &gender;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR genderU =(LPCWSTR)(Marshal::StringToHGlobalUni(gender)).ToPointer();//Unicode char set	
//***************************************************************************************************
pin_ptr<String ^> ppppppp = &age;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR ageU =(LPCWSTR)(Marshal::StringToHGlobalUni(age)).ToPointer();//Unicode char set	
//***************************************************************************************************

// Now we can proceed to build the voice token		
int langidbase10 = Convert::ToInt32(language,16); // This base conversion is very useful
HRESULT	hr;
	CLSID pguid=ConvertStringtoCLSID(m_CLSID);
	CComPtr<ISpObjectToken> cpToken;
	CComPtr<ISpDataKey> cpDataKeyAttribs;
	hr = SpCreateNewTokenEx(SPCAT_VOICES, m_voice_descriptionU,
						&pguid, 
						m_voicenameU, 
						langidbase10, 
						m_voicenameU, 
						&cpToken,
						&cpDataKeyAttribs);	          
	hr = cpDataKeyAttribs->SetStringValue(L"Gender", L"Male");
	hr = cpDataKeyAttribs->SetStringValue(L"Description", m_voice_descriptionU);
	hr = cpDataKeyAttribs->SetStringValue(L"Language", languageU);
	hr = cpDataKeyAttribs->SetStringValue(L"Name", m_voicenameU);
	hr = cpDataKeyAttribs->SetStringValue(L"Age", ageU);
    hr = cpDataKeyAttribs->SetStringValue(L"Gender", genderU);
	hr = cpDataKeyAttribs->SetStringValue(L"Vendor", L"DM&Mary");
	hr = cpToken->SetStringValue(L"VoiceWordsDB", m_pathvoicewordsU);
	hr = cpToken->SetStringValue(L"VoicePhonesIntelligence", m_pathvoicephonesU);
			
	if(SUCCEEDED(hr)) return true; else return false;
	// This return value is the CLSID of our blank voice and must be used when developing the 
	// engine for this newly created voice
}

inline void LexHelper::UpdateWordAudioData(String ^ m_voicefile, String ^ m_wordtoupdate,short * m_newaudioinfo,
						 int m_sizenewaudioinfo)
{
// memory defrag ************************************************************************************
ULONG  HeapFragValue = 2;
HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
// **************************************************************************************************
pin_ptr<String ^> p = &m_voicefile;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voicefileU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voicefile)).ToPointer();//Unicode char set
//*********************************************************************************************
pin_ptr<String ^> pp = &m_wordtoupdate;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_wordtoupdateU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_wordtoupdate)).ToPointer();//Unicode char set
//*********************************************************************************************
// First and foremost is to consider the size of the original audio sample for the given word
int m_size =0;int m_audio_address=0;
short * m_useless =RetrieveAudioDataFromWord(m_voicefile,m_wordtoupdate, m_size,m_audio_address); delete [] m_useless;
// now we have three cases
long m_lenfile =GetVoiceFileLength(m_voicefile); // This operation must be done before opening a map to file
	
	if( m_sizenewaudioinfo == m_size) // here we merely have to operate a substitution without inflating 
	// or shrinking the whole file
	{
		// we must copy the  m_sizenewaudioinfo  bytes of   m_newaudioinfo at the m_audio_address location of our voicefile
		HANDLE HN = CreateFile(m_voicefileU,GENERIC_ALL ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);	
		HANDLE HP = CreateFileMapping(HN,NULL,PAGE_READWRITE,0,0,NULL);		
		HANDLE HX = MapViewOfFile(HP,FILE_MAP_ALL_ACCESS,0,0,0);
		unsigned char * pt = (unsigned char *)HX; // We cast to byte type 		
		memcpy_s(&pt[m_audio_address],m_lenfile,m_newaudioinfo,m_sizenewaudioinfo*sizeof(short));
		UnmapViewOfFile(HX); // This invalidate the handle so there's no need to close it
		CloseHandle(HP);
		CloseHandle(HN);
	}
	if( m_sizenewaudioinfo > m_size) // here we have to expand the voicefile size
	{
		int delta = m_sizenewaudioinfo-m_size; // the difference to inflate the voicefile 
		long newsize = m_lenfile + delta * sizeof(short);
		byte * half = new byte[delta * sizeof(short)];
		memset(half,0,delta * sizeof(short));
		HANDLE HN = CreateFile(m_voicefileU,GENERIC_ALL ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);	
		DWORD dwPos = SetFilePointer(HN, 0, NULL, FILE_END);
		//FILE_BEGIN FILE_CURRENT FILE_END
		DWORD m_written=0;// the reference of bytes written
		WriteFile(HN,half,delta * sizeof(short), &m_written,  NULL); 
		// We update the number of bytes for this audio sample
		dwPos = SetFilePointer(HN, m_audio_address-sizeof(DWORD), NULL, FILE_BEGIN);
		DWORD tmp = (DWORD)(m_sizenewaudioinfo) * sizeof(short);
		WriteFile(HN,&tmp,sizeof(DWORD),&m_written, NULL);
		// now we have made room for the bytes in excess. we now move the memory.
		HANDLE HP = CreateFileMapping(HN,NULL,PAGE_READWRITE,0,0,NULL);		
		HANDLE HX = MapViewOfFile(HP,FILE_MAP_ALL_ACCESS,0,0,0);
		unsigned char * pt = (unsigned char *)HX; // We cast to byte type 
		// we must shift ahead the memory that is comprised between (chunkaddress + sizechunkbefore) of difference   
		memcpy_s(&pt[m_audio_address + m_sizenewaudioinfo * sizeof(short)],  m_lenfile + (delta * sizeof(short)),
		&pt[m_audio_address+ m_size* sizeof(short)],m_lenfile- (m_size * sizeof(short) ) - m_audio_address);
		memcpy_s(&pt[m_audio_address],newsize,m_newaudioinfo,m_sizenewaudioinfo * sizeof(short));
		UnmapViewOfFile(HX); // This invalidate the handle so there's no need to close it
		CloseHandle(HP);
		CloseHandle(HN);
		delete [] half; // We have just inflated the voice file size

	}
	if( m_sizenewaudioinfo < m_size) // here we have to shrink the voicefile size
	{

		int delta = m_sizenewaudioinfo-m_size;
		int newsize = m_lenfile + (delta*sizeof(short));
		HANDLE HN = CreateFile(m_voicefileU,GENERIC_WRITE|GENERIC_READ ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_REPARSE_POINT|FILE_SHARE_WRITE|FILE_SHARE_READ|FILE_NOTIFY_CHANGE_SIZE,NULL);	
		HANDLE HP = CreateFileMapping(HN,NULL,PAGE_READWRITE,0,0,NULL);		
		HANDLE HX =MapViewOfFile(HP,FILE_MAP_ALL_ACCESS,0,0,0);
		unsigned char * pt = (unsigned char *)HX; // We cast to byte type
		// We update the number of bytes for this audio sample (the four bytes before audio data contains the length of audio data)
		DWORD dwPos = SetFilePointer(HN, m_audio_address - sizeof(DWORD), NULL, FILE_BEGIN);
		DWORD tmp = (DWORD)(m_sizenewaudioinfo * sizeof(short));
		DWORD m_written=0;// the reference of bytes written
		WriteFile(HN,&tmp,sizeof(DWORD),&m_written, NULL);
		memcpy_s(&pt[m_audio_address+m_sizenewaudioinfo*sizeof(short)],newsize,&pt[m_audio_address + m_size*sizeof(short)],
		m_lenfile-m_audio_address-m_size*sizeof(short));
		memcpy_s(&pt[m_audio_address],newsize,m_newaudioinfo,m_sizenewaudioinfo*sizeof(short));
		// now we can truncate the file to the new size
		// first we close the mapping
		UnmapViewOfFile(HX); // This invalidate the handle so there's no need to close it
		CloseHandle(HP);
		dwPos = SetFilePointer(HN, newsize, NULL, FILE_BEGIN);
		SetEndOfFile(HN);
		dwPos = SetFilePointer(HN, 0, NULL, FILE_BEGIN);
		CloseHandle(HN);
	}
}

String ^ LexHelper::CreateBlankVoice(void)
{
	HRESULT	hr;
	CLSID pguid=CreateGuiD();
	CComPtr<ISpObjectToken> cpToken;
	CComPtr<ISpDataKey> cpDataKeyAttribs;
	hr = SpCreateNewTokenEx(SPCAT_VOICES, L"The Blank Voice of Diego",
						&pguid, 
						L"Blank Voice", 
						0x410, 
						L"Blank Voice", 
						&cpToken,
						&cpDataKeyAttribs);	          
	hr = cpDataKeyAttribs->SetStringValue(L"Gender", L"Male");
	hr = cpDataKeyAttribs->SetStringValue(L"Name", L"The Blank Voice od Diego");
	hr = cpDataKeyAttribs->SetStringValue(L"Language", L"410");
	hr = cpDataKeyAttribs->SetStringValue(L"Age", L"Adult");
	hr = cpDataKeyAttribs->SetStringValue(L"Vendor", L"Diegus");
	hr = cpToken->SetStringValue(L"VoiceData", L"c:\\blank.vce");
	wchar_t * pguidString;
	StringFromCLSID(pguid,&pguidString);
	String ^ ret = gcnew String(pguidString);
	return  ret; // This return value is the CLSID of our blank voice and must be used when developing the 
	// engine for this newly created voice
}




void LexHelper::StopSpeak()
{
	// Stop current rendering with a PURGEBEFORESPEAK...
	HRESULT hr = pVoice->Speak( NULL, SPF_PURGEBEFORESPEAK, 0 );
}

long LexHelper::GetVoiceFileLength(String ^ m_voicefile)
{
pin_ptr<String ^> p = &m_voicefile;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voicefileU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voicefile)).ToPointer();//Unicode char set
//*********************************************************************************************
FILE  *hVoiceFile = NULL; 
// We open the voice file for binary reading
if (fopen_s(&hVoiceFile, CW2A(m_voicefileU), "rb") != 0 )
 {
   hVoiceFile = NULL;
   return 0;
 }
// Now we search for the word in the voicefile
int ret= fseek(hVoiceFile, 0, SEEK_END); // I move the file pointer to the end of file so I can read its length
long m_lenfile = ftell(hVoiceFile);// here I read the file length
fclose(hVoiceFile);
return m_lenfile;
}

short * LexHelper::RetrieveAudioDataFromWord(String ^ m_voicefile, String ^ m_wordm, int &m_sizesample, int &m_audio_address)
{
// memory defrag ************************************************************************************
ULONG  HeapFragValue = 2;
HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
HRESULT hr;
// **************************************************************************************************
pin_ptr<String ^> p = &m_voicefile;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voicefileU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voicefile)).ToPointer();//Unicode char set
//*********************************************************************************************
pin_ptr<String ^> pp = &m_wordm;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_wordU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_wordm)).ToPointer();//Unicode char set
//*********************************************************************************************
FILE  *hVoiceFile = NULL; 
// We open the voice file for binary reading
if (fopen_s(&hVoiceFile, CW2A(m_voicefileU), "rb") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return nullptr;
 }
// Now we search for the word in the voicefile
rewind(hVoiceFile);
fseek( hVoiceFile, sizeof(DWORD), SEEK_SET ); // The first four bytes are the file version
int m_numwords=0;
fread(&m_numwords,sizeof(DWORD),1,hVoiceFile);
// Now we proceed to read the size of the first word and its value
LPCWSTR m_word[512];
ULONG m_wordlength=0;
	for(int n = 0; n< m_numwords; ++n)
	{
		fread( &m_wordlength, sizeof(ULONG), 1, hVoiceFile ) &&
		fread(m_word, m_wordlength, 1, hVoiceFile );
		fread(&m_sizesample, sizeof(ULONG), 1, hVoiceFile ) ;
		// here we verify the word to see if it matches 
		if(wcscmp(m_wordU,(const wchar_t*)m_word)==0)
		{
			// here we allocate the memory for our audio sample
			short * m_audiodata= new short[m_sizesample];
			// we read all the audio data in this buffer
			m_audio_address= ftell(hVoiceFile); // The start address of audio data
			fread(m_audiodata,1,m_sizesample,hVoiceFile);
			// now we can close and return
			fclose( hVoiceFile );
			m_sizesample = m_sizesample/sizeof(short);
			return m_audiodata;
		}
		fseek( hVoiceFile, m_sizesample, SEEK_CUR ); // we move beyond the audio data to look for the next word
	}
fclose( hVoiceFile );
return nullptr; // no word is found
}


bool LexHelper::NormalizeAudioinVoiceFile(String ^ m_voicefile, float &m_coeffaverage)
{
// memory defrag ************************************************************************************
ULONG  HeapFragValue = 2;
HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
HRESULT hr;
// **************************************************************************************************
pin_ptr<String ^> pp = &m_voicefile;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voicefileU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voicefile)).ToPointer();//Unicode char set
//*********************************************************************************************
FILE  *hVoiceFile = NULL; 
// We open the voice file for binary read/write
if (fopen_s(&hVoiceFile, CW2A(m_voicefileU), "r+b") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return false;
  }
// Now we read all words in the voicefile
// in order to dimension the array of words we must read first the word number in the voice file.
rewind(hVoiceFile);
fseek( hVoiceFile, sizeof(DWORD), SEEK_SET ); // The first four bytes are the file version
int m_numwords=0;
fread(&m_numwords,sizeof(DWORD),1,hVoiceFile);
// Now we proceed to read the size of the first word and its value
WCHAR m_word[512];ULONG m_wordlength=0;ULONG m_sizesample=0;float m_coeff=0;int pos=0;
short * m_temp_audio;
for(int n = 0; n < m_numwords; n++)
{
	fread( &m_wordlength, sizeof(ULONG), 1, hVoiceFile ); 
	fread(m_word, m_wordlength, 1, hVoiceFile );
	fread(&m_sizesample, sizeof(ULONG), 1, hVoiceFile ) ;
	pos = ftell(hVoiceFile);
	m_temp_audio= new short[m_sizesample/sizeof(short)];	
	fread(m_temp_audio,1,m_sizesample,hVoiceFile);	
	short * res=NormalizeAudio(m_temp_audio,(m_sizesample/sizeof(short)),m_coeff);
	int j = ftell(hVoiceFile);
	fseek( hVoiceFile, pos, SEEK_SET ); // we reset the file pointer at the beginning of audio data	
	fwrite(m_temp_audio,1,m_sizesample,hVoiceFile);
	fseek( hVoiceFile, j, SEEK_SET ); // the file pointer is moved at the end of the normalized sample
	delete[] m_temp_audio;
	m_coeffaverage+=m_coeff;	
}
m_coeffaverage/=m_numwords;
rewind(hVoiceFile);
fclose( hVoiceFile );
return true;
}



bool LexHelper::SmoothEndsinVoiceFile(String ^ m_voicefile,int m_samplestosmooth)
{
// memory defrag ************************************************************************************
ULONG  HeapFragValue = 2;
HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
HRESULT hr;
// **************************************************************************************************
pin_ptr<String ^> pp = &m_voicefile;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voicefileU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voicefile)).ToPointer();//Unicode char set
//*********************************************************************************************
FILE  *hVoiceFile = NULL; 
// We open the voice file for binary read/write
if (fopen_s(&hVoiceFile, CW2A(m_voicefileU), "r+b") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return false;
  }
// Now we read all words in the voicefile
// in order to dimension the array of words we must read first the word number in the voice file.
rewind(hVoiceFile);
fseek( hVoiceFile, sizeof(DWORD), SEEK_SET ); // The first four bytes are the file version
int m_numwords=0;
fread(&m_numwords,sizeof(DWORD),1,hVoiceFile);
// Now we proceed to read the size of the first word and its value
WCHAR m_word[512];ULONG m_wordlength=0;ULONG m_sizesample=0;int pos=0;
short * m_temp_audio;
for(int n = 0; n < m_numwords; n++)
{
	fread( &m_wordlength, sizeof(ULONG), 1, hVoiceFile ); 
	fread(m_word, m_wordlength, 1, hVoiceFile );
	fread(&m_sizesample, sizeof(ULONG), 1, hVoiceFile ) ;
	pos = ftell(hVoiceFile);
	m_temp_audio= new short[m_sizesample/sizeof(short)];	
	fread(m_temp_audio,1,m_sizesample,hVoiceFile);	
	if(m_samplestosmooth < (int)(m_sizesample/sizeof(short))  ) // This check serves not to go out of memory because of trespassing boundaries
	{
		//**************************************************
		TriangularIncrease(m_temp_audio, m_samplestosmooth);
		TriangularFading(&m_temp_audio[(m_sizesample/sizeof(short))-m_samplestosmooth],  m_samplestosmooth); 
		//**************************************************	
	}
	int j = ftell(hVoiceFile);
	fseek( hVoiceFile, pos, SEEK_SET ); // we reset the file pointer at the beginning of audio data	
	fwrite(m_temp_audio,1,m_sizesample,hVoiceFile);
	fseek( hVoiceFile, j, SEEK_SET ); // the file pointer is moved at the end of the normalized sample
	delete[] m_temp_audio;
		
}

rewind(hVoiceFile);
fclose( hVoiceFile );
return true;
}


array <String^> ^ LexHelper::ListWordsInVoice(String ^ m_voicefile,int &m_voiceversion)
{
// memory defrag ************************************************************************************
ULONG  HeapFragValue = 2;
HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
HRESULT hr;
// **************************************************************************************************
pin_ptr<String ^> pp = &m_voicefile;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voicefileU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voicefile)).ToPointer();//Unicode char set
//*********************************************************************************************
FILE  *hVoiceFile = NULL; 
// We open the voice file for binary reading
if (fopen_s(&hVoiceFile, CW2A(m_voicefileU), "rb") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return nullptr;
  }
// Now we read all words in the voicefile
// in order to dimension the array of words we must read first the word number in the voice file.
rewind(hVoiceFile);
// The sample rate of the voice, that is the voice quality is the first DWORD of the file
fread(&m_voiceversion,sizeof(DWORD),1,hVoiceFile);
//fseek( hVoiceFile, sizeof(DWORD), SEEK_SET ); // The first four bytes are the file version
int m_numwords=0;
fread(&m_numwords,sizeof(DWORD),1,hVoiceFile);
array <String^,1>^ m_wordlist = gcnew array <String^,1>(m_numwords);
// Now we proceed to read the size of the first word and its value
LPCWSTR m_word[512];
ULONG m_wordlength=0;
ULONG m_sizesample=0;
IntPtr temp=IntPtr::Zero;
for(int n = 0; n< m_numwords; ++n)
{
	fread( &m_wordlength, sizeof(ULONG), 1, hVoiceFile ) &&
	fread(m_word, m_wordlength, 1, hVoiceFile );
	// We convert the UNICODE POINTER to managed string and add it to our array
	temp = (IntPtr)m_word;
	m_wordlist[n] =  Marshal::PtrToStringUni(temp);
	// now we can read the size of the correspondent audio data;
	// First we write the number of bytes
	fread(&m_sizesample, sizeof(ULONG), 1, hVoiceFile ) ;
	// we now have to move the file pointer ahead of m_sizesample bytes in order to reach the next word
	fseek( hVoiceFile, m_sizesample, SEEK_CUR );
}
rewind(hVoiceFile);
fclose( hVoiceFile );
return m_wordlist;
}

void LexHelper::AppendWordToVoiceUsingCurrentAudio(String ^ WordtoAppend,String ^ m_voicefile, short * m_audiodata, int m_sizeaudiodata)
{
// memory defrag ************************************************************************************
ULONG  HeapFragValue = 2;
HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
HRESULT hr;
// **************************************************************************************************
pin_ptr<String ^> p = &WordtoAppend;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR WordtoAppendU =(LPCWSTR)(Marshal::StringToHGlobalUni(WordtoAppend)).ToPointer();//Unicode char set
pin_ptr<String ^> pp = &m_voicefile;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voicefileU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voicefile)).ToPointer();//Unicode char set
//*********************************************************************************************
ULONG m_sizesample= 2* m_sizeaudiodata; // the amount in bytes of the sample
FILE  *hVoiceFile = NULL; 
// We open the voice file for binary writing
if (fopen_s(&hVoiceFile, CW2A(m_voicefileU), "a+b") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return;
  }
// Now we write all the data for the specific word
size_t ulTextLen=wcslen(WordtoAppendU);
ulTextLen = (ulTextLen+1) * sizeof(WCHAR); // The amount of bytes.
if( fwrite( &ulTextLen, sizeof(ulTextLen), 1, hVoiceFile ) &&
    fwrite( WordtoAppendU, ulTextLen, 1, hVoiceFile ) )
// we can append the audio for the unknown word
{
// First we write the number of bytes
fwrite(&m_sizesample, sizeof(m_sizesample), 1, hVoiceFile ) ;
// Now we can write the audio content ahead of our voice file
fwrite( (BYTE*)m_audiodata, 1, m_sizesample, hVoiceFile ); 
// now we must update the word number counter which is in a dword from byte 5 to byte 8 in the voice file

// We cannot update the word count here because the file was opened in append mode; 
// We now close it and then we reopen with w+ flag
fclose( hVoiceFile );
if (fopen_s(&hVoiceFile, CW2A(m_voicefileU), "r+b") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return;
  }

fseek( hVoiceFile, sizeof(DWORD), SEEK_SET );
DWORD wordnum=0;
fread(&wordnum,sizeof(DWORD),1,hVoiceFile); // we have collected the number of word
wordnum+=1;// we increment it

fseek( hVoiceFile, sizeof(DWORD), SEEK_SET );
fwrite(&wordnum,sizeof(DWORD),1,hVoiceFile);
// We can close now
fclose( hVoiceFile );
}

}


void LexHelper::AppendWordToVoice(String ^ WordtoAppend,String ^ m_voicefile, String ^ m_wordaudiofilename)
{
// memory defrag ************************************************************************************
ULONG  HeapFragValue = 2;
HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
HRESULT hr;
// **************************************************************************************************
pin_ptr<String ^> p = &WordtoAppend;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR WordtoAppendU =(LPCWSTR)(Marshal::StringToHGlobalUni(WordtoAppend)).ToPointer();//Unicode char set
pin_ptr<String ^> pp = &m_voicefile;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voicefileU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voicefile)).ToPointer();//Unicode char set
pin_ptr<String ^> ppp = &m_wordaudiofilename;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_wordaudiofilenameU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_wordaudiofilename)).ToPointer();//Unicode char set
//*********************************************************************************************
//*********************************************************************************************
FILE  *hVoiceFile = NULL; 
// We open the voice file for binary writing
if (fopen_s(&hVoiceFile, CW2A(m_voicefileU), "a+b") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return;
  }
// Now we write all the data for the specific word
size_t ulTextLen=wcslen(WordtoAppendU);
ulTextLen = (ulTextLen+1) * sizeof(WCHAR); // The amount of bytes.
if( fwrite( &ulTextLen, sizeof(ulTextLen), 1, hVoiceFile ) &&
    fwrite( WordtoAppendU, ulTextLen, 1, hVoiceFile ) )
// we can append the audio for the unknown word
{
ULONG m_sizesample;
BYTE* Buff=NULL;
ISpStream* pStream;
hr = SPBindToFile(m_wordaudiofilenameU, SPFM_OPEN_READONLY, &pStream );
CSpStreamFormat Fmt;
Fmt.AssignFormat(pStream);
if( Fmt.ComputeFormatEnum() == SPSF_44kHz16BitMono ||
   Fmt.ComputeFormatEnum() == SPSF_22kHz16BitMono ||
   Fmt.ComputeFormatEnum() == SPSF_16kHz16BitMono ||
   Fmt.ComputeFormatEnum() == SPSF_11kHz16BitMono  )
   { 
	   tagSTATSTG Stat;
	   hr = pStream->Stat(&Stat, STATFLAG_NONAME );
	   m_sizesample = Stat.cbSize.LowPart;
	   Buff = new BYTE[m_sizesample];  
	   hr = pStream->Read( Buff, m_sizesample, NULL );
	   pStream->Close();
	   pStream->Release();		  
   }
   else
   { m_sizesample=0; }
// First we write the number of bytes
fwrite(&m_sizesample, sizeof(m_sizesample), 1, hVoiceFile ) ;
// Now we can write the audio content ahead of our voice file
fwrite( Buff, 1, m_sizesample, hVoiceFile ); 
// we free  
delete [] Buff;
// now we must update the word number counter which is in a dword from byte 5 to byte 8 in the voice file
fseek( hVoiceFile, sizeof(DWORD), SEEK_SET );
DWORD wordnum=0;
fread(&wordnum,sizeof(DWORD),1,hVoiceFile); // we have collected the number of word
wordnum+=1;// we increment it
// We cannot update the word count here because the file was opened in append mode; 
// We now close it and then we reopen with w+ flag
fclose( hVoiceFile );
if (fopen_s(&hVoiceFile, CW2A(m_voicefileU), "r+b") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return;
  }
fseek( hVoiceFile, sizeof(DWORD), SEEK_SET );
fwrite(&wordnum,sizeof(DWORD),1,hVoiceFile);
// We can close now
fclose( hVoiceFile );
}
}



int LexHelper::GetAllocationGranularity()
{
	SYSTEM_INFO syInfo;
	GetSystemInfo(&syInfo);
	return (int)syInfo.dwAllocationGranularity;			
}


// PERFORMING INVERSE FFT to get back the original result ComplexFFT(vector,sample_rate,-1);
void LexHelper::ComplexFFT(double *vector, unsigned int sample_rate, int sign)
{	//variables for the fft
	unsigned long n,mmax,m,j,istep,i;
	double wtemp,wr,wpr,wpi,wi,theta,tempr,tempi;
	n=sample_rate << 1;
	j=0;
	for (i=0;i<n/2;i+=2) {
		if (j > i) {
			SWAP(vector[j],vector[i]);
			SWAP(vector[j+1],vector[i+1]);
			if((j/2)<(n/4)){
				SWAP(vector[(n-(i+2))],vector[(n-(j+2))]);
				SWAP(vector[(n-(i+2))+1],vector[(n-(j+2))+1]);
			}
		}
		m=n >> 1;
		while (m >= 2 && j >= m) {
			j -= m;
			m >>= 1;
		}
		j += m;
	}
	//end of the bit-reversed order algorithm Danielson-Lanzcos routine
	mmax=2;
	while (n > mmax) {
		istep=mmax << 1;
		theta=sign*(2*Math::PI/mmax);
		wtemp=Math::Sin(0.5F*theta);
		wpr = -2.0*wtemp*wtemp;
		wpi=Math::Sin(theta);
		wr=1.0;
		wi=0.0;
		for (m=1;m<mmax;m+=2) {
			for (i=m;i<=n;i+=istep) {
				j=i+mmax;
				tempr=wr*vector[j-1]-wi*vector[j];
				tempi=wr*vector[j]+wi*vector[j-1];
				vector[j-1]=vector[i-1]-tempr;
				vector[j]=vector[i]-tempi;
				vector[i-1] += tempr;
				vector[i] += tempi;
			}
			wr=(wtemp=wr)*wpr-wi*wpi+wr;
			wi=wi*wpr+wtemp*wpi+wi;
		}
		mmax=istep;
	}
	if (sign==-1) {
		for (i=0; i<n; i++)
			vector[i] /= sample_rate;}

}


void  LexHelper::CreateLexiconFromDB(String^ m_dbpath, String ^ m_archivepath)
{// For a 32 bit solution using CDaodatabase MFC class (deprecated see the ttsconverter solution in the main folder
		Dictionnary ** m_allwords;	
		
		int sizewords;		   
		String ^ m_cns = "Provider=Microsoft.ACE.OLEDB.12.0;Data Source=" + 
		m_dbpath + ";Persist Security Info=False";
		OleDbConnection ^ m_cn = gcnew OleDbConnection();
		m_cn->ConnectionString=m_cns;
		OleDbDataAdapter ^ m_ada = gcnew OleDbDataAdapter("select trim(written),trim(pronounced) from words",m_cn);
		DataSet ^ m_ds = gcnew DataSet("Words");
		m_ada->Fill(m_ds);		
		m_cn = nullptr;
		m_ada = nullptr;
		sizewords = m_ds->Tables[0]->Rows->Count;
		m_allwords = new Dictionnary*[sizewords];
		CFile Fi((CString)m_archivepath,CFile::modeCreate|CFile::modeWrite);
		Fi.Write(&sizewords,sizeof(int)); // we write the size so we know  how to dimension the array
		CArchive archive(&Fi, CArchive::store);

			for (int n = 0 ; n< sizewords;n++)
			{
			
				m_allwords[n]= new Dictionnary(  (CString) (m_ds->Tables[0]->Rows[n][0]->ToString()),
				(CString)(m_ds->Tables[0]->Rows[n][1]->ToString()));
					
			
			}	

// this fragment sorts the Database if it is not already sorted however just to be sure;
// //Sorting is mandatory in order to perform Binary search correctly
// We don't trust the sorting process that takes place in the select query of database
		int k,i,j,number;
		Dictionnary  *hdic;
		hdic= new Dictionnary;
		number=sizewords;
		k=sizewords/2;
		  while (k>0)
		  {
			  for (i=0; i<number-k; ++i)
			 {
			 j=i;
				 while (j>=0 &&  (  m_allwords[j]->GetWritten() >m_allwords[j+k]->GetWritten()) )
				 {  
					 *hdic =*m_allwords[j];
					 *m_allwords[j]=*m_allwords[j+k];
					* m_allwords[j+k]=*hdic;
					 j=j-k;    
				 }
			 }
			 k=k/2;
		  }
		delete hdic;


for (int n = 0 ; n< sizewords;n++)
			{
						
				m_allwords[n]->Serialize(archive);
				
			
			}	

			archive.Close();
			Fi.Close();

for (int n = 0 ; n< sizewords;n++)
			{				
				
				delete m_allwords[n];			
			
			}	






			delete[] m_allwords;

			m_allwords=nullptr;
			m_ds=nullptr;






}








