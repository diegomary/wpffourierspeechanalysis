// corelexicon.h
#pragma once
#include "sapi.h"
#include "sphelper.h" 
#include <sphelper.h>
#include <spuihelp.h>
#include <math.h>


using namespace System;
using namespace System::Collections;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;
using namespace System::Data;
using namespace System::Data::OleDb;
using namespace System::Threading;
using namespace System::Drawing::Imaging;
using namespace System::IO;

#define SWAP(a,b) {tempr=(a);(a)=(b);(b)=tempr;}
// *****************************GLOBAL DEFINES FOR AUDIO RECORDING************************
#define SAMPLESPERFRAME 400
#define MM_BUF_FRAMES   20 
#define MM_BUF_SIZE     (SAMPLESPERFRAME * MM_BUF_FRAMES)  // = 16000 bytes that are 8000 samples
#define NUM_BUFFERS		2 // Circular buffer
namespace corelexicon {
// *****************************GLOBAL VARS and CALLBACK FOR AUDIO RECORDING************************
HWAVEIN hwi;
WAVEFORMATEX wfx;
WAVEHDR whdr[NUM_BUFFERS];
int m_bufferSize;


//unsigned char wavBuffer[2][MM_BUF_SIZE];
char ** wavBuffer;
int recording    = 0;
HANDLE hAudiofile; // This is the temporary file used to record data from the microphone
int m_min_recthreshold;
HWAVEOUT hw;	
WAVEHDR headwav0; // this is for playing the speech
// These are global vars used to choose the voice and speak text fragments
ISpObjectToken  *pCurVoiceToken;
CComPtr<ISpVoice>   pVoice;
//***************************************************************************
 bool AnalyzeIntensityOfsamples(LPSTR  data, int m_numbytes)
{
	if(m_numbytes==0) return false; // this condition is crucial because one of the two buffers has 
	// no bytesrecorded when stopping the recording 
	short * temp = (short * )data; int m_numsamples = m_numbytes /sizeof(short);
	int sum =0;
	int mean =0;
	int counter=0;	
	
	//*****************************************************
	for(int c = 0; c< m_numsamples;++c)
	 {
		if(m_min_recthreshold <  (abs( temp[c])) ) counter+=1;
	 }	
	double coeff = ((double)counter)/ ((double)m_numsamples);
	if(coeff < 0.7) return false;else return true;
	
	//********************************************************
	
	//for(int c = 0; c< m_numsamples;++c)
	// {
	//	  sum += abs(temp[c]);
	// }	
	//if (sum==0)mean =0;	
	//else{mean = sum/m_numsamples;}
	////  we don't want to record what is below a threshold
	//if(mean > m_min_recthreshold )
	//return true;
	//else return false;





}
// We have commented the signature because it doesn't work either on 32 an 64 bit
void CALLBACK waveInCallback(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD* dwParam1, DWORD dwParam2)
//void CALLBACK waveInCallback(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	WAVEHDR *pwhdr;
	int res;
	pwhdr = (WAVEHDR*)dwParam1;
	if (uMsg == WIM_DATA)
	{
	DWORD sz=	GetFileSize(hAudiofile,nullptr);
		if(sz==0)
		{
			// We analyze if it's worth recording only 
			if(AnalyzeIntensityOfsamples(pwhdr->lpData,pwhdr->dwBytesRecorded))
			{		
				DWORD m_written=0;
				WriteFile(hAudiofile,pwhdr->lpData,pwhdr->dwBytesRecorded,&m_written,NULL);		
			}
		}
		else
		{
		DWORD m_written=0;
		WriteFile(hAudiofile,pwhdr->lpData,pwhdr->dwBytesRecorded,&m_written,NULL);	
		
		}


if (recording)
		{
			waveInUnprepareHeader(hwi, pwhdr, sizeof(WAVEHDR)); // It works without this line
			waveInPrepareHeader(hwi, pwhdr, sizeof(WAVEHDR)); // it works without this line
			pwhdr->dwBytesRecorded=0;
			res = waveInAddBuffer(hwi, pwhdr, sizeof(WAVEHDR));
		}
	}
}
//********************************************************************************************************
public ref class PackDCHWND // This class is used to store in a single object various data for
// Drawing the recorded/rejected data on the picturebox
{
public: 
	IntPtr m_hdc;
	IntPtr m_hwnd;
	int m_threshold;
	int m_samplerate;
	System::Drawing::Graphics ^ grap;
	System::Windows::Forms::PictureBox ^ m_pbox;
	public:

PackDCHWND()
{
	/*m_hdc=IntPtr::Zero; 
	m_hwnd=IntPtr::Zero;*/
    m_threshold=0;
	m_samplerate=11025;
    grap=nullptr;
    m_pbox=nullptr;
} 



void operator =(const PackDCHWND %r)
	{
		m_hdc=r.m_hdc;
		m_hwnd=r.m_hwnd;	
		m_threshold=r.m_threshold;
		m_samplerate=r.m_samplerate;
		grap=r.grap;
		m_pbox=r.m_pbox;
	}
};


//**************************************************************
	public ref class LexHelper
	{
	//**********************************************************************************************		
	private:
	Thread ^ m_thread_rec;
	Thread ^ m_thread_recFFT;
	Thread ^ m_thread_recSPECTRUM;
	int m_freq_ext;
	bool is_playing_phonema; // It serves for stopping a current audio playing
	public:
	enum class LANGUAGES{ENGLISH,ITALIAN,FRENCH,SPANISH};	
	LexHelper();
	~LexHelper();
	public:

		bool InsertWord(String ^ WordToIns, String ^ Pronunciation, int PartOfSpeech,LANGUAGES curlang)
		{		
			
			if(WordToIns == nullptr || WordToIns == String::Empty || Pronunciation == nullptr || Pronunciation == String::Empty ) return false;
			pin_ptr<String ^> p = &WordToIns;  // We Block Garbage collection relocation to grant safety
			// to the Unmanaged pointer that represents the filename in the native env.
			LPCWSTR WordToInsU =(LPCWSTR)(Marshal::StringToHGlobalUni(WordToIns)).ToPointer();//Unicode char set
			pin_ptr<String ^> pp = &Pronunciation;  // We Block Garbage collection relocation to grant safety
			// to the Unmanaged pointer that represents the filename in the native env.
			LPCWSTR PronunciationU =(LPCWSTR)(Marshal::StringToHGlobalUni(Pronunciation)).ToPointer();//Unicode char set		
			HRESULT                      hr= S_OK ;
			CComPtr<ISpPhoneConverter>   cpPhoneConv;
			CComPtr<ISpLexicon>          cpLexicon;
			LANGID                       langidUS;
			SPPHONEID                    wszId[SP_MAX_PRON_LENGTH];
			hr = cpLexicon.CoCreateInstance(CLSID_SpLexicon);		
			
			switch(curlang)
			{
				case LANGUAGES::ENGLISH:
				langidUS = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
				break;

				case LANGUAGES::ITALIAN:
				langidUS = MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN);
				break;

				case LANGUAGES::FRENCH:
				langidUS = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
				break;

				case LANGUAGES::SPANISH:
				langidUS = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH);
				break;
			}			
			
			   hr = SpCreatePhoneConverter(langidUS, NULL, NULL, &cpPhoneConv);		
			   hr = cpPhoneConv->PhoneToId(PronunciationU, wszId);			
			   hr = cpLexicon->AddPronunciation(WordToInsU, langidUS, (SPPARTOFSPEECH)PartOfSpeech, wszId);
			   if(SUCCEEDED(hr))
			   {	cpPhoneConv.Release();
					cpLexicon.Release();
				    return true;
			   }			   
			   else 
			   {	
				   cpPhoneConv.Release();
				   cpLexicon.Release();
				   return false;
			   }		   
			
		}

//***************************************************************************************************
bool RemoveWord(String ^ WordToRemove, LANGUAGES curlang, int partOfSpeech, String ^ WordPronunciation)
		{			
			pin_ptr<String ^> p = &WordToRemove;  // We Block Garbage collection relocation to grant safety
			// to the Unmanaged pointer that represents the filename in the native env.
			LPCWSTR WordToRemoveU =(LPCWSTR)(Marshal::StringToHGlobalUni(WordToRemove)).ToPointer();//Unicode char set
			pin_ptr<String ^> pp = &WordPronunciation;  // We Block Garbage collection relocation to grant safety
			// to the Unmanaged pointer that represents the filename in the native env.
			LPCWSTR WordPronunciationU =(LPCWSTR)(Marshal::StringToHGlobalUni(WordPronunciation)).ToPointer();//Unicode char set
	
			SPWORDPRONUNCIATIONLIST spwordpronlist;
			HRESULT                      hr= S_OK ;
			CComPtr<ISpPhoneConverter>   cpPhoneConv;
			CComPtr<ISpLexicon>          cpLexicon;
			LANGID                       langidUS;
			WCHAR  pronounced[SP_MAX_WORD_LENGTH];
			hr = cpLexicon.CoCreateInstance(CLSID_SpLexicon);

			switch(curlang)
			{
				case LANGUAGES::ENGLISH:
				langidUS = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
				break;

				case LANGUAGES::ITALIAN:
				langidUS = MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN);
				break;

				case LANGUAGES::FRENCH:
				langidUS = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
				break;

				case LANGUAGES::SPANISH:
				langidUS = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH);
				break;
			}

			 hr = SpCreatePhoneConverter(langidUS, NULL, NULL, &cpPhoneConv);
			 //ISpLexicon::RemovePronunciation removes a word and its pronunciations from a user lexicon	
			 memset(&spwordpronlist, 0, sizeof(spwordpronlist));
			 hr=  cpLexicon->GetPronunciations(WordToRemoveU, langidUS, eLEXTYPE_USER |
			 eLEXTYPE_APP , &spwordpronlist);

			 for (SPWORDPRONUNCIATION * pwordpron = spwordpronlist.pFirstWordPronunciation;
				  pwordpron != NULL;pwordpron = pwordpron->pNextWordPronunciation)
			 {
				cpPhoneConv->IdToPhone(pwordpron->szPronunciation,pronounced);
				if( (pwordpron->ePartOfSpeech == (SPPARTOFSPEECH)partOfSpeech) && ( wcscmp(pronounced,WordPronunciationU) ==0  ))
				hr = cpLexicon->RemovePronunciation(WordToRemoveU, langidUS, (SPPARTOFSPEECH)partOfSpeech, pwordpron->szPronunciation);			 
			 }
			 
			 if(SUCCEEDED(hr))
			 {
				cpPhoneConv.Release();
			    cpLexicon.Release();							   
				return true;
			 }
			 
			 else
			 {	
				cpPhoneConv.Release();
				cpLexicon.Release();					
				return false;
			 }
		}
//******************************************************************************************
		DataTable ^ ListWordsInUserLexicon(LANGUAGES curlang)
		{
			
			DataTable ^ TB = gcnew DataTable("Lexicon");
			DataColumn ^ dcw = gcnew DataColumn("Word",System::Type::GetType("System.String"));
			DataColumn ^ dcp = gcnew DataColumn("Pronunciation",System::Type::GetType("System.String"));
			DataColumn ^ dcpps = gcnew DataColumn("Part of speech",System::Type::GetType("System.String"));
			TB->Columns->Add(dcw);
			TB->Columns->Add(dcp);
			TB->Columns->Add(dcpps);			
			HRESULT                      hr=S_FALSE;
			//::CoInitialize( NULL );	
			CComPtr<ISpLexicon> spLex;
			CComPtr<ISpPhoneConverter>   cpPhoneConv;
			LANGID                       langidUS;
			WCHAR  pronounced[SP_MAX_WORD_LENGTH];
			SPWORDPRONUNCIATIONLIST spwordpronlist;			
			switch(curlang)
			{
				case LANGUAGES::ENGLISH:
				langidUS = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
				break;

				case LANGUAGES::ITALIAN:
				langidUS = MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN);
				break;

				case LANGUAGES::FRENCH:
				langidUS = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
				break;

				case LANGUAGES::SPANISH:
				langidUS = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH);
				break;

			}			
			hr = SpCreatePhoneConverter(langidUS, NULL, NULL, &cpPhoneConv);
			SPWORDLIST speechWords;			
			DWORD dwGeneration = 0;
			DWORD dwCookie = 0;
			spLex.CoCreateInstance(CLSID_SpLexicon);
			memset(&speechWords, 0, sizeof(speechWords));
			hr = spLex->GetWords(eLEXTYPE_USER|eLEXTYPE_APP,&dwGeneration,&dwCookie,&speechWords);
			for (SPWORD *pword = speechWords.pFirstWord;pword != NULL; pword = pword->pNextWord)
			  {
					 if (pword->LangID == langidUS)
					 {
					 memset(&spwordpronlist, 0, sizeof(spwordpronlist));
					 hr=  spLex->GetPronunciations(pword->pszWord, langidUS, eLEXTYPE_USER |eLEXTYPE_APP
				     , &spwordpronlist);				 
					 for (SPWORDPRONUNCIATION * pwordpron = spwordpronlist.pFirstWordPronunciation;
							 pwordpron != NULL;
							 pwordpron = pwordpron->pNextWordPronunciation)
						{
							 cpPhoneConv->IdToPhone(pwordpron->szPronunciation,pronounced);										
							 DataRow ^ dr =   TB->NewRow(); dr[0]=gcnew String(pword->pszWord); dr[1]=gcnew String(pronounced);
							 switch(pwordpron->ePartOfSpeech)
							 {
							 case SPPS_NotOverriden:
								dr[2]="Part of speech SHOULD NOT BE OVERWRITTEN";
								break;
							 case SPPS_Unknown :
								dr[2]="Part of speech is UNKNOWN";
								break;
							 case SPPS_Noun  :
								dr[2]="Part of speech is a NOUN.";
								break;
							 case SPPS_Verb  :
								dr[2]="Part of speech is a VERB.";
								break;
							 case SPPS_Modifier   :
								dr[2]="Part of speech is a MODIFIER.";
								break;
							 case SPPS_Function   :
								dr[2]="Part of speech is a FUNCTION.";
								break;
							 case SPPS_Interjection :
								dr[2]="Part of speech is an INTERJECTION.";
								break;
							 case SPPS_Noncontent   :
								dr[2]="Part of speech WITH NO SEMANTIC VALUE.";
								break;
							 case SPPS_LMA   :
								dr[2]="PPS SPPS_LMA (see reference guide).";
								break;
							 case SPPS_SuppressWord  :
								dr[2]="This part of speech SHOULD NOT BE RECOGNIZED.";
								break;					 
							 }							 
							 TB->Rows->Add(dr);				
							 }
					 }
				  }
			CoTaskMemFree(speechWords.pvBuffer);
			CoTaskMemFree(speechWords.pFirstWord);
			cpPhoneConv.Release();
			spLex.Release();			
		
			TB->DefaultView->Sort = String::Format("{0} {1}", "Word", "Asc");
			return TB->DefaultView->Table;
		}
int GetLangIDIntegerForm(LANGUAGES curlang)
{
	int LangID;
	switch(curlang)
		{
			case LANGUAGES::ENGLISH:
			LangID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
			break;
			case LANGUAGES::ITALIAN:
			LangID = MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN);
			break;
			case LANGUAGES::FRENCH:
			LangID = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
			break;
			case LANGUAGES::SPANISH:
			LangID = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH);
			break;
		}	
	return LangID;
}


BYTE * GetAudioDataFromPhonemaFile(String ^ FileName, int &m_sizesample, int &m_checked)
{
pin_ptr<String ^> p = &FileName;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR FileNameU =(LPCWSTR)(Marshal::StringToHGlobalUni(FileName)).ToPointer();//Unicode char set
BYTE* Buff=NULL;
ISpStream* pStream;
HRESULT hr = SPBindToFile(FileNameU, SPFM_OPEN_READONLY, &pStream );
CSpStreamFormat Fmt;
Fmt.AssignFormat(pStream);

   if( (Fmt.ComputeFormatEnum() == SPSF_44kHz16BitMono) ||
	   (Fmt.ComputeFormatEnum() == SPSF_22kHz16BitMono) ||
	   (Fmt.ComputeFormatEnum() == SPSF_16kHz16BitMono)||
	   (Fmt.ComputeFormatEnum() == SPSF_11kHz16BitMono)  )
   {
		   m_checked=Fmt.ComputeFormatEnum();
		   tagSTATSTG Stat;
		   hr = pStream->Stat(&Stat, STATFLAG_NONAME );
		   m_sizesample = Stat.cbSize.LowPart;
		   Buff = new BYTE[m_sizesample];  
		   hr = pStream->Read( Buff, m_sizesample, NULL );
		   pStream->Close();
		   pStream->Release();
		   return Buff;
   }
   else
   {
	   m_sizesample=0;
	   return NULL;  
   }
}





BYTE * GetAudioDataFrom44khzPCM(String ^ FileName, int &m_sizesample)

{
pin_ptr<String ^> p = &FileName;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR FileNameU =(LPCWSTR)(Marshal::StringToHGlobalUni(FileName)).ToPointer();//Unicode char set
BYTE* Buff=NULL;
FILE *fp; 
errno_t err= _wfopen_s(&fp,FileNameU, _T("r+b"));
  fseek(fp, 0, SEEK_END);
  int lenBuffer=ftell(fp);
  rewind(fp);
  Buff = new BYTE[lenBuffer]; // we must add silence to the end and to the beginning
  memset(Buff,0,lenBuffer);
  fread(Buff, 1, lenBuffer, fp);
  fclose(fp);
  m_sizesample = lenBuffer;
  return  Buff;

}

void SetAudioDataIn44khzPCMFile(String ^ m_path,String ^ FileName, BYTE * buffer,int m_sizesample)
{
	

FileName = m_path+FileName+ ".pcm";
pin_ptr<String ^> p = &FileName;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.


LPCWSTR FileNameU =(LPCWSTR)(Marshal::StringToHGlobalUni(FileName)).ToPointer();//Unicode char set
BYTE* Buff=NULL;
FILE *fp; 
errno_t err= _wfopen_s(&fp,FileNameU, _T("w+b"));
fwrite(buffer, 1, m_sizesample, fp);
fclose(fp);
}







short * GetAudioPhonema(String ^ FileName,int m_index, int &m_sizesample)
{
pin_ptr<String ^> p = &FileName;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR FileNameU =(LPCWSTR)(Marshal::StringToHGlobalUni(FileName)).ToPointer();//Unicode char set
// memory defrag ************************************************************************************
ULONG  HeapFragValue = 2;
HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
HRESULT hr;
// **************************************************************************************************
int len =0;
FILE  *hVoiceFile = NULL; 
// We open the voice file for binary reading
if (fopen_s(&hVoiceFile, CW2A(FileNameU), "rb") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return nullptr;
  }
rewind(hVoiceFile);
fread(&len,sizeof(int),1,hVoiceFile);

int * countlen = new int[len];
fread(countlen,sizeof(int)*len,1,hVoiceFile);


for(int n =0; n < m_index; n++)
{
fseek( hVoiceFile, countlen[n], SEEK_CUR );
}
byte *soundsbuffer = new  byte[countlen[m_index]];
fread(soundsbuffer,1,countlen[m_index],hVoiceFile);
m_sizesample=countlen[m_index]/sizeof(short);
delete[] countlen;
fclose(hVoiceFile);
return (short*)soundsbuffer;
}


void CreateEnglishPhoneSetFile(String ^ FileName,int len)
{
pin_ptr<String ^> p = &FileName;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR FileNameU =(LPCWSTR)(Marshal::StringToHGlobalUni(FileName)).ToPointer();//Unicode char set
// memory defrag ************************************************************************************
ULONG  HeapFragValue = 2;
HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
HRESULT hr;
// **************************************************************************************************
FILE  *hVoiceFile = NULL; 
// We create the file
if (fopen_s(&hVoiceFile, CW2A(FileNameU), "wb") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return;
  }
rewind(hVoiceFile);

fwrite(&len,sizeof(int),1,hVoiceFile);
int * countlen = new int[len];
for(int n = 0; n<len; n++) countlen[n] = 5000; // 11 mseconds initial
fwrite(countlen,sizeof(int)*len,1,hVoiceFile);
// at this point we can write all the 47 empty buffers of 1000 bytes each;
byte * temp = new byte[5000];
memset(temp,0,5000);
// The following sample generates an initial 55 samples of sound with a 440 frequency
short * temp1 = (short*)temp;
double amplitude = 0.25 * MAXSHORT ;
double frequency = 440;
for (int n = 0; n < 2500; n++)  temp1[n] = ((short)(amplitude * sin((2 * Math::PI * n * frequency) / 44100)));
// other frequencies can be summed
//+ 
//((short)(amplitude * sin((2 * 3.141592653 * n * frequency1) / 44100)))
//+
//((short)(amplitude * sin((2 * 3.141592653 * n * frequency2) / 44100)))
// Once we have the signal we write all the phonemes
for(int n =0; n< len; n++) fwrite(temp,1,5000,hVoiceFile);
delete[] temp;
fclose(hVoiceFile);
}

void CreateItalianPhoneSetFile(String ^ FileName)
{
pin_ptr<String ^> p = &FileName;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR FileNameU =(LPCWSTR)(Marshal::StringToHGlobalUni(FileName)).ToPointer();//Unicode char set
// memory defrag ************************************************************************************
ULONG  HeapFragValue = 2;
HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
HRESULT hr;
// **************************************************************************************************
FILE  *hVoiceFile = NULL; 
// We create the file
if (fopen_s(&hVoiceFile, CW2A(FileNameU), "wb") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return;
  }
rewind(hVoiceFile);
int len =36;
fwrite(&len,sizeof(int),1,hVoiceFile);
int * countlen = new int[len];
for(int n = 0; n<len; n++) countlen[n] = 5000; // 11 mseconds initial
fwrite(countlen,sizeof(int)*len,1,hVoiceFile);
// at this point we can write all the 47 empty buffers of 1000 bytes each;
byte * temp = new byte[5000];
memset(temp,0,5000);
// The following sample generates an initial 55 samples of sound with a 440 frequency
short * temp1 = (short*)temp;
double amplitude = 0.25 * MAXSHORT ;
double frequency = 440;
for (int n = 0; n < 2500; n++)  temp1[n] = ((short)(amplitude * sin((2 * Math::PI * n * frequency) / 44100)));
// other frequencies can be summed
//+ 
//((short)(amplitude * sin((2 * 3.141592653 * n * frequency1) / 44100)))
//+
//((short)(amplitude * sin((2 * 3.141592653 * n * frequency2) / 44100)))
// Once we have the signal we write all the phonemes
for(int n =0; n< len; n++) fwrite(temp,1,5000,hVoiceFile);
delete[] temp;
fclose(hVoiceFile);
}



void SetAudioDataIntoPhonemaFile(String ^ FileName, int FormatAudio, BYTE * data, int m_sizedata)
{
	HRESULT hr;
	pin_ptr<String ^> p = &FileName;  // We Block Garbage collection relocation to grant safety
	// to the Unmanaged pointer that represents the filename in the native env.
	LPCWSTR FileNameU =(LPCWSTR)(Marshal::StringToHGlobalUni(FileName)).ToPointer();//Unicode char set
	//*********************************************************************************************
		ISpStream * cpStream = NULL; // Pass-through object between the voice and file stream object 
		ISpStream * cpFileStream = NULL; // Actually generates wave file
		CSpStreamFormat format((SPSTREAMFORMAT)FormatAudio, &hr); //set the format for the wav 
		hr = SPBindToFile(FileNameU,SPFM_CREATE_ALWAYS,&cpFileStream, &format.FormatId(),format.WaveFormatExPtr() ); 	
		// Create pass-through stream and set its base stream to be the file stream 
		hr = CoCreateInstance(CLSID_SpStream, NULL, CLSCTX_ALL, IID_ISpStream, (void **)&cpStream);
		hr = cpStream->SetBaseStream(cpFileStream, format.FormatId(), format.WaveFormatExPtr());
		cpFileStream->Write(data,m_sizedata,NULL);
		cpFileStream->Close(); 
		cpFileStream->Release();
		cpStream->Close(); 
		cpStream->Release();
	//*********************************************************************************************
}

short *  AppendPhonema(short * origin, int sizeorigin, short * toappend, int sizetoappend, bool issilence)
{	
	if (issilence) memset(toappend,0,sizetoappend*sizeof(short));	
	int newsize = (sizeorigin + sizetoappend);
	short * temp = new short[newsize];
	memcpy_s(temp,sizeof(short) * newsize,origin,sizeof(short) * sizeorigin);
	memcpy_s(&temp[sizeorigin] ,sizeof(short)*newsize,toappend,sizeof(short)*sizetoappend);
	return temp;	
	// origin contains the total amount of audio data (origin + toappend)
}

short *  AppendPhonemaAtBeginning(short * origin, int sizeorigin, short * toappend, int sizetoappend, bool issilence)
{
	
	if (issilence) memset(toappend,0,sizetoappend*sizeof(short));	
	int newsize = (sizeorigin + sizetoappend);
	short * temp = new short[newsize];
	memcpy_s(temp,sizeof(short) * newsize,  toappend,sizeof(short) * sizetoappend);
	memcpy_s(&temp[sizetoappend] ,sizeof(short)*newsize,origin,sizeof(short)*sizeorigin);
	return temp;
	
	// origin contains the total amount of audio data (origin + toappend)
}

short * InsertPhonemaAtCurPosition(short * origin, int sizeorigin,int insertpos,
								   short *toinsert, int sizetoinsert,bool issilence) 
{
	if (issilence) memset(toinsert,0,sizetoinsert*sizeof(short));
	int newsize = (sizeorigin + sizetoinsert);
	short * temp = new short[newsize];
	// First we copy a part of the origin up to the insertion point
	memcpy_s(temp,newsize*sizeof(short),origin,insertpos * sizeof(short));
	// Second we add the new signal at the due position
	memcpy_s(&temp[insertpos],newsize*sizeof(short),toinsert,sizetoinsert * sizeof(short));
	// Third we add the second part of the signal which is the following
	memcpy_s(&temp[insertpos+sizetoinsert],newsize*sizeof(short),&origin[insertpos],(sizeorigin-insertpos)* sizeof(short));
	return temp;
}

void SaveSpokenTextToWav(String ^ m_filename, String ^ m_texttospeak)
{
	pin_ptr<String ^> p = &m_filename;  // We Block Garbage collection relocation to grant safety
	// to the Unmanaged pointer that represents the filename in the native env.
	LPCWSTR m_szWFileName =(LPCWSTR)(Marshal::StringToHGlobalUni(m_filename)).ToPointer();//Unicode char set
	//*********************************************************************************************
			CComPtr<ISpStreamFormat> cpOldStream;
			CComPtr<ISpStream> cpWavStream;
            CSpStreamFormat OriginalFmt;
			HRESULT  hr = pVoice->GetOutputStream( &cpOldStream );
            if (hr == S_OK)
            {
                hr = OriginalFmt.AssignFormat(cpOldStream);
            }
            else
            {
                hr = E_FAIL;
            }
            // User SAPI helper function in sphelper.h to create a wav file
            if (SUCCEEDED(hr))
            {
                hr = SPBindToFile( m_szWFileName, SPFM_CREATE_ALWAYS, &cpWavStream, &OriginalFmt.FormatId(), OriginalFmt.WaveFormatExPtr() ); 
            }
            if( SUCCEEDED( hr ) )
            {
                // Set the voice's output to the wav file instead of the speakers
                hr = pVoice->SetOutput(cpWavStream, TRUE);
            }

            if ( SUCCEEDED( hr ) )
            {
                // Do the Speak
             SpeakText(m_texttospeak);
            }
            // Set output back to original stream
            // Wait until the speak is finished if saving to a wav file so that
            // the smart pointer cpWavStream doesn't get released before its
            // finished writing to the wav.
            pVoice->WaitUntilDone( INFINITE );
            cpWavStream.Release();   
			// Reset output
            pVoice->SetOutput( cpOldStream, FALSE );
            cpOldStream.Release();

}

short * HammingWindow(short * m_originalData, int m_sizedata)
{
	for(int c =0 ; c< m_sizedata; c++)
	{
		m_originalData[c] = (short)((float)m_originalData[c] * (0.53836 - 0.46164 * Math::Cos( (2*Math::PI * c ) /(m_sizedata-1) ) ));
	}
	return m_originalData;
}



short * CosineWindow(short * m_originalData, int m_sizedata)
{
	for(int c =0 ; c< m_sizedata; c++)
	{
		m_originalData[c] = (short)( (float)m_originalData[c] *   Math::Sin(   (Math::PI*c )/ (m_sizedata-1) )          );
	}
	return m_originalData;
}

double * HammingWindow(double * m_originalData, int m_sizedata)
{
	for(int c =0 ; c< m_sizedata; c++)
	{
		m_originalData[c] = m_originalData[c] * (0.53836 - 0.46164 * Math::Cos( (2*Math::PI * c ) /(m_sizedata-1) ) );
	}
	return m_originalData;
}


double * CosineWindow(double * m_originalData, int m_sizedata)
{
	for(int c =0 ; c< m_sizedata; c++)
	{
		m_originalData[c] = ( m_originalData[c] *   Math::Sin(   (Math::PI*c )/ (m_sizedata-1) )          );
	}
	return m_originalData;
}


double * BlackmanWindow(double * m_originalData, int m_sizedata)
{
	double a0,a1,a2;
	a0=(1.0F- 0.16)/2;
	a1=0.5;
	a2=0.16/2;
	for(int c =0 ; c< m_sizedata; c++)
	{
		m_originalData[c] =   m_originalData[c] *   (a0 - a1*Math::Cos((2*Math::PI*c)/m_sizedata-1) + a2*Math::Cos((4*Math::PI*c)/m_sizedata-1))       ; 
	}
	return m_originalData;
}




short * BlackmanWindow(short * m_originalData, int m_sizedata)
{
	float a0,a1,a2;
	a0=(1- 0.16F)/2;
	a1=0.5F;
	a2=0.16F/2;
	for(int c =0 ; c< m_sizedata; c++)
	{
		m_originalData[c] = (short)(  (float)m_originalData[c] *   (a0 - a1*Math::Cos((2*Math::PI*c)/m_sizedata-1) + a2*Math::Cos((4*Math::PI*c)/m_sizedata-1))         ); 
	}
	return m_originalData;
}

double ZeroCrossingRate(short * m_signal,int m_sizeinsamples)
{
// The zero crossing rate can have as its maximum value =0.5 in case of a signal that contains all
// frequencies of m_samplerate / 2

double count =0;
for(int c=0; c < m_sizeinsamples; c++)
{
	if ( (m_signal[c] >=0)  && (m_signal[c-1] < 0 )  ) count+=1;
	if ( (m_signal[c] < 0 ) && (m_signal[c-1] >= 0 )) count+=1;
}
double zeroc= count/m_sizeinsamples;
return zeroc;
}




short * PrheEmphasis(short * m_originaldata, int m_sizedata)
{
//prheemphasis coefficient = 15/16
  for (int x=1 ; x < m_sizedata; x++)
   {
	 m_originaldata[x] = m_originaldata[x]  -  (short)((0.95)* (float) m_originaldata[x-1]);
   }
  return m_originaldata;
}


bool NormalizeAudioinVoiceFile(String ^ m_voicefile,float &m_coeffaverage);

short * NormalizeAudio(short * m_originaldata, int m_sizedata, float &coeff)
{
// first of all we must find the maximum value of our signal
int maxamplitude=0;
int minamplitude =0;
for(int c = 0; c< m_sizedata;++c)
 {
  if (m_originaldata[c] > maxamplitude) maxamplitude = m_originaldata[c];
  if (m_originaldata[c] < minamplitude) minamplitude = m_originaldata[c];	
 }		
int maxabsoluteamplitude = maxamplitude > System::Math::Abs(minamplitude) ? maxamplitude : System::Math::Abs(minamplitude);	
// in order not to create any distortion we must calculate the coefficient in this way
coeff = ((float)MAXSHORT * (float)0.98)/(float)maxabsoluteamplitude; // usually 98% (-0.3 dB) or 100% (0 dB)[
// Now we can apply it to every sample of our audio data
for (int x=1 ; x < m_sizedata; x++)
{
 m_originaldata[x]=(short)((float)m_originaldata[x] * coeff);
}
  return m_originaldata;
}

short * CreateAudioBackup( short * m_audiodata, int size)
{
	short * m_backup = new short[size];
	memcpy_s(m_backup,sizeof(short)*size,m_audiodata,sizeof(short)*size);
	return m_backup;
}
short * GetAudioBackup(short * m_audiodata,int size)
{
	short * ret = new short[size];
	memcpy_s(ret,sizeof(short)*size,m_audiodata,sizeof(short)*size);
	return ret;
}

short * AmplyfyPhonemeVolume(short * m_audiodata, int size)
{
	// We sample 10% every shot
	for(int y = 0 ; y < size; ++y) m_audiodata[y] = (short)((double) m_audiodata[y]* 1.1);
	return m_audiodata;
}

short * LessenPhonemeVolume(short * m_audiodata, int size)
{
	// We sample 10% every shot
	for(int y = 0 ; y < size; ++y) m_audiodata[y] = (short)((double) m_audiodata[y]* 0.9);
	return m_audiodata;
}

short * TriangularIncrease(short * m_audiodata, int size)
{
	float step = (float)1/(float)size;
	float step1=0;
	for(int y = 0 ; y < size; ++y)
	{
		m_audiodata[y] = (short)((float) m_audiodata[y]* step1);
		step1+=step;
	}
	return m_audiodata;
}

short * TriangularFading(short * m_audiodata, int size)
{
	float step = (float)1/(float)size;
	float step1=0;
	for(int y = 0 ; y < size; ++y)
	{
		m_audiodata[y] = (short)((float) m_audiodata[y]* (1 - step1) );
		step1+=step;
	}
	return m_audiodata;
}

double  ConfrontZeroCrossings(short * m_audio_data1, int m_sizeaudio1, short * m_audio_data2, int m_sizeaudio2,double &dist )
{
// Band pass filtering
//short * fil1;
//short * fil2;
//fil1= BandPass(m_audio_data1,m_sizeaudio1,100,11025,1000,2000,1);
//fil2= BandPass(m_audio_data2,m_sizeaudio2,100,11025,1000,2000,1);
//memcpy_s(m_audio_data1,m_sizeaudio1 * sizeof(short),fil1,m_sizeaudio1 * sizeof(short));
//delete[] fil1;
//memcpy_s(m_audio_data2,m_sizeaudio2 * sizeof(short),fil2,m_sizeaudio2 * sizeof(short));
//delete[] fil2;
// End band pass filtering

short ** chunks; 
int numchunks = 20;
int chunksize1=m_sizeaudio1/numchunks;
double * cr1  = new double[numchunks]; // The array which will contain the information of zero crossing for the 20 chunks
chunks = new short* [numchunks];
for (int h = 0; h < numchunks;h++)
{
	chunks[h]= new short[chunksize1];
	memset((char*)chunks[h],0,chunksize1*sizeof(short)); // we zero memory of the chunk
    {
		memcpy_s((char*)chunks[h],chunksize1* sizeof(short), (char*)&m_audio_data1[h*chunksize1],chunksize1* sizeof(short));
	}
	cr1[h]=ZeroCrossingRate(chunks[h],chunksize1);
	delete [] chunks[h];
}
delete[] chunks; chunks =NULL;
int chunksize2=m_sizeaudio2/numchunks;
double * cr2  = new double[numchunks]; // The array which will contain the information of zero crossing for the 20 chunks
chunks = new short* [numchunks];
for (int h = 0; h < numchunks;h++)
{
	chunks[h]= new short[chunksize2];
	memset((char*)chunks[h],0,chunksize2*sizeof(short)); // we zero memory of the chunk
    {
		memcpy_s((char*)chunks[h],chunksize2* sizeof(short), (char*)&m_audio_data2[h*chunksize2],chunksize2* sizeof(short));
	}
	cr2[h]=ZeroCrossingRate(chunks[h],chunksize2);
	delete [] chunks[h];
}
delete[] chunks; chunks =NULL;
// now we have cr1 and cr2;
// we can now calculate the mean of the differences between the two series;
double distance =0;
double ret = AutoCorrelation(cr1,cr2,numchunks,distance);
return ret;
}



short * ExtractPortionOfAudioData(short *m_audio_data, int m_fragstart, int m_fragend, int m_audiosize, int &m_sizefrag)
{
	m_sizefrag = abs(m_fragend-m_fragstart);
	short * temp = new short[m_sizefrag];
	memcpy_s(temp,m_sizefrag* sizeof(short),&m_audio_data[m_fragstart],m_sizefrag* sizeof(short));
	return temp;
}


short * CutAwayAudioSelection(short *m_audio_data, int m_fragstart, int m_fragend, int m_audiosize, int &m_newsizes)
{
memcpy_s(&m_audio_data[m_fragstart] , m_audiosize * sizeof(short),
&m_audio_data[m_fragend],(m_audiosize-m_fragend)* sizeof(short));
int m_newsize = m_audiosize-(m_fragend-m_fragstart);
short * temp = new short[m_newsize];
memcpy_s(temp,m_newsize * sizeof(short), m_audio_data, m_newsize * sizeof(short));
m_newsizes = m_newsize;
return temp;
}

//*****************************VOICE MANAGEMENT*******************************************************
bool CreatePreviewTextForVoice(String ^ m_default_phrase, String ^ m_language, bool m_append_voice_name)
{
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_default_phraseU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_default_phrase)).ToPointer();//Unicode char set
//***************************************************************************************************
// to the Unmanaged pointer that represents the filename in the native env.
m_language=m_language->ToUpper();
LPCWSTR m_languageU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_language)).ToPointer();//Unicode char set
wchar_t M_langID_ITA[] = L"410";
wchar_t M_langID_ENG[] = L"409";
wchar_t M_langID_SPA[] = L"40A";
wchar_t M_langID_FRE[] = L"40C";
int lan =0;
if(wcscmp(m_languageU,L"ITALIAN")==0) lan =0;
if(wcscmp(m_languageU,L"ENGLISH")==0) lan=1;
if(wcscmp(m_languageU,L"SPANISH")==0) lan =2;
if(wcscmp(m_languageU,L"FRENCH")==0)  lan=3;
//***************************************************************************************************
// The following fragment creates the preview Key in the registry and sets a defaul phrase
wchar_t defaultphrase[2000] = {0};
wcscpy_s(defaultphrase,2000,m_default_phraseU);
if(m_append_voice_name) wcscat_s(defaultphrase,2000,L" %s"); // The %s adds the voice name
  // wchar_t * kname = L"Preview";   
   HKEY hk; 
   DWORD dwDisp; 
   WCHAR  szBuf[MAX_PATH];
   wcscpy_s(szBuf,MAX_PATH,L"SOFTWARE\\MICROSOFT\\Speech\\Voices\\Preview");
   WCHAR szBuf1[MAX_PATH];
   wcscpy_s(szBuf1,MAX_PATH,L"SOFTWARE\\MICROSOFT\\Speech\\Voices\\Preview\\");

  // size_t cchSize = MAX_PATH;  
   //HRESULT hr = StringCchPrintf(szBuf, cchSize,L"SOFTWARE\\MICROSOFT\\Speech\\Voices\\%s",kname); 
   if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szBuf,0, NULL, REG_OPTION_NON_VOLATILE,KEY_WRITE, NULL, &hk, &dwDisp)) 
   {
	return false;
   }
   
wchar_t* templan=nullptr;
switch(lan)
	   {
	   case 0:
		   templan=M_langID_ITA;
		   break;
	   case 1:
			templan=M_langID_ENG;
			break;
	   case 2:
			templan=M_langID_SPA;
			break;
	   case 3:
			templan=M_langID_FRE;
			break;
	   }
// Here we delete the subkey if it exists because we only want a preview for every language.
//hr=StringCchPrintf(szBuf1, cchSize,L"SOFTWARE\\MICROSOFT\\Speech\\Voices\\Preview\\%s",templan); 
wcscat_s(szBuf1,MAX_PATH,templan);
RegDeleteKeyEx(hk,szBuf1,KEY_WOW64_32KEY|KEY_WOW64_64KEY,0);   
if (RegSetValueEx(hk, templan ,0, REG_SZ,(LPBYTE) defaultphrase,(DWORD) (lstrlen(defaultphrase)+1)*sizeof(TCHAR))) 
{
   RegCloseKey(hk); 
   return false;
}
return true;

}

 CLSID CreateGuiD()
{ 
	CLSID pguid; 
	HRESULT hr = CoCreateGuid(&pguid); 
	return pguid;
}

 String ^ GetEngineWORDSCLSID()
 {
  CLSID CLSID_TTSEngineWORDS = {0xA832755E,0x9C2A,0x40b4,{0x89,0xB2,0x3A,0x92,0xEE,0x70,0x58,0x52}};
  return ConvertCLSIDtoString(CLSID_TTSEngineWORDS);
 
 }

String ^ GetEngineCONCATCLSID()
 {
  CLSID CLSID_TTSEngineCONCAT = {0xA832755E,0x9C2A,0x40b4,{0x89,0xB2,0x3A,0x92,0xEE,0x70,0x58,0x51}};
  return ConvertCLSIDtoString(CLSID_TTSEngineCONCAT);
 
 }
String ^ CreateGuidString()
{
	CLSID pguid; 
	HRESULT hr = CoCreateGuid(&pguid); 
	wchar_t * pguidString;
	StringFromCLSID(pguid,&pguidString);
	return gcnew String(pguidString);
}


String ^ ConvertCLSIDtoString(CLSID pguid)
{

	wchar_t * pguidString;
	StringFromCLSID(pguid,&pguidString);
	return gcnew String(pguidString);

}


CLSID ConvertStringtoCLSID(String ^ strCLSID)
{
	CLSID pguid; 
	pin_ptr<String ^> p = &strCLSID;  // We Block Garbage collection relocation to grant safety
	// to the Unmanaged pointer that represents the filename in the native env.
	LPCWSTR strCLSIDU =(LPCWSTR)(Marshal::StringToHGlobalUni(strCLSID)).ToPointer();//Unicode char set
	//***************************************************************************************************
	CLSIDFromString((wchar_t *)strCLSIDU,&pguid);
	return pguid;
}


String ^ CreateBlankVoice(void);


// The following function create a new voice token in the registry and return the CLSID to be used in
// The correspondent engine. The function produces a new voice entry which is visible in the Contro Panel
// under Text-to-speech shortcut
bool CreateNewVoiceWORDS(String ^ m_voicename,String ^ m_voice_description, String ^ language,
						  String^ m_CLSID, String ^ m_pathvoicefile, String ^ gender, String ^ age);		

bool CreateNewVoiceCONCAT(String ^ m_voicename,String ^ m_voice_description, String ^ language,
						  String^ m_CLSID, String ^ m_pathvoicewords,String ^ m_pathvoicephones,  String ^ gender, String ^ age);	

void CreateLexiconFromDB(String^ m_dbpath, String ^ m_archivepath); // Creates the File archive ttss from an access database


void InitializeComboVoices(IntPtr handle) // Not used here so far
{
	HWND hn = (HWND)handle.ToPointer();
	HRESULT	 hr = SpInitTokenComboBox(hn, SPCAT_VOICES );
}
// This is a very crucial function and must be used every time a word update or append has been done
// in order to reload the Voice with the updated words.
void RefreshVoiceAfterWordUpdates()
{
	if(pVoice != nullptr) 
{
	pVoice.Release();
	pVoice=nullptr;
}
	HRESULT hr = pVoice.CoCreateInstance( CLSID_SpVoice );
	pVoice->SetVoice(pCurVoiceToken);	
}

void DetachVoice()
{
pVoice.Release();
pVoice=nullptr;
}



ArrayList ^ GetVoicesList()
{
	ArrayList ^ m_voice_names_container = gcnew ArrayList();
	CComPtr<IEnumSpObjectTokens>  cpIEnum;
	HRESULT hr = SpEnumTokens(SPCAT_VOICES,NULL,NULL, &cpIEnum);
	ISpObjectToken * pToken;
	WCHAR * value ;
	while (cpIEnum->Next(1, &pToken, NULL) == S_OK)
	{
	// At this point, all we know is that pToken is a pointer to a Voice token. 
		hr = pToken->GetStringValue(L"",&value);
		
		m_voice_names_container->Add(gcnew String(value));
	}
	m_voice_names_container->Sort();

    cpIEnum.Release(); // always remember to get rid off of COM resources that are out of scope
	return m_voice_names_container;
}

void SetCurrentVoice(String ^ m_voicename)
{
	
pin_ptr<String ^> p = &m_voicename;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_voicenameU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_voicename)).ToPointer();//Unicode char set
//***************************************************************************************************
	CComPtr<IEnumSpObjectTokens>  cpIEnum;
	HRESULT hr = SpEnumTokens(SPCAT_VOICES,NULL,NULL, &cpIEnum);
	ISpObjectToken * pToken;

	if (pVoice != nullptr){
		
		pVoice.Release();
		pVoice=nullptr;
		HRESULT hr = pVoice.CoCreateInstance( CLSID_SpVoice );
		
		}
	if(pCurVoiceToken != nullptr) pCurVoiceToken->Release();
	WCHAR * value ;
	while (cpIEnum->Next(1, &pToken, NULL) == S_OK)
	{
	// At this point, all we know is that pToken is a pointer to a Voice token. 
		hr = pToken->GetStringValue(L"",&value);
		if(wcscmp(value,m_voicenameU)==0)
		{
			pCurVoiceToken = pToken; 
			break;
		}
	}
	pVoice->SetVoice(pCurVoiceToken);	
	cpIEnum.Release(); // always remember to get rid off of COM resources that are out of scope
	
}


void SetVoiceSpeed(int speed)
{

pVoice->SetRate(speed);

}



void SpeakText(String ^ m_text_fragment)
{

pin_ptr<String ^> p = &m_text_fragment;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_text_fragmentU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_text_fragment)).ToPointer();//Unicode char set
//***************************************************************************************************
HRESULT hr  = pVoice->Speak(m_text_fragmentU, SPF_ASYNC |SPF_IS_NOT_XML, 0 );
}

void StopSpeak();
inline void UpdateWordAudioData(String ^ m_voicefile, String ^ m_wordtoupdate,short * m_newaudioinfo,int m_sizenewaudioinfo);

inline void UpdatePhoneAudioData(String ^ m_phonefile, int m_phoneindex ,short * m_newaudioinfo,int m_sizenewaudioinfo)
{
long m_lenfile =GetVoiceFileLength(m_phonefile);
pin_ptr<String ^> p = &m_phonefile;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_phonefileU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_phonefile)).ToPointer();//Unicode char set
//***************************************************************************************************
//if(m_phonefile->EndsWith(".phe",true,nullptr)) // we are dealing with the english phone set
//{
// first of all we must open the file for reading and writing
// we must copy the  m_sizenewaudioinfo  bytes of   m_newaudioinfo at the m_audio_address location of our voicefile
		HANDLE HN = CreateFile(m_phonefileU,GENERIC_ALL ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);	
		HANDLE HP = CreateFileMapping(HN,NULL,PAGE_READWRITE,0,0,NULL);		
		HANDLE HX = MapViewOfFile(HP,FILE_MAP_ALL_ACCESS,0,0,0);
		byte * pt = (byte *)HX; // We cast to byte type
		int * len = (int*)pt;
		int * ar = NULL;
		pt+=sizeof(int);
		ar = (int*) pt;
		// ar is an array of integer which spawns from 0 and len-1 (in the english case n = 47 , that is the number of phonemes)
		int oldsize = ar[m_phoneindex]; // we memorize the old size for late usage REMEMBER !! THE SIZE IS IN BYTES
		// we update with the ne size
	    ar[m_phoneindex]=m_sizenewaudioinfo * sizeof(short);
		// we move the pointer at the beginning of audio data
		pt+=( *len * sizeof(int));
		// we have now three cases:
		// 1) the oldsize is equal to the new size;
		if(ar[m_phoneindex] == oldsize)
		{
		int offset=0;for (int c =0; c < m_phoneindex; c++) offset+= ar[c];pt+=offset;
		memcpy_s(pt,ar[m_phoneindex],m_newaudioinfo,ar[m_phoneindex]);
		
		UnmapViewOfFile(HX); // This invalidate the handle so there's no need to close it
		CloseHandle(HP);
		CloseHandle(HN);
		return;
		
		}
		// 2) the new size is smaller than the old size
		if(ar[m_phoneindex] < oldsize)
		{
		int offset=0;for (int c =0; c < m_phoneindex; c++) offset+= ar[c];pt+=offset;
		int remain = m_lenfile - offset - (*len*sizeof(int)) - sizeof(int) - oldsize;// The remain of the file from end
		int delta = oldsize - ar[m_phoneindex]; // in this case delta is positive
		int newsize = m_lenfile - delta; // the new file length		
		memcpy_s(pt+ar[m_phoneindex],remain, pt + oldsize,remain);
		memcpy_s(pt,ar[m_phoneindex],m_newaudioinfo,ar[m_phoneindex]);
		// now we can truncate the file to the new size
		UnmapViewOfFile(HX); // This invalidate the handle so there's no need to close it
		CloseHandle(HP);	// before changing the end of file we need to unmap	
		DWORD dwPos = SetFilePointer(HN, newsize, NULL, FILE_BEGIN);		
		SetEndOfFile(HN);
		dwPos = SetFilePointer(HN, 0, NULL, FILE_BEGIN);
		CloseHandle(HN);
		return;
		}
		// 3) the new size is greater then the old
		if(ar[m_phoneindex] > oldsize)
		{
			int offset=0;for (int c =0; c < m_phoneindex; c++) offset+= ar[c];pt+=offset;
			int delta = ar[m_phoneindex] - oldsize; // the difference in bytes to inflate the voicefile 
			int remain = m_lenfile - offset - (*len*sizeof(int)) - sizeof(int)-oldsize;// The remain of the file from end
			byte * half = new byte[delta];
			memset(half,0,delta);
			DWORD dwPos = SetFilePointer(HN, 0, NULL, FILE_END);
			DWORD m_written=0;// the reference of bytes written
			WriteFile(HN,half,delta, &m_written,  NULL); 
			delete [] half; // We have just inflated the voice file size
			dwPos = SetFilePointer(HN, 0, NULL, FILE_BEGIN);
			// As we have increased the file size, the map becomes invalid and the file must be remapped
			UnmapViewOfFile(HX); 
		    CloseHandle(HP);
		    HP = CreateFileMapping(HN,NULL,PAGE_READWRITE,0,0,NULL); // We map with the new size		
		    HX = MapViewOfFile(HP,FILE_MAP_ALL_ACCESS,0,0,0);
			pt = (byte *)HX; // We cast to byte type
			pt+=(offset + (*len*sizeof(int)) + sizeof(int) ) ;	// I place the pointer where it must be
			// at the start of the audio phonemes that must be updated
			memcpy_s(pt+oldsize+delta,remain,pt+oldsize,remain);
			memcpy_s(pt,ar[m_phoneindex],m_newaudioinfo,ar[m_phoneindex]);
		
		UnmapViewOfFile(HX); // This invalidate the handle so there's no need to close it
		CloseHandle(HP);
		CloseHandle(HN);
		return;
		}
		

}

long GetVoiceFileLength(String ^ m_voicefile);
short * RetrieveAudioDataFromWord(String ^ m_voicefile, String ^ m_wordm, int &m_sizesample, int &m_audio_address);
array <String^> ^ ListWordsInVoice(String ^ m_voicefile,int &m_voiceversion);
void AppendWordToVoiceUsingCurrentAudio(String ^ WordtoAppend,String ^ m_voicefile, short * m_audiodata, int m_sizeaudiodata);
bool SmoothEndsinVoiceFile(String ^ m_voicefile,int m_samplestosmooth);
void AppendWordToVoice(String ^ WordtoAppend,String ^ m_voicefile, String ^ m_wordaudiofilename);
bool CreateVoiceFile(String ^ m_filename, String ^ m_fileblank, int m_voice_quality)
{
// memory defrag ************************************************************************************
ULONG  HeapFragValue = 2;
HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
// **************************************************************************************************
HRESULT hr;
static const DWORD dwVersion = (DWORD) m_voice_quality;
static const DWORD dwWordcount = {1}; // We automatically insert the first word which is the Unknown word, 
//that is when we miss a word we play a simple sound corresponding to Unknown (first of the list)
pin_ptr<String ^> p = &m_filename;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_filenameU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_filename)).ToPointer();//Unicode char set
pin_ptr<String ^> pp = &m_fileblank;  // We Block Garbage collection relocation to grant safety
// to the Unmanaged pointer that represents the filename in the native env.
LPCWSTR m_fileblankU =(LPCWSTR)(Marshal::StringToHGlobalUni(m_fileblank)).ToPointer();//Unicode char set
//*********************************************************************************************
FILE  *hVoiceFile = NULL; 
// We open the voice file for binary writing creating a new file or truncating to 0 if exists
if (fopen_s(&hVoiceFile, CW2A(m_filenameU), "wb") != 0 )
 {
   hVoiceFile = NULL;
   hr = E_FAIL;
   return false;
  }
// ****************** SUCCESS IN CREATION OF VOICE FILE **************************
//--- Write file version and initial word count =1
if( !fwrite( &dwVersion, sizeof(DWORD), 1, hVoiceFile ) )
  {
    hr = E_FAIL;
	return false;
  }
// Write the initial number of words present in this voicefile. They are 0 at the beginning
if( !fwrite( &dwWordcount, sizeof(DWORD), 1, hVoiceFile ) )
  {
    hr = E_FAIL;
	return false;
  }
// Now we write the Unknown word and its audio data
WCHAR * m_firstword =L"unknown";
size_t ulTextLen=wcslen(m_firstword);
ulTextLen = (ulTextLen+1) * sizeof(WCHAR); // The amount of bytes.
if( fwrite( &ulTextLen, sizeof(ulTextLen), 1, hVoiceFile ) &&
    fwrite( m_firstword, ulTextLen, 1, hVoiceFile ) )
// we can append the audio for the unknown word
{
ULONG m_sizesample;
BYTE* Buff=NULL;
ISpStream* pStream;
hr = SPBindToFile(m_fileblankU, SPFM_OPEN_READONLY, &pStream );
CSpStreamFormat Fmt;
Fmt.AssignFormat(pStream);
if(  ((Fmt.ComputeFormatEnum() == SPSF_11kHz16BitMono ) &&  (m_voice_quality ==11025))
   ||((Fmt.ComputeFormatEnum() == SPSF_16kHz16BitMono)  && ( m_voice_quality ==16000))
   ||((Fmt.ComputeFormatEnum() == SPSF_22kHz16BitMono)  &&  (m_voice_quality ==22050)) 
   ||((Fmt.ComputeFormatEnum() == SPSF_44kHz16BitMono)  &&  (m_voice_quality ==44100))  )
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
   {	m_sizesample=0;
		fclose( hVoiceFile );
		DeleteFile(m_filenameU);
		return false;
	}
// First we write the number of bytes
fwrite(&m_sizesample, sizeof(m_sizesample), 1, hVoiceFile ) ;
// Now we can write the audio content ahead of our voice file
fwrite( Buff, 1, m_sizesample, hVoiceFile ); 
// we free  
delete [] Buff;
}
fclose( hVoiceFile );
return true;

}

//**************************************************************************************

void UseGDI(IntPtr h,int px,int px1,int py,int p1x, int p1y)
{
//HDC  dc = (HDC) (h.ToPointer()) ;	
	//SetGraphicsMode( (HDC)(h.ToPointer())  ,GM_ADVANCED);
	int mode = SetROP2((HDC) (h.ToPointer()),R2_XORPEN);
	HPEN pen = CreatePen(PS_SOLID,1,RGB(255,255,255));
	HBRUSH brush = CreateSolidBrush(RGB(0,120,120));
	HGDIOBJ   brushold = SelectObject((HDC) (h.ToPointer()),brush);
	HGDIOBJ   penold = SelectObject((HDC) (h.ToPointer()),pen);	
	Rectangle((HDC) (h.ToPointer()),px,py+3,p1x,p1y-3);
	// The following deals with the fact that in a case we must redraw a selection
    if(px1 != -1) Rectangle((HDC) (h.ToPointer()),px,py+3,px1,p1y-3);
	SelectObject((HDC) (h.ToPointer()),penold);
	SelectObject((HDC) (h.ToPointer()),brushold);
	
}


void SetCap(IntPtr hwnd)
{
	SetCapture((HWND)hwnd.ToPointer());
}

short * ShrinkSignalToSel( short * allsignal, int startfrag, int endfrag, int &newsize)
{
	int m_shrinkedsize = (endfrag-startfrag) + 1;
	short * m_temp = new short[m_shrinkedsize];
	memcpy_s(m_temp,sizeof(short)*m_shrinkedsize,&allsignal[startfrag],sizeof(short)*m_shrinkedsize);
	//delete [] allsignal;
	//allsignal = m_temp;
	newsize=m_shrinkedsize;
	return m_temp;
}


void CreateAppLexicon()
{}

void Playphoneme(BYTE * paudio, int sizeaudio, int m_samplerate)
{

//// memory defrag ************************************************************************************
//ULONG  HeapFragValue = 2;
//HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation, &HeapFragValue,sizeof(HeapFragValue));
//// **************************************************************************************************
is_playing_phonema=true;
//static HWAVEOUT hw;	
	static double hh= 2 * m_samplerate,ff=1000; // parameter necessary to compute the time to sleep.
	double gg; // parameter necessary to compute the time to sleep.
	DWORD TimeToPlay;// parameter necessary to compute the time to sleep.
	gg = ((double)sizeaudio / hh ) * ff; // Time in milliseconds of the wave
	TimeToPlay  = (long)gg; // now we have the sleep time
	int bufSize=sizeaudio;
	static WAVEFORMATEX wf ;
	wf.nSamplesPerSec=m_samplerate;
	wf.wFormatTag=WAVE_FORMAT_PCM;
	wf.wBitsPerSample=16;
	wf.nChannels=1;
	wf.nBlockAlign=(wf.nChannels * wf.wBitsPerSample)/8;
	wf.nAvgBytesPerSec=(wf.nSamplesPerSec * wf.nBlockAlign);
	headwav0.dwFlags=WHDR_DONE;headwav0.dwUser=1;	
	waveOutOpen(&hw,WAVE_MAPPER,&wf,NULL,0,NULL);	
	headwav0.dwBufferLength=(DWORD)bufSize;
	headwav0.lpData=(char*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, bufSize );
	memcpy_s(headwav0.lpData,bufSize,paudio,sizeaudio);
	waveOutPrepareHeader(hw,&headwav0,sizeof(WAVEHDR));
	waveOutWrite(hw,&headwav0,sizeof(WAVEHDR));
	Sleep(TimeToPlay);// we use the sleep time now to play all and wait some more	
	if(!is_playing_phonema) return;// it means that we have pressed the stop button so we must exit
	waveOutUnprepareHeader(hw,&headwav0,sizeof(WAVEHDR));
	//waveOutReset(hw); it seems not good
	waveOutClose(hw);	
	HeapFree(GetProcessHeap(), 0, headwav0.lpData);	
	is_playing_phonema=false;
}
void StopPlayPhoneme()
{
	if(is_playing_phonema)
	{
		waveOutUnprepareHeader(hw,&headwav0,sizeof(WAVEHDR));
		HeapFree(GetProcessHeap(), 0, headwav0.lpData);
		waveOutReset(hw);
		waveOutClose(hw);		
		is_playing_phonema=false;
	}
}

void ComplexFFT(double *vector, unsigned int sample_rate, int sign);
// *********************************************  U T I L I T I E S ******************************************
int GetAllocationGranularity();



//***************************************AUDIO RECORD**********************************************************		

void SetMasterVolume(int m_ratio) // the volume is a number between 0 and 65535
{
	MMRESULT result;
	HMIXER hMixer;
	result = mixerOpen(&hMixer, MIXER_OBJECTF_MIXER, 0, 0, 0);	
	MIXERLINE ml = {0};
	ml.cbStruct = sizeof(MIXERLINE);
	ml.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	result = mixerGetLineInfo((HMIXEROBJ) hMixer,&ml, MIXER_GETLINEINFOF_COMPONENTTYPE);
	if(result !=0) // we try to get the line info for headphones
	{
		ml.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_HEADPHONES;
		result = mixerGetLineInfo((HMIXEROBJ) hMixer,&ml, MIXER_GETLINEINFOF_COMPONENTTYPE);
	}
	MIXERCONTROL mc = {0};
	MIXERLINECONTROLS mlc = {0};	
	mlc.cbStruct = sizeof(MIXERLINECONTROLS);
	mlc.dwLineID = ml.dwLineID;
	mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mlc.cControls = 1;
	mlc.pamxctrl = &mc;
	mlc.cbmxctrl = sizeof(MIXERCONTROL);
	result = mixerGetLineControls((HMIXEROBJ) hMixer,&mlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);
	MIXERCONTROLDETAILS_UNSIGNED mcdu = {0};
	MIXERCONTROLDETAILS mcd = {0};
	mcdu.dwValue = m_ratio; // the volume is a number between 0 and 65535
	mcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
	mcd.hwndOwner = 0;
	mcd.dwControlID = mc.dwControlID;
	mcd.paDetails = &mcdu;
	mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
	mcd.cChannels = 1;
	result = mixerSetControlDetails((HMIXEROBJ) hMixer,&mcd, MIXER_SETCONTROLDETAILSF_VALUE);
}

void ShowTrainingDialog(IntPtr h)
{
	// Declare local identifiers:
	HRESULT                    hr = S_OK;
	CComPtr<ISpRecognizer>     cpRecognizer;
	hr = cpRecognizer.CoCreateInstance( CLSID_SpSharedRecognizer );
	// Display microphone training UI for the current recognizer.
	hr = cpRecognizer->DisplayUI((HWND)h.ToPointer(), L"Microphone training", SPDUI_MicTraining, NULL, NULL);
	cpRecognizer.Release();
}

void StartRecord(int m_samplerate,int m_threshold,IntPtr I, IntPtr H,System::Drawing::Graphics ^ g,
				 System::Windows::Forms::PictureBox ^ pic,System::Windows::Forms::PictureBox ^ picFFT,
				 System::Windows::Forms::PictureBox ^ picSPECTRUM,bool m_showrealtime)
{
	m_min_recthreshold=m_threshold;	
	m_bufferSize=MM_BUF_SIZE;
	hAudiofile = CreateFile(L"c:\\temprecord.pcm",GENERIC_ALL,FILE_SHARE_WRITE|FILE_SHARE_READ,0,
						  CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,0); 
	memset(&wfx,0x00,sizeof(wfx));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.wBitsPerSample = 16;
	wfx.cbSize = 0;
	wfx.nSamplesPerSec = m_samplerate;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec *(wfx.wBitsPerSample/8);
	wfx.nBlockAlign = (wfx.wBitsPerSample*wfx.nChannels)/8;
	if (m_bufferSize % wfx.nBlockAlign != 0)
	{
        m_bufferSize += wfx.nBlockAlign - (m_bufferSize % wfx.nBlockAlign);
	}
	// Buffer allocation
	wavBuffer = new char*[NUM_BUFFERS];
	if (int res = waveInOpen(&hwi, WAVE_MAPPER, &wfx, (DWORD_PTR)waveInCallback, 0, CALLBACK_FUNCTION)) return	;
	for(int c =0; c < NUM_BUFFERS; c++)
	{
		wavBuffer[c]=new char[m_bufferSize];
		memset(wavBuffer[c],0,m_bufferSize);
		whdr[c].lpData          = wavBuffer[c];                   
		whdr[c].dwBufferLength  = m_bufferSize;           
		whdr[c].dwBytesRecorded = 0;
		whdr[c].dwUser          = 0;
		whdr[c].dwFlags         = 0;
		whdr[c].dwLoops         = 1;
		whdr[c].lpNext          = 0;
		whdr[c].reserved        = 0;
		if (waveInPrepareHeader(hwi, &whdr[c], sizeof(WAVEHDR))) return	;
		if (waveInAddBuffer(hwi, &whdr[c], sizeof(WAVEHDR))) return;
	}	
	recording  = 1;	
	int res = waveInStart(hwi);	
	// Here we want to send to the thread procedure not only the HDC but also the HWND of the window
	// So we pack them using the appiosite class PackDCHWND
	if(m_showrealtime)
	{
	PackDCHWND ^ msender = gcnew PackDCHWND();
	msender->m_hdc=I;// The device context Here is not set because we don't use the old GDI approach
	msender->m_hwnd=H;// The window handle
	msender->m_threshold=m_threshold;
	msender->grap=g;
	msender->m_pbox=pic;
	msender->m_samplerate=m_samplerate;	
	PackDCHWND ^ msenderFFT = gcnew PackDCHWND();
	msenderFFT->m_hdc=I;// The device context Here is not set because we don't use the old GDI approach
	msenderFFT->m_hwnd=H;// The window handle
	msenderFFT->m_threshold=m_threshold;
	msenderFFT->grap=g;
	msenderFFT->m_pbox=picFFT; // We change only this because the fft output is in another window
	msenderFFT->m_samplerate=m_samplerate;
	PackDCHWND ^ msenderSPECTRO = gcnew PackDCHWND();
	msenderSPECTRO->m_hdc=I;// The device context Here is not set because we don't use the old GDI approach
	msenderSPECTRO->m_hwnd=H;// The window handle
	msenderSPECTRO->m_threshold=m_threshold;
	msenderSPECTRO->grap=g;
	msenderSPECTRO->m_pbox=picSPECTRUM; // We change only this because the fft output is in another window
	msenderSPECTRO->m_samplerate=m_samplerate;
	m_thread_rec = gcnew Thread(gcnew ParameterizedThreadStart(this,  &LexHelper::NotifyGDIPLUSBITMAP));
	m_thread_recFFT=gcnew Thread(gcnew ParameterizedThreadStart(this,  &LexHelper::NotifyFFT));
	m_thread_recSPECTRUM =gcnew Thread(gcnew ParameterizedThreadStart(this,  &LexHelper::NotifySPECTROGRAM));
	m_thread_recFFT->Start(msenderFFT);
	m_thread_rec->Start(msender);
	m_thread_recSPECTRUM->Start(msenderSPECTRO);
	}

}

Void NotifyFFT(Object ^ al)
{
System::Drawing::Pen ^ PenFFT = gcnew System::Drawing::Pen(System::Drawing::Color::FromArgb(255,12,200,123),0);// The color of rejected sample
System::Drawing::Pen ^ PenFFT1 = gcnew System::Drawing::Pen(System::Drawing::Color::FromArgb(255,12,123,180),0);// The color of rejected sample
System::Drawing::Drawing2D::Matrix ^ MatFft;
PackDCHWND ^ got = (PackDCHWND^)al;		
int fftsize =0;
float coeff=0;
int m_samplerate=got->m_samplerate;
double * m_vector=NULL;
while(recording==1)
{
System::Drawing::Bitmap ^ canvas = gcnew System::Drawing::Bitmap(got->m_pbox->Width, got->m_pbox->Height);
System::Drawing::Graphics ^ offScreenDC = System::Drawing::Graphics::FromImage(canvas);
int smplrec = whdr[0].dwBytesRecorded == 0 ?  (whdr[1].dwBytesRecorded)/sizeof(short) :(whdr[0].dwBytesRecorded)/sizeof(short) ;
if(smplrec < 2000) continue;	
short * tempaudio = whdr[0].dwBytesRecorded == 0 ? (short*) whdr[1].lpData :(short*) whdr[0].lpData;	

static double hh= 2 * got->m_samplerate,ff=1000; // parameter necessary to compute the time to sleep.
double gg; // parameter necessary to compute the time to sleep.
int TimeToPlay;// parameter necessary to compute the time to sleep.
gg = ((double)smplrec*sizeof(short) / hh ) * ff; // Time in milliseconds of the wave
TimeToPlay  = (long)gg; // now we have the sleep time


if(smplrec < 32768) fftsize = 32768; // We want to use at least an fftsize of 32768 for small portions of data
else
{
	int tempF=0;int tempB=2;
	int p=1;
	while(1)
	{
		tempF = (int) (Math::Pow(tempB,p));
		if (tempF > smplrec ) break;
		p++;
	}
	 fftsize= tempF;
}
coeff = (float)m_samplerate/(float)fftsize; // this coefficient serves for drawing correctly the frequency
m_vector=new double [2*fftsize];
for(int n=0; n<fftsize;n++)
{
	if(n < smplrec)	m_vector[2*n]=(double)tempaudio[n]; //we fill the pairs 2,4,6,8,10
	else m_vector[2*n]=0; //the remaining are filled with zeros 
	m_vector[2*n+1]=0; // the odds are the imaginary part and must be initialized to zero
}
// We invoke the function with 1 as sign (-1 returns the original signal affected by machine error)
ComplexFFT(m_vector, fftsize,1);
// now we must understand what is the max extent of our window.
double m_maxyfft=0;double m_tempyfft;
for(int n = 0 ; n < (fftsize); n++)
{
	m_tempyfft = Math::Sqrt( (m_vector[2*n] *  m_vector[2*n])  +    ( m_vector[2*n+1] * m_vector[2*n+1]) );
	//m_tempyfft = Math::Log10( (m_vector[2*n] *  m_vector[2*n])  +    ( m_vector[2*n+1] * m_vector[2*n+1]) );
	if (m_tempyfft > m_maxyfft)  m_maxyfft = m_tempyfft;
}
float coeffX =  (float)got->m_pbox->ClientRectangle.Width /(float)m_freq_ext; // We are interested to wwatch frequencies of human so we stay between 0 and 3000 hz
float coeffY = (float)got->m_pbox->ClientRectangle.Height/(float)m_maxyfft; // is the absolute  y extent of our windows
// First we create the graphics object
// We change the direction of the y axis by 180 degrees
MatFft = gcnew System::Drawing::Drawing2D::Matrix(1,0,0,-1, 0,0);							
MatFft->Scale(coeffX,coeffY);
// We move the x axis to the botton of the picturebox
MatFft->Translate(0,-(float)m_maxyfft);
offScreenDC->Transform = MatFft;
float m_tempyfftprev, m_tempyfftpost;float gtemp=0;

// *********************************************************************

if(AnalyzeIntensityOfsamples((LPSTR)tempaudio,smplrec*sizeof(short)))
{
for(int n = 0 ; n < fftsize -1; n++)
{
	m_tempyfftprev =(float) Math::Sqrt(  (m_vector[2*n] *  m_vector[2*n])  +  ( m_vector[2*n+1] * m_vector[2*n+1]) );
	n++;
	m_tempyfftpost =(float) Math::Sqrt(  (m_vector[2*n] *  m_vector[2*n])  +  ( m_vector[2*n+1] * m_vector[2*n+1]) );
	gtemp=(float)n*coeff;
	offScreenDC->DrawLine(PenFFT,  (n-1) * coeff    , m_tempyfftprev,  gtemp  , m_tempyfftpost);
	// We multiply the x axis * coeff because we must acknowledge the difference between the current samplerate and the tftsize
}
got->m_pbox->Image=canvas;
Thread::Sleep(TimeToPlay);

}


else
{

for(int n = 0 ; n < fftsize -1; n++)
{
	m_tempyfftprev =(float) Math::Sqrt(  (m_vector[2*n] *  m_vector[2*n])  +  ( m_vector[2*n+1] * m_vector[2*n+1]) );
	n++;
	m_tempyfftpost =(float) Math::Sqrt(  (m_vector[2*n] *  m_vector[2*n])  +  ( m_vector[2*n+1] * m_vector[2*n+1]) );
	gtemp=(float)n*coeff;
	offScreenDC->DrawLine(PenFFT1,  (n-1) * coeff    , m_tempyfftprev,  gtemp  , m_tempyfftpost);
	// We multiply the x axis * coeff because we must acknowledge the difference between the current samplerate and the tftsize
}
got->m_pbox->Image=canvas;
Thread::Sleep(TimeToPlay);
}


//**********************************************************************************************************



delete [] m_vector;		
}
	got->m_pbox->Image=nullptr;	
	MatFft=nullptr;

}


//**************************************************************************************************
//**************************************************************************************************


Void NotifySPECTROGRAM(Object ^ al)
{
PackDCHWND ^ got = (PackDCHWND^)al;		
int fftsize =0;
float coeff=0;
int m_samplerate=got->m_samplerate;
double * m_vector=NULL;
System::Drawing::Bitmap ^ temp= gcnew System::Drawing::Bitmap(got->m_pbox->Width, got->m_pbox->Height);
System::Drawing::Bitmap ^ canvas = gcnew System::Drawing::Bitmap(got->m_pbox->Width, got->m_pbox->Height);
got->m_pbox->Image=canvas;
while(recording==1)
{
int smplrec = whdr[0].dwBytesRecorded == 0 ?  (whdr[1].dwBytesRecorded)/sizeof(short) :(whdr[0].dwBytesRecorded)/sizeof(short) ;
if(smplrec < 2000) continue;

static double hh= 2 * got->m_samplerate,ff=1000; // parameter necessary to compute the time to sleep.
double gg; // parameter necessary to compute the time to sleep.
int TimeToPlay;// parameter necessary to compute the time to sleep.
gg = ((double)smplrec*sizeof(short) / hh ) * ff; // Time in milliseconds of the wave
TimeToPlay  = (long)gg; // now we have the sleep time

short * tempaudio = whdr[0].dwBytesRecorded == 0 ? (short*) whdr[1].lpData :(short*) whdr[0].lpData;	
if(smplrec < 32768) fftsize = 32768; // We want to use at least an fftsize of 32768 for small portions of data
else
{
	int tempF=0;int tempB=2;
	int p=1;
	while(1)
	{
		tempF = (int) (Math::Pow(tempB,p));
		if (tempF > smplrec ) break;
		p++;
	}
	 fftsize= tempF;
}
coeff = (float)m_samplerate/(float)fftsize; // this coefficient serves for drawing correctly the frequency
m_vector=new double [2*fftsize];
for(int n=0; n<fftsize;n++)
{
	if(n < smplrec)	m_vector[2*n]=(double)tempaudio[n]; //we fill the pairs 2,4,6,8,10
	else m_vector[2*n]=0; //the remaining are filled with zeros 
	m_vector[2*n+1]=0; // the odds are the imaginary part and must be initialized to zero
}
// We invoke the function with 1 as sign (-1 returns the original signal affected by machine error)
ComplexFFT(m_vector, fftsize,1);
// now we must understand what is the max extent of our window.
double m_maxyfft=0;double m_tempyfft;

for(int n = 0 ; n < (fftsize); n++)
{
	m_tempyfft = Math::Sqrt( (m_vector[2*n] *  m_vector[2*n])  +    ( m_vector[2*n+1] * m_vector[2*n+1]) );
	if (m_tempyfft > m_maxyfft)  m_maxyfft = m_tempyfft;
	
}
temp = (System::Drawing::Bitmap ^) got->m_pbox->Image; // declare a new bitmap from picbox
Thread::Sleep(TimeToPlay);
	PixelFormat  format1 = temp->PixelFormat; // Read the format	
	BitmapData ^ data1 = temp->LockBits(System::Drawing::Rectangle(0, 0, temp->Width, temp->Height), System::Drawing::Imaging::ImageLockMode::ReadOnly, format1);
	int bytes = temp->Width * (temp->Height-1) * 4; // The number of bytes for all row -1
	array<Byte>^rgbValues = gcnew array<Byte>(bytes); // The array which holds all rows -1
	unsigned char * pixel1 = (unsigned char *)data1->Scan0.ToPointer()+data1->Stride; // The address of the second line
	IntPtr ptr = (IntPtr)pixel1; // The address converted to pointer
	System::Runtime::InteropServices::Marshal::Copy( ptr, rgbValues, 0, bytes ); // We copy all from the second line
	IntPtr ptrzero = data1->Scan0; // The address of the first line
	System::Runtime::InteropServices::Marshal::Copy( rgbValues, 0, ptrzero, bytes ); // We shift to the first line
	pixel1+=(data1->Stride * (temp->Height-2)); // The address of the last row
							
	for (int x = 0; x < temp->Width; x++, pixel1 += 4) // For every row (The pixel sums so the next row is at the right starting address
	{                      
		  
	       int xfft= int(6000.0F *x/temp->Width / coeff);
           double ampli = Math::Sqrt( (m_vector[2*xfft] *  m_vector[2*xfft])  +    ( m_vector[2*xfft+1] * m_vector[2*xfft+1]) );
           double ratio = m_maxyfft/ampli;		  
		   pixel1[0] = (unsigned char) ((255 / ratio));	 // The blue component
		   pixel1[1] = (unsigned char) ((255 / ratio));	 // The green component		   
		   pixel1[2]= (unsigned char) ((255 / ratio));		   
		   pixel1[3] = 255; // The alpha or intensity
     }

		temp->UnlockBits( data1 );	
		got->m_pbox->Image = temp;
		

}


}


void SetRecThreshold(int m_value)
{
	m_min_recthreshold=m_value;
}

void SetFFTfreqExtent(int m_value)
{
	m_freq_ext=m_value;
}


void NotifyGDIPLUSBITMAP(Object ^ al)
{	
	PackDCHWND ^ got = (PackDCHWND^)al;		
	System::Drawing::Pen ^ PenREJ = gcnew System::Drawing::Pen(System::Drawing::Color::Red);// The color of rejected sample
	System::Drawing::Pen ^ PenREC = gcnew System::Drawing::Pen(System::Drawing::Color::Green);// the color of recorded samples
	int m_height = got->m_pbox->Height;
	int m_width = got->m_pbox->Width;
	float coeffy = ((float)m_height) / MAXSHORT;
	System::Drawing::Drawing2D::Matrix ^ Mat;// In order not to create a new matrix every time	
	
	int smplrec=0;
	while(recording==1)
	{	
	smplrec = whdr[0].dwBytesRecorded == 0 ?  (whdr[1].dwBytesRecorded)/sizeof(short) :(whdr[0].dwBytesRecorded)/sizeof(short) ;
	if(smplrec < 2000) continue;
	 // Here we calculate the duration of the sample *****************************************************
		static double hh= 2 * got->m_samplerate,ff=1000; // parameter necessary to compute the time to sleep.
		double gg; // parameter necessary to compute the time to sleep.
		int TimeToPlay;// parameter necessary to compute the time to sleep.
		gg = ((double)smplrec*sizeof(short) / hh ) * ff; // Time in milliseconds of the wave
		TimeToPlay  = (long)gg; // now we have the sleep time
	System::Drawing::Bitmap ^ canvas = gcnew System::Drawing::Bitmap(got->m_pbox->Width, got->m_pbox->Height);
	System::Drawing::Graphics ^ offScreenDC = System::Drawing::Graphics::FromImage(canvas);
	
		short * m_realdata = whdr[0].dwBytesRecorded == 0 ? (short*) whdr[1].lpData :(short*) whdr[0].lpData;	
		short* tempaudio = new short[smplrec];
		memcpy_s(tempaudio, smplrec * sizeof(short),m_realdata,smplrec * sizeof(short));
		float coeffx = (float)m_width / (float)smplrec;	
		Mat = gcnew System::Drawing::Drawing2D::Matrix(1,0,0,-1,0,(float)(m_height)/2);
		Mat->Scale(coeffx,coeffy);
		offScreenDC->Transform=Mat;

// Only the first time we must check
DWORD sz=	GetFileSize(hAudiofile,nullptr);
if(sz==0)
{
		if(AnalyzeIntensityOfsamples((LPSTR)tempaudio,smplrec*sizeof(short)))
			{	
				for(int j =0; j< smplrec-1;j++) offScreenDC->DrawLine(PenREC,j,tempaudio[j], (j+1),tempaudio[j+1]);	
				got->m_pbox->Image=canvas;	
				Thread::Sleep(TimeToPlay);
			}
			else
			{
				for(int j =0; j< smplrec-1;j++) offScreenDC->DrawLine(PenREJ,j,tempaudio[j], (j+1),tempaudio[j+1]);	
				got->m_pbox->Image=canvas;
				Thread::Sleep(TimeToPlay);
			}
}

else // The second time all the recorded is kept
{
				for(int j =0; j< smplrec-1;j++) offScreenDC->DrawLine(PenREC,j,tempaudio[j], (j+1),tempaudio[j+1]);	
				got->m_pbox->Image=canvas;	
				Thread::Sleep(TimeToPlay);
}




		delete[]tempaudio;
	}
	got->m_pbox->Image=nullptr;
	
}

//void NotifyGDI(Object ^ al) // This requires a device context
//{	
//	PackDCHWND ^ got = (PackDCHWND^)al;
//	IntPtr scr = got->m_hdc;
//	IntPtr hw=got->m_hwnd;
//	RECT alfa;
//	GetClientRect((HWND)hw.ToPointer(),&alfa);
//	int mode = SetROP2((HDC) (scr.ToPointer()),R2_XORPEN);
//	SetGraphicsMode( (HDC)(scr.ToPointer())  ,GM_ADVANCED);
//	HPEN pen = CreatePen(PS_SOLID,1,RGB(255,0,54));// the pen that draws unrecorded data
//	HPEN penREC = CreatePen(PS_SOLID,1,RGB(0,255,25));// the pen that draws recorded data
//	HBRUSH brush = CreateSolidBrush(RGB(0,0,0));
//	HGDIOBJ   brushold = SelectObject((HDC) (scr.ToPointer()),brush);
//	HGDIOBJ   penold;// = SelectObject((HDC) (scr.ToPointer()),pen);
//	const int m_buf_size =whdr[1].dwBufferLength;
//	POINT * total=new POINT[m_buf_size]; // The maximum value of bytes recorded	
//	SetMapMode(  (HDC) (scr.ToPointer())  ,MM_ANISOTROPIC);
//	//SetWindowOrgEx((HDC) (scr.ToPointer()), 0, 0, NULL);
//	POINT m_oldpoint;
//	SetViewportOrgEx((HDC) (scr.ToPointer()),0,238/2,&m_oldpoint);
//	SIZE m_oldsize;
//	SetViewportExtEx((HDC) (scr.ToPointer()),651,-238,&m_oldsize);// i added the minus sign 
//	while(recording==1)
//	{	
//	//RedrawWindow((HWND)hw.ToPointer(),&alfa,NULL,RDW_INVALIDATE);
//	int smplrec = whdr[0].dwBytesRecorded == 0 ?  (whdr[1].dwBytesRecorded)/sizeof(short) :(whdr[0].dwBytesRecorded)/sizeof(short) ;
//	if(smplrec==0) continue;	
//	short * tempaudio = whdr[0].dwBytesRecorded == 0 ? (short*) whdr[1].lpData :(short*) whdr[0].lpData;	
//	SetWindowExtEx((HDC) (scr.ToPointer()),smplrec,32768,&m_oldsize);	
//	for( int p = 0; p < smplrec; p++ )
//	{
//		total[p].x=p;
//		total[p].y=tempaudio[p];
//	}
//
//if(AnalyzeIntensityOfsamples((LPSTR)tempaudio,smplrec*sizeof(short),got->m_threshold))
//{
//	penold = SelectObject((HDC) (scr.ToPointer()),penREC);	// These are samples that are recorded
//	Polyline((HDC) (scr.ToPointer()),total,smplrec);
//	Polyline((HDC) (scr.ToPointer()),total,smplrec);
//	//Rectangle((HDC) (scr.ToPointer()),0,-16384,smplrec,16384);
//}
//else
//{
//	penold = SelectObject((HDC) (scr.ToPointer()),pen);	// these samples are not recorded
//	Polyline((HDC) (scr.ToPointer()),total,smplrec);
//	//Rectangle((HDC) (scr.ToPointer()),0,-16384,smplrec,16384);
//	Polyline((HDC) (scr.ToPointer()),total,smplrec);
//}
//
//	}
//	delete[] total;
//	/*SelectObject((HDC) (scr.ToPointer()),penold);
//	SelectObject((HDC) (scr.ToPointer()),brushold);*/
//}

void StopRecord()
{
	int res;
	recording = 0;
	// In the following line we manage the fact that we record a small amount of data
	// so the callback is not yet invoked
	if(GetFileSize(hAudiofile,NULL) == 0)
	{
// We analyze if it's worth recording
if(AnalyzeIntensityOfsamples(whdr[0].lpData,whdr[0].dwBufferLength))
{
	DWORD m_written=0;
	WriteFile(hAudiofile,whdr[0].lpData,whdr[0].dwBytesRecorded,&m_written,NULL);
}
		res=waveInReset(hwi);
		res=waveInStop(hwi); // The callback is invoked here and all last recorded data are written to file
		for(int c=0; c< NUM_BUFFERS; c++)
		{		
		waveInUnprepareHeader(hwi, &whdr[c], sizeof(WAVEHDR));	
		delete[] wavBuffer[c];		
		}		
		res = waveInClose(hwi);	
		CloseHandle(hAudiofile);
		delete [] wavBuffer;
		return;
	}
	else{
		res=waveInReset(hwi);
		res=waveInStop(hwi); // The callback is invoked here and all last recorded data are written to file
		// from the callback body
		
	for(int c=0; c< NUM_BUFFERS; c++)
		{		
		waveInUnprepareHeader(hwi, &whdr[c], sizeof(WAVEHDR));	
		delete[] wavBuffer[c];		
		}		
		res = waveInClose(hwi);
		CloseHandle(hAudiofile);
		delete []  wavBuffer;		
		return;
	}		
	
}

short * GetAudiDataFromRecordSession(int &m_sizeaudio_inbytes)
{
	// At the end of a recording session we have the file c:\temprecord.pcm which is filled with the recorded data.
	// This file has not header and the sample rate information comes from the setup in recording. 
	// Consequently this function must be called only when the Stoprecord button is pressed.
	// First of all we check if the file exist by opening it
	FILE * hRecorded;
	if (fopen_s(&hRecorded, "c:\\temprecord.pcm", "rb") != 0 )
	{
	   hRecorded = NULL;
	   return nullptr;
	}
	// If we come here it means that the file is present. however we must check for a consistent filesize
	int ret= fseek(hRecorded, 0, SEEK_END); // I move the file pointer to the end of file so I can read its length
	m_sizeaudio_inbytes = ftell(hRecorded);// here I read the file length
	if(m_sizeaudio_inbytes == 0)
	{
		fclose(hRecorded); 
		return nullptr;
	}
	rewind(hRecorded);// I move the pointer back to the beginning to start reading operation
	unsigned char * m_audiodata_in_bytes = new unsigned char[m_sizeaudio_inbytes];
	fread(m_audiodata_in_bytes, 1, m_sizeaudio_inbytes, hRecorded);
	// now we can close the file because we have all we need
	fclose(hRecorded);
	short * m_audidata_in_samples = (short*) m_audiodata_in_bytes;
	return m_audidata_in_samples;
}


short * ShortenSignalU(short * bufShort,int m_sizeinsample, int ratio,int step, int &m_newsize)
{

short * result;
int n=0;
int lun =0;
result = new short[m_sizeinsample]; // We don't know the 
memset(result,0,m_sizeinsample*sizeof(short));
for(int x=0; x < m_sizeinsample; x++)
{
	
	// Step is 1102 for a samplerate of 11025
	if(n== step)
	{
		//memcpy(&result[lun-step],HammingWindow(&result[lun-step],step),2);
		// It appears to be more convenient to use a cosine windowing for smoothing discontinuities
		memcpy(&result[lun-step],CosineWindow(&result[lun-step],step),2);
		n=0;memset(&result[lun],0,(m_sizeinsample-lun)*sizeof(short));
		x= x+ratio;  //<---  400  gives a stretch of 67,75% ratio
	}
	result[lun]= bufShort[x];n++;lun++;

}
  
m_newsize=lun;
return result;

}


short * DoubleSpeed(short * bufShort,int m_sizeinsample, float ratio, int &m_newsize)
{
// ratio must be between 1 and 2
// first of all we calculate the new size
m_newsize = (int)(m_sizeinsample/ratio);
short * result = new short[m_newsize];
memset(result,0,m_newsize*sizeof(short));
int m_toremove = m_sizeinsample-m_newsize;
// we must try to distribute this population in chunks in order not to alter to much the formant of the word
// we can create 20 chunks so
int m_sizechunk = m_toremove / 10;

// now from the beginning to end we must remove 20 chunks of audio data 
for (int n = 0; n< 10; n++)
{
	memcpy_s(&result[n * m_sizechunk] ,m_newsize*sizeof(short),
	CosineWindow(& bufShort[(2*n) * m_sizechunk],m_sizechunk) ,m_sizechunk * sizeof(short));
}

return result;
}


short * ShortenSignalRAND(short * bufShort,int m_sizeinsample, float ratio, int &m_newsize)
{
ArrayList ^ m_usedvalues = gcnew ArrayList();m_usedvalues->Sort();
int fragstart =0;
int fragend =m_sizeinsample;
float m_speed_coeff =ratio;
int newsize =Convert::ToInt32(((float)fragend)/m_speed_coeff);
m_newsize=newsize;
int m_sample_toremove=fragend-newsize;
Random ^ rand;int q=0;
rand = gcnew Random(System::DateTime::Now.Millisecond * System::DateTime::Now.Second );
while(true)
{  
   q = rand->Next(0,fragend); 
   if(m_usedvalues->BinarySearch(q) < 0 ){m_usedvalues->Add(q);m_usedvalues->Sort(); }
   if( m_usedvalues->Count==m_sample_toremove) break;	
}
int j = m_usedvalues->Count;
short * scaledsignal = new short[fragend];

memset(scaledsignal,0,newsize*sizeof(short));int n=0;
for(int g =0; g < fragend;g++)
{
	if(m_usedvalues->BinarySearch((Object^)g) < 0 )
	{
		scaledsignal[n]=bufShort[g];
		n+=1; 
	};

}
m_usedvalues=nullptr;
return scaledsignal;
}

//************************************************************************************************************

short * PopulationRemoval(short * m_original, int m_sizeoriginal, int ratio, int &m_newsize,int m_samplerate)
{
// Small words must be scaled with a different accuracy because they contain less information,
// it is important to fine tuning the scaling process for small words by creating smaller chunks 
//             TO DO************************************
	// at present we choose not to scale audio fragments whic are less then maximum allowed frequency for a given
	// samplerate. We 'll see later  Genoa 22/04/2009	
	/*if(m_sizeoriginal < ((2 * m_samplerate)/10) )
	{
		m_newsize= m_sizeoriginal;	
		short * m_nochanged = new short[m_sizeoriginal];
		memcpy_s(m_nochanged,m_sizeoriginal*sizeof(short),m_original,m_sizeoriginal*sizeof(short));
		return m_nochanged;	
	}*/
int m_fade_increase = 25 ; //this value is used to smooth (it seems it ranges between 18 and 40)
int numchunks=20;
int rem = m_sizeoriginal % numchunks; // This is the remainder that allows us to calculate the size of the last chunk
int sizechunk = m_sizeoriginal/numchunks;
// The following operation is very important to avoid buffer overrun during the smooting process (see ***)
m_fade_increase= m_fade_increase > (sizechunk/2) ? sizechunk/2: m_fade_increase;
// so we have 20 chunks of sizechunk and a reminder of rem
int lastchunksize = sizechunk + rem;	// If the division has not remainder the last chunk is equal to the others
// Chunks tassellation **************************************************************************
short ** allchunks; allchunks = new short*[numchunks];
int q=0;
for (q = 0; q < (numchunks-1); q++)
{
allchunks[q] = new short[sizechunk]; // I allocate a chunk
memcpy_s(allchunks[q],sizechunk * sizeof(short),&m_original[q*sizechunk],sizechunk * sizeof(short));
}
allchunks[q]= new short[lastchunksize]; // this is the last chunk that is numchunks-1
memcpy_s(allchunks[q],lastchunksize *sizeof(short),&m_original[q*sizechunk],lastchunksize * sizeof(short));
//Chunks removal and rebuild**********************************************************************
short * recomposeds = new short[m_sizeoriginal-(ratio*sizechunk)]; // based on the table we discard the eighth chunk
m_newsize=m_sizeoriginal-(ratio*sizechunk);
memset(recomposeds,0,(m_sizeoriginal-(ratio*sizechunk) )*sizeof(short));
int real =0;q=0;
switch(ratio)
{
case 1: // 5% speed enhanced
	for(q=0; q <(numchunks-1); q++)
	{
	if(q==8)
	{
		// we tried this but it is not so good
		/*short *temp1 =HammingWindow(allchunks[q+1],sizechunk);memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk *sizeof(short));
		short * temp2=HammingWindow(allchunks[q-1],sizechunk);memcpy_s(allchunks[q-1],sizechunk*sizeof(short),temp2,sizechunk *sizeof(short));*/
		// this is very good
		short * temp1=TriangularIncrease(allchunks[q+1],m_fade_increase); //(see ***)
		memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk * sizeof(short));
		short * temp2=TriangularFading(&recomposeds[real * sizechunk]-m_fade_increase,m_fade_increase);
		memcpy_s(&recomposeds[real * sizechunk],sizechunk*sizeof(short),temp2,sizechunk * sizeof(short));
		
		
		continue;
	} // here! Smoothing
	else{memcpy_s(&recomposeds[real * sizechunk],m_sizeoriginal*sizeof(short),allchunks[q],sizechunk*sizeof(short));}
	real++; }
	// the last chunk to add to the recomposed signal
	memcpy_s(&recomposeds[real*sizechunk],lastchunksize*sizeof(short),allchunks[q],lastchunksize*sizeof(short));
break;

case 2: // 10% speed enhanced
	for(q=0; q <(numchunks-1); q++)
	{  
	if( (q==4) || (q==16) )
	{
		// we tried this but it is not so good
		/*short *temp1 =HammingWindow(allchunks[q+1],sizechunk);memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk *sizeof(short));
		short * temp2=HammingWindow(allchunks[q-1],sizechunk);memcpy_s(allchunks[q-1],sizechunk*sizeof(short),temp2,sizechunk *sizeof(short));*/
		
		short * temp1=TriangularIncrease(allchunks[q+1],m_fade_increase);
		memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk * sizeof(short));
		short * temp2=TriangularFading(&recomposeds[real * sizechunk]-m_fade_increase,m_fade_increase);
		memcpy_s(&recomposeds[real * sizechunk],sizechunk*sizeof(short),temp2,sizechunk * sizeof(short));
		
		
		continue;
	} // here! Smoothing
	else{memcpy_s(&recomposeds[real * sizechunk],m_sizeoriginal*sizeof(short),allchunks[q],sizechunk*sizeof(short));}
	real++; }
	// the last chunk to add to the recomposed signal
	memcpy_s(&recomposeds[real*sizechunk],lastchunksize*sizeof(short),allchunks[q],lastchunksize*sizeof(short));
break;

case 3: // 15% speed enhanced
	for(q=0; q <(numchunks-1); q++)
	{
	if( (q==2) || (q==8) || (q==17)  )
	{
		short * temp1=TriangularIncrease(allchunks[q+1],m_fade_increase);
		memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk * sizeof(short));
		short * temp2=TriangularFading(&recomposeds[real * sizechunk]-m_fade_increase,m_fade_increase);
		memcpy_s(&recomposeds[real * sizechunk],sizechunk*sizeof(short),temp2,sizechunk * sizeof(short));

		continue;
	} // here! Smoothing
	else{memcpy_s(&recomposeds[real * sizechunk],m_sizeoriginal*sizeof(short),allchunks[q],sizechunk*sizeof(short));}
	real++; }
	// the last chunk to add to the recomposed signal
	memcpy_s(&recomposeds[real*sizechunk],lastchunksize*sizeof(short),allchunks[q],lastchunksize*sizeof(short));
break;

case 4: // 20% speed enhanced
	for(q=0; q <(numchunks-1); q++)
	{
	if( (q==2) || (q==6) || (q==12) || (q==17) )
	{
		short * temp1=TriangularIncrease(allchunks[q+1],m_fade_increase);
		memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk * sizeof(short));
		short * temp2=TriangularFading(&recomposeds[real * sizechunk]-m_fade_increase,m_fade_increase);
		memcpy_s(&recomposeds[real * sizechunk],sizechunk*sizeof(short),temp2,sizechunk * sizeof(short));
		continue;	
	} // here! Smoothing
	else{memcpy_s(&recomposeds[real * sizechunk],m_sizeoriginal*sizeof(short),allchunks[q],sizechunk*sizeof(short));}
	real++; }
	// the last chunk to add to the recomposed signal
	memcpy_s(&recomposeds[real*sizechunk],lastchunksize*sizeof(short),allchunks[q],lastchunksize*sizeof(short));
break;

case 5: // 25% speed enhanced
	for(q=0; q <(numchunks-1); q++)
	{
	if( (q==2) || (q==6) || (q==9) || (q==14) || (q==18) )
	{
		
		short * temp1=TriangularIncrease(allchunks[q+1],m_fade_increase);
		memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk * sizeof(short));
		short * temp2=TriangularFading(&recomposeds[real * sizechunk]-m_fade_increase,m_fade_increase);
		memcpy_s(&recomposeds[real * sizechunk],sizechunk*sizeof(short),temp2,sizechunk * sizeof(short));
		
		continue;
	} // here! Smoothing
	else{memcpy_s(&recomposeds[real * sizechunk],m_sizeoriginal*sizeof(short),allchunks[q],sizechunk*sizeof(short));}
	real++; }

	// the last chunk to add to the recomposed signal
	memcpy_s(&recomposeds[real*sizechunk],lastchunksize*sizeof(short),allchunks[q],lastchunksize*sizeof(short));
break;

case 6: // 30% speed enhanced
	for(q=0; q <(numchunks-1); q++)
	{
	if( (q==1) || (q==4) || (q==7) || (q==11) || (q==15) || (q==18) )
	{
		short * temp1=TriangularIncrease(allchunks[q+1],m_fade_increase);
		memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk * sizeof(short));
		short * temp2=TriangularFading(&recomposeds[real * sizechunk]-m_fade_increase,m_fade_increase);
		memcpy_s(&recomposeds[real * sizechunk],sizechunk*sizeof(short),temp2,sizechunk * sizeof(short));

		continue;
	} // here! Smoothing
	else{memcpy_s(&recomposeds[real * sizechunk],m_sizeoriginal*sizeof(short),allchunks[q],sizechunk*sizeof(short));}
	real++; }
	// the last chunk to add to the recomposed signal
	memcpy_s(&recomposeds[real*sizechunk],lastchunksize*sizeof(short),allchunks[q],lastchunksize*sizeof(short));
break;

case 7: // 35% speed enhanced
	for(q=0; q <(numchunks-1); q++)
	{
	if( (q==1) || (q==4) || (q==7) || (q==10) || (q==13) || (q==16) || (q==18) )
	{
		short * temp1=TriangularIncrease(allchunks[q+1],m_fade_increase);
		memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk * sizeof(short));
		short * temp2=TriangularFading(&recomposeds[real * sizechunk]-m_fade_increase,m_fade_increase);
		
		memcpy_s(&recomposeds[real * sizechunk],sizechunk*sizeof(short),temp2,sizechunk * sizeof(short));
		continue;
	} // here! Smoothing
	else{memcpy_s(&recomposeds[real * sizechunk],m_sizeoriginal*sizeof(short),allchunks[q],sizechunk*sizeof(short));}
	real++; }
	// the last chunk to add to the recomposed signal
	memcpy_s(&recomposeds[real*sizechunk],lastchunksize*sizeof(short),allchunks[q],lastchunksize*sizeof(short));
break;

case 8: // 40% speed enhanced
	for(q=0; q <(numchunks-1); q++)
	{
	if( (q==1) || (q==4) || (q==7) || (q==9) || (q==11) || (q==13) || (q==15) || (q==18) )
	{
		short * temp1=TriangularIncrease(allchunks[q+1],m_fade_increase);
		memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk * sizeof(short));
		short * temp2=TriangularFading(&recomposeds[real * sizechunk]-m_fade_increase,m_fade_increase);
		memcpy_s(&recomposeds[real * sizechunk],sizechunk*sizeof(short),temp2,sizechunk * sizeof(short));
		
		continue;
	} // here! Smoothing
	else{memcpy_s(&recomposeds[real * sizechunk],m_sizeoriginal*sizeof(short),allchunks[q],sizechunk*sizeof(short));}
	real++; }
	// the last chunk to add to the recomposed signal
	memcpy_s(&recomposeds[real*sizechunk],lastchunksize*sizeof(short),allchunks[q],lastchunksize*sizeof(short));
break;

case 9: // 45% speed enhanced
	for(q=0; q <(numchunks-1); q++)
	{
	if((q==1)|| (q==3) || (q==5) || (q==7) || (q==9) || (q==11) || (q==13) || (q==15) || (q==18) )
	{
		short * temp1=TriangularIncrease(allchunks[q+1],m_fade_increase);
		memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk * sizeof(short));
		short * temp2=TriangularFading(&recomposeds[real * sizechunk]-m_fade_increase,m_fade_increase);
		memcpy_s(&recomposeds[real * sizechunk],sizechunk*sizeof(short),temp2,sizechunk * sizeof(short));

		continue;	
	} // here! Smoothing
	else{memcpy_s(&recomposeds[real * sizechunk],m_sizeoriginal*sizeof(short),allchunks[q],sizechunk*sizeof(short));}
	real++; }
	// the last chunk to add to the recomposed signal
	memcpy_s(&recomposeds[(real*sizechunk) + sizechunk],lastchunksize*sizeof(short),allchunks[q],(lastchunksize-sizechunk)*sizeof(short));
break;

case 10: // 50% speed enhanced
	for(q=0; q <(numchunks-1); q++)
	{
	if( (q==1) || (q==3) || (q==5) || (q==7) || (q==9) || (q==11) || (q==13) || (q==15) || (q==17))
	{
		short * temp1=TriangularIncrease(allchunks[q+1],m_fade_increase);
		memcpy_s(allchunks[q+1],sizechunk*sizeof(short),temp1,sizechunk * sizeof(short));
		short * temp2=TriangularFading(&recomposeds[real * sizechunk]-m_fade_increase,m_fade_increase);
		memcpy_s(&recomposeds[real * sizechunk],sizechunk*sizeof(short),temp2,sizechunk * sizeof(short));

		continue;
	} // here! Smoothing
	else{memcpy_s(&recomposeds[real * sizechunk],m_sizeoriginal*sizeof(short),allchunks[q],sizechunk*sizeof(short));}
	real++; }
	// the last chunk to add to the recomposed signal
	memcpy_s(&recomposeds[real*sizechunk],lastchunksize*sizeof(short),allchunks[q],(lastchunksize-sizechunk)*sizeof(short)); 
break;
}
delete[] allchunks[numchunks-1];for (q = 0; q < (numchunks-1); q++)delete [] allchunks[q];delete allchunks;allchunks=NULL;

//return PrheEmphasis(recomposeds,m_newsize);
return recomposeds;
}


short * FFTFilter(short * m_original, int m_sizeoriginal, int m_lowfrequency, int m_highfrequency, int m_samplerate)
{
// first of all we must allocate the memory.
int fftsize =0;double * m_vector=NULL;
if(m_sizeoriginal < 32768)
{
	fftsize = 32768; // We want to use at least an fftsize of 32768 for small portions of data
}
// if the audio sample is greater than 32768 we must choose an appropriate fftsize that covers the whole signal
else
{
	int tempF=0;int tempB=2;
	int p=1;
	while(1)
	{
		tempF = (int) (Math::Pow(tempB,p));
		if (tempF > m_sizeoriginal ) break;
		p++;
	}
	 fftsize= tempF;
}
// Whatever, here we have the suitable value of fftsize which is at least 32767
if(m_vector!=nullptr)
{
	delete [] m_vector;
	m_vector = nullptr;
}
float coeff = (float)m_samplerate/(float)fftsize; // this coefficient serves for drawing correctly the frequency
m_lowfrequency=(int)((float)m_lowfrequency * coeff);
m_highfrequency=(int)((float)m_highfrequency * coeff);
// here we allocate our vector for using the fft function
m_vector=new double [2*fftsize];
for(int n=0; n<fftsize;n++)
{
	if(n < m_sizeoriginal)	m_vector[2*n]=(double)m_original[n]; //we fill the pairs 2,4,6,8,10
	else m_vector[2*n]=0; //the remaining are filled with zeros 
	m_vector[2*n+1]=0; // the odds are the imaginary part and must be initialized to zero
}
// We invoke the function with 1 as sign (-1 returns the original signal affected by machine error)
ComplexFFT(m_vector, fftsize,1);
// here we have our frequency domain

//for(int n=0; n<fftsize;n++)
//{
// m_vector[2*n] = 0.F;  
// m_vector[2*n+1]=0.F;
//}

ComplexFFT(m_vector, fftsize, -1);

for(int n=0; n<fftsize;n++)
{
	if(n < m_sizeoriginal)	m_original[n] = (short)m_vector[2*n];
	
}
return m_original;
}

//LOW-PASS WINDOWED-SINC FILTER
// Good performance for a size of the kernel filter = to 100
// the m_windowing string can be String::Empty, hamming, blackman, cosine
inline short * LowPass(short * m_original,int m_sizeoriginal, int m_kernelsize,int m_samplerate, double m_cutfrequency, int m_windowing)
{
	
	double fc = (0.5 * m_cutfrequency) / (m_samplerate/2); // fc varies between 0 and 0.5 fraction of samplerate
	double * m_kernel;
	m_kernel = new double[m_kernelsize];	
	// here we calculate the array for the filter	
	double PI = 3.141592653589793238462643;
	for(int  q=0; q< m_kernelsize; q++)
	{
		if(( q- (m_kernelsize/2) ) == 0 ) m_kernel[q]=   Math::Sin(2*PI*fc) ;
		if(   (q-  (m_kernelsize/2)  ) != 0  )
		m_kernel[q] =( Math::Sin(2*PI*fc * (q-(m_kernelsize/2)))  / (q-( m_kernelsize/2)) );
	}
	if (m_windowing == 1) HammingWindow(m_kernel,m_kernelsize);
	if (m_windowing == 2)BlackmanWindow(m_kernel,m_kernelsize);
	if (m_windowing == 3)CosineWindow(m_kernel,m_kernelsize);

	double sum=0;
	for(int i = 0; i<m_kernelsize; i++)
	{
	sum = sum +m_kernel[i];
	}
		for(int i =0; i<m_kernelsize; i++)
		{
		m_kernel[i] = m_kernel[i]/sum;
		}
	int m_sizeconvolved = m_sizeoriginal + m_kernelsize;
	short * m_convolved = new short[m_sizeconvolved];
	memset(m_convolved,0,m_sizeconvolved * sizeof(short));
	for(int i =0; i < m_sizeoriginal; i++)
	{
		for(int j =0; j < m_kernelsize; j++)   m_convolved[i+j]= m_convolved[i+j] + (short)( m_original[i] * m_kernel[j]); 
	}
	float m_coeff_norm=0;
	delete[] m_kernel;
	// we normalize the audio data and eliminate the head and tail that were formed with convolution with the 
	// Filter kernel
	NormalizeAudio(m_convolved,m_sizeconvolved,m_coeff_norm);
	short * m_filtered = new short[m_sizeoriginal];
	memcpy_s(m_filtered,sizeof(short)*m_sizeoriginal,&m_convolved[m_kernelsize/2],sizeof(short)*m_sizeoriginal);
	delete[] m_convolved;
	return m_filtered;	
	

}

//HIGH-PASS WINDOWED-SINC FILTER
// Good performance for a size of the kernel filter = to 100
// the m_windowing string can be String::Empty, hamming, blackman, cosine
inline short * HighPass(short * m_original,int m_sizeoriginal, int m_kernelsize,int m_samplerate, double m_cutfrequency, int m_windowing)
{	

	
	double fc = (0.5 * m_cutfrequency) / (m_samplerate/2); // fc varies between 0 and 0.5 fraction of samplerate
	double * m_kernel;
	m_kernel = new double[m_kernelsize];	
	// here we calculate the array for the filter	
	double PI = 3.141592653589793238462643;
	for(int  q=0; q< m_kernelsize; q++)
	{
		if(( q- (m_kernelsize/2) ) == 0 ) m_kernel[q]=   Math::Sin(2*PI*fc) ;
		if(   (q-  (m_kernelsize/2)  ) != 0  )
		m_kernel[q] =( Math::Sin(2*PI*fc * (q-(m_kernelsize/2)))  / (q-( m_kernelsize/2)) );
	}
	if (m_windowing == 1) HammingWindow(m_kernel,m_kernelsize);
	if (m_windowing == 2) BlackmanWindow(m_kernel,m_kernelsize);
	if (m_windowing == 3) CosineWindow(m_kernel,m_kernelsize);

	double sum=0;
	for(int i = 0; i<m_kernelsize; i++)
	{
	sum = sum +m_kernel[i];
	}
		for(int i =0; i<m_kernelsize; i++)
		{
		m_kernel[i] = m_kernel[i]/sum;
		}
// SPECTRAL INVERSION changes a low pass filter into a high band pass filter **********************
	for(int  q=0; q< m_kernelsize; q++)
	{
		m_kernel[q] = -m_kernel[q];
	}
	m_kernel[m_kernelsize/2] = m_kernel[m_kernelsize/2]+1;
//****************************************************************END Spectral Inversion***********
	int m_sizeconvolved = m_sizeoriginal + m_kernelsize;
	short * m_convolved = new short[m_sizeconvolved];
	memset(m_convolved,0,m_sizeconvolved * sizeof(short));
	for(int i =0; i < m_sizeoriginal; i++)
	{
		for(int j =0; j < m_kernelsize; j++)   m_convolved[i+j]= m_convolved[i+j] + (short)( m_original[i] * m_kernel[j]); 
	}
	float m_coeff_norm=0;
	delete[] m_kernel;
	// we normalize the audio data and eliminate the head and tail that were formed with convolution with the 
	// Filter kernel
    NormalizeAudio(m_convolved,m_sizeconvolved,m_coeff_norm);
	short * m_filtered = new short[m_sizeoriginal];
	memcpy_s(m_filtered,sizeof(short)*m_sizeoriginal,&m_convolved[m_kernelsize/2],sizeof(short)*m_sizeoriginal);
	delete[] m_convolved;
	return m_filtered;	

}

//BAND-PASS WINDOWED-SINC FILTER
// Good performance for a size of the kernel filter = to 100
// the m_windowing string can be String::Empty, hamming, blackman, cosine
inline short * BandPass(short * m_original,int m_sizeoriginal, int m_kernelsize,int m_samplerate, double m_cutfrequency_low,double m_cutfrequency_high, int  m_windowing)
{
	
	double fcl = (0.5 * m_cutfrequency_low) / (m_samplerate/2); // fc varies between 0 and 0.5 fraction of samplerate
	double fch = (0.5 * m_cutfrequency_high) / (m_samplerate/2); // fc varies between 0 and 0.5 fraction of samplerate
	double * m_kernellow = new double[m_kernelsize];
	double * m_kernelhigh = new double[m_kernelsize];
	double * m_kernelresult =new double[m_kernelsize];
	double PI = 3.141592653589793238462643;
// The first low kernel filter
	for(int  q=0; q< m_kernelsize; q++)
	{
		if(( q- (m_kernelsize/2) ) == 0 ) m_kernellow[q]=Math::Sin(2*PI*fcl) ;
		if(   (q-  (m_kernelsize/2)  ) != 0  )
		m_kernellow[q] =( Math::Sin(2*PI*fcl * (q-(m_kernelsize/2)))  / (q-( m_kernelsize/2)) );
	}
	if (m_windowing == 1) HammingWindow(m_kernellow,m_kernelsize);
	if (m_windowing == 2)BlackmanWindow(m_kernellow,m_kernelsize);
	if (m_windowing == 3)CosineWindow(m_kernellow,m_kernelsize);
	double sum=0;
	// normalization
	for(int i = 0; i<m_kernelsize; i++) sum = sum +m_kernellow[i];
	for(int i =0; i<m_kernelsize; i++)  m_kernellow[i] = m_kernellow[i]/sum;
	sum =0;		
// The second low kernel filter
for(int  q=0; q< m_kernelsize; q++)
	{
		if(( q- (m_kernelsize/2) ) == 0 ) m_kernelhigh[q]=Math::Sin(2*PI*fch) ;
		if(   (q-  (m_kernelsize/2)  ) != 0  )
		m_kernelhigh[q] =( Math::Sin(2*PI*fch * (q-(m_kernelsize/2)))  / (q-( m_kernelsize/2)) );
	}
	if (m_windowing == 1) HammingWindow(m_kernelhigh,m_kernelsize);
	if (m_windowing == 2)BlackmanWindow(m_kernelhigh,m_kernelsize);
	if (m_windowing == 3)CosineWindow(m_kernelhigh,m_kernelsize);
	// normalization
	for(int i = 0; i<m_kernelsize; i++) sum = sum +m_kernelhigh[i];
	for(int i =0; i<m_kernelsize; i++)  m_kernelhigh[i] = m_kernelhigh[i]/sum;
	sum =0;
// SPECTRAL INVERSION changes the second low pass filter into a high band pass filter **********************
	for(int  q=0; q< m_kernelsize; q++)
	{
		m_kernelhigh[q] = -m_kernelhigh[q];
	}
	m_kernelhigh[m_kernelsize/2] = m_kernelhigh[m_kernelsize/2]+1;
	// Add the first low pass filter kernel to the second high pass filter kernel in order to form
	// a band reject filter kernel
for(int  q=0; q< m_kernelsize; q++) m_kernelresult[q] = m_kernellow[q] + m_kernelhigh[q];	
// SPECTRAL INVERSION transforms the band reject filter kernel into a band pass filter kernel
for(int  q=0; q< m_kernelsize; q++)
	{
		m_kernelresult[q] = - m_kernelresult[q];
	}
	m_kernelresult[m_kernelsize/2] = m_kernelresult[m_kernelsize/2]+1;
	delete[] m_kernellow;
	delete[] m_kernelhigh;
// Now we can go for convolution
	int m_sizeconvolved = m_sizeoriginal + m_kernelsize;
	short * m_convolved = new short[m_sizeconvolved];
	memset(m_convolved,0,m_sizeconvolved * sizeof(short));
	for(int i =0; i < m_sizeoriginal; i++)
	{
		for(int j =0; j < m_kernelsize; j++)   m_convolved[i+j]= m_convolved[i+j] + (short)( m_original[i] * m_kernelresult[j]); 
	}
	delete []  m_kernelresult; // We cleanup also the result filter kernel
	float m_coeff_norm=0;
	// we normalize the audio data and eliminate the head and tail that were formed with convolution with the 
	// Filter kernel
	NormalizeAudio(m_convolved,m_sizeconvolved,m_coeff_norm);
	short * m_filtered = new short[m_sizeoriginal];
	memcpy_s(m_filtered,sizeof(short)*m_sizeoriginal,&m_convolved[m_kernelsize/2],sizeof(short)*m_sizeoriginal);
	delete[] m_convolved;
	return m_filtered;
	

}

// this function returns a value of correlation that is = 100 when the two signals are identical
// moreover the function feeds a reference  ( distance which can be used in conjunction with autocorrelation
// if the distance is high and the correlation is low, it means that the two signals have scarce probabilitis to be equal.
inline double  AutoCorrelation(short * m_first_serie, short * m_second_serie, int m_lenfirst, int m_lensecond, double & distance)
{

// We operate in order to confront two signals of the same length by padding with zero's at the beginning and the end
// of the smaller signal in order to reach the length of the bigger signal.
int m_len =  m_lenfirst;
if(m_lenfirst > m_lensecond)
{
	short * temp = new short[m_lenfirst];memset(temp,0,m_lenfirst * sizeof(short)); // zero the array
	int delta = (m_lenfirst-m_lensecond)/2;
    // we place the smaller signal in order to stay in the middle of the size of the bigger signal
	memcpy_s(&temp[delta] ,m_lenfirst * sizeof(short),m_second_serie, m_lensecond * sizeof(short)); 
	m_len = m_lenfirst;
	delete[] m_second_serie; m_second_serie = new short[m_len];
	memcpy_s(m_second_serie,m_len * sizeof(short),temp,m_len * sizeof(short));
	delete[] temp;
}

 if(m_lenfirst < m_lensecond)
{
	short * temp = new short[m_lensecond];memset(temp,0,m_lenfirst * sizeof(short)); // zero the array
	int delta = (m_lensecond-m_lenfirst)/2;
    // we place the smaller signal in order to stay in the middle of the size of the bigger signal
	memcpy_s(&temp[delta] ,m_lensecond * sizeof(short),m_first_serie, m_lenfirst * sizeof(short)); 
	m_len = m_lensecond;
	delete[] m_first_serie; m_first_serie = new short[m_len];
	memcpy_s(m_first_serie,m_len * sizeof(short),temp,m_len * sizeof(short));
	delete[] temp;
}
	
double * m_autocorr = new double[m_len];
   double mx,my,sx,sy,sxy,denom;   
   /* Calculate the mean of the two series x[], y[] */
   mx = 0;
   my = 0;  
   int j =0;
   for (int i=0;i<m_len;i++) {
      mx += m_first_serie[i];
      my += m_second_serie[i];
   }
   mx /= m_len;
   my /= m_len;

   /* Calculate the denominator */
   sx = 0;
   sy = 0;
   for (int i=0;i<m_len;i++) {
      sx += (m_first_serie[i] - mx) * (m_first_serie[i] - mx);
      sy += (m_second_serie[i] - my) * (m_second_serie[i] - my);
   }
   denom = sqrt(sx*sy);

   /* Calculate the correlation series */
   int ncalc=0;
   for (int delay=-m_len/2;delay< m_len/2 ;delay++) {
      sxy = 0;
      for (int i=0;i<m_len;i++) {
         j = i + delay;
         if (j < 0 || j >= m_len)
            continue;
         else
            sxy += (m_first_serie[i] - mx) * (m_second_serie[j] - my);
        
	  }
      m_autocorr[ncalc] = sxy / denom;
	  ncalc+=1;
    }

  // here we can compute the coefficient
double max=0, posmax=0;

for(int y =0; y< m_len; y++)
{
	if( m_autocorr[y] > max){ max =  m_autocorr[y]; posmax =y;}
}
distance = fabs( (double)(m_len/2 - posmax) / (m_len/2) * 100) ;
delete [] m_autocorr;
delete [] m_first_serie;
delete [] m_second_serie;
return max * 100;
}
// this function returns a value of correlation that is = 100 when the two signals are identical
// moreover the function feeds a reference  ( distance which can be used in conjunction with autocorrelation
// if the distance is high and the correlation is low, it means that the two signals have scarce probabilitis to be equal.
inline double  AutoCorrelation(double * m_first_serie, double * m_second_serie, int m_len, double & distance)
{
   double * m_autocorr = new double[m_len];
   double mx,my,sx,sy,sxy,denom;   
   /* Calculate the mean of the two series x[], y[] */
   mx = 0;
   my = 0;  
   int j =0;
   for (int i=0;i<m_len;i++) {
      mx += m_first_serie[i];
      my += m_second_serie[i];
   }
   mx /= m_len;
   my /= m_len;

   /* Calculate the denominator */
   sx = 0;
   sy = 0;
   for (int i=0;i<m_len;i++) {
      sx += (m_first_serie[i] - mx) * (m_first_serie[i] - mx);
      sy += (m_second_serie[i] - my) * (m_second_serie[i] - my);
   }
   denom = sqrt(sx*sy);
   /* Calculate the correlation series */
   int ncalc=0;
   for (int delay=-m_len/2;delay< m_len/2 ;delay++) {
      sxy = 0;
      for (int i=0;i<m_len;i++) {
         j = i + delay;
         if (j < 0 || j >= m_len)
            continue;
         else
            sxy += (m_first_serie[i] - mx) * (m_second_serie[j] - my);
        
	  }
      m_autocorr[ncalc] = sxy / denom;
	  ncalc+=1;
    }

  // here we can compute the coefficient
double max=0, posmax=0;

for(int y =0; y< m_len; y++)
{
	if( m_autocorr[y] > max){ max =  m_autocorr[y]; posmax =y;}
}
distance = fabs( (double)(m_len/2 - posmax) / (m_len/2) * 100) ;
delete [] m_autocorr;
delete [] m_first_serie;
delete [] m_second_serie;
return max * 100;
}


HRESULT RegisterEngine(String ^ dllname)
{
typedef HRESULT    (__cdecl * REGISTERSERVERPROC)(void); 
pin_ptr<String ^> p = &dllname;  // We Block Garbage collection relocation to grant safety
	// to the Unmanaged pointer that represents the filename in the native env.
	LPCWSTR m_dllname =(LPCWSTR)(Marshal::StringToHGlobalUni(dllname)).ToPointer();//Unicode char set
	//*********************************************************************************************
	HINSTANCE hinstLib1; 
	REGISTERSERVERPROC RegisterServerENGINE;
	hinstLib1 = LoadLibrary(m_dllname); 
	RegisterServerENGINE = (REGISTERSERVERPROC) GetProcAddress(hinstLib1, "DllRegisterServer");
	HRESULT hr;
	try
{
	HRESULT hr =RegisterServerENGINE();
}
	catch(Exception ^ex) 
	{String ^ er = ex->Message;
		hr=-1;}
	return hr;

}

HRESULT UnRegisterEngine(String ^ dllname)
{
typedef HRESULT    (__cdecl * UNREGISTERSERVERPROC)(void); 
pin_ptr<String ^> p = &dllname;  // We Block Garbage collection relocation to grant safety
	// to the Unmanaged pointer that represents the filename in the native env.
	LPCWSTR m_dllname =(LPCWSTR)(Marshal::StringToHGlobalUni(dllname)).ToPointer();//Unicode char set
	//*********************************************************************************************
	HINSTANCE hinstLib1; 
	UNREGISTERSERVERPROC UnRegisterServerENGINE;
	hinstLib1 = LoadLibrary(m_dllname); 
	UnRegisterServerENGINE = (UNREGISTERSERVERPROC) GetProcAddress(hinstLib1, "DllUnregisterServer");
	
	
		HRESULT hr;
	try
{
	HRESULT hr =UnRegisterServerENGINE();
}
	catch(Exception ^ex) 
	{String ^ er = ex->Message;
		hr=-1;}
	return hr;
}























};
}
/* American English Phoneme Representation  ***********************************************************

							-			 syllable boundary (hyphen) 1 
							!			 Sentence terminator (exclamation mark) 2 
							&			word boundary 3 
							,			Sentence terminator (comma) 4 
							.			Sentence terminator (period) 5 
							?			Sentence terminator (question mark) 6 
							_			Silence (underscore) 7 
							1			Primary stress 8 
							2			Secondary stress 9 
							aa			father 10 
							ae			cat 11 
							ah			cut 12 
							ao			dog 13 
							aw			foul 14 
							ax			ago 15 
							ay			bite 16 
							b			big 17 
							ch			chin 18 
							d			dig 19 
							dh			then 20 
							eh			pet 21 
							er			fur 22 
							ey			ate 23 
							f			fork 24 
							g			gut 25 
							h			help 26 
							ih			fill 27 
							iy			feel 28 
							jh			joy 29 
							k			cut 30 
							l			lid 31 
							m			mat 32 
							n			no 33 
							ng			sing 34 
							ow			go 35 
							oy			toy 36 
							p			put 37 
							r			red 38 
							s			sit 39 
							sh			she 40 
							t			talk 41 
							th			thin 42 
							uh			book 43 
							uw			too 44 
							v			vat 45 
							w			with 46 
							y			yard 47 
							z			zap 48 
							zh			pleasure 49	

American English Phoneme Representation  *************************************************************/
	

//WAVEFORMATEX * m_format = NULL; m_format = new WAVEFORMATEX;
//m_format->wFormatTag = WAVE_FORMAT_PCM;
//m_format->nChannels = 1;
//m_format->nSamplesPerSec=11025;
//m_format->wBitsPerSample= 16; // 16 bit;
//m_format->nAvgBytesPerSec= m_format->nSamplesPerSec * m_format->nBlockAlign;
//m_format->nBlockAlign= (m_format->nChannels * m_format->wBitsPerSample)/8;
//m_format->cbSize=0;