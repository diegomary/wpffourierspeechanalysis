#include "stdafx.h"
#include "SpeechUtil.h"
#include <memory.h>

namespace SpeechUtil
{
	void LowLevelWrapper::FFTnew(double* data, unsigned long nn) // This is an alternative but it seems it works all the same as the other
	{   
		unsigned long n, mmax, m, j, istep, i;
		double wtemp, wr, wpr, wpi, wi, theta;
		double tempr, tempi; 
		// reverse-binary reindexing
		n = nn<<1;
		j=1;
		for (i=1; i<n; i+=2) {
			if (j>i) {
				SWAP(data[j-1], data[i-1]);
				SWAP(data[j], data[i]);
			}
			m = nn;
			while (m>=2 && j>m) {
				j -= m;
				m >>= 1;
			}
			j += m;
		}; 
		// here begins the Danielson-Lanczos section
		mmax=2;
		while (n>mmax) {
			istep = mmax<<1;
			theta = -(2*Math::PI/mmax);
			wtemp = Math::Sin(0.5*theta);
			wpr = -2.0*wtemp*wtemp;
			wpi = Math::Sin(theta);
			wr = 1.0;
			wi = 0.0;
			for (m=1; m < mmax; m += 2) {
				for (i=m; i <= n; i += istep) {
					j=i+mmax;
					tempr = wr*data[j-1] - wi*data[j];
					tempi = wr * data[j] + wi*data[j-1];
 
					data[j-1] = data[i-1] - tempr;
					data[j] = data[i] - tempi;
					data[i-1] += tempr;
					data[i] += tempi;
				}
				wtemp=wr;
				wr += wr*wpr - wi*wpi;
				wi += wi*wpr + wtemp*wpi;
			}
			mmax=istep;
		}

	}
	void LowLevelWrapper::ComplexFFT(double *vector, unsigned int sample_rate, int sign)
	{ 	
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
	array<double>^ LowLevelWrapper:: WrapFFT(int numaudiosamples,int fftsize, array<short>^ signal, int sign)
	{
	
		pin_ptr<short> pinnedvector = &signal[0];   // pin pointer to first element in array
		short * signalunmanaged = pinnedvector;   // pointer to the first element in arr	
		double*	m_vector=new double [2*fftsize];
		for(int n=0; n<fftsize;n++)
		{
			if(n < numaudiosamples)	m_vector[2*n]=(double)signalunmanaged[n]; //we fill the pairs 2,4,6,8,10
			else m_vector[2*n]=0; //the remaining are filled with zeros 
			m_vector[2*n+1]=0; // the odds are the imaginary part and must be initialized to zero
		}
		// We invoke the function with 1 as sign (-1 returns the original signal affected by machine error)
		ComplexFFT(m_vector, fftsize,1);
		// We convert Back the fft to managed array.
		array<double>^ fftoutput = gcnew array<double>(2*fftsize);
		Marshal::Copy( IntPtr( (void*) m_vector ), fftoutput, 0, 2*fftsize );
		delete[] m_vector; m_vector=nullptr; // This is un unmanaged array and must be deallocated
		return fftoutput;
	}
	array<double>^ LowLevelWrapper:: WrapNewFFT(int numaudiosamples,int fftsize, array<short>^ signal, int sign)
	{	
		pin_ptr<short> pinnedvector = &signal[0];   // pin pointer to first element in array
		short * signalunmanaged = pinnedvector;   // pointer to the first element in arr	
		double*	m_vector=new double [2*fftsize];
		for(int n=0; n<fftsize;n++)
		{
			if(n < numaudiosamples)	m_vector[2*n]=(double)signalunmanaged[n]; //we fill the pairs 2,4,6,8,10
			else m_vector[2*n]=0; //the remaining are filled with zeros 
			m_vector[2*n+1]=0; // the odds are the imaginary part and must be initialized to zero
		}
		// We invoke the function with 1 as sign (-1 returns the original signal affected by machine error)
		FFTnew(m_vector,fftsize );
		// We convert Back the fft to managed array.
		array<double>^ fftoutput = gcnew array<double>(2*fftsize);
		Marshal::Copy( IntPtr( (void*) m_vector ), fftoutput, 0, 2*fftsize );
		delete[] m_vector; m_vector=nullptr; // This is un unmanaged array and must be deallocated
		return fftoutput;
	}
	double LowLevelWrapper:: Factorial(double n)
		{
			double  ret = 1;
			for(unsigned int i = 1; i <= n; ++i)
			{
				ret *= i;
			}
			return ret;	
		}
	int LowLevelWrapper::Permutation(double population, double grouping)
	 {
		return (int) (Factorial(population)/Factorial(population-grouping)); 
	 }
	// The following algo calculates all the diphones present in a phone set: the accuracy is the number of attempts to perform to find the last diphone in the set
	array<String^> ^ LowLevelWrapper::ComputeDiphonesInPhonesSet(int accuracy)
	{	    
			 String ^ tx= File::ReadAllText(L"..\\..\\ListPhones.txt");
			 String^ delimStr = L"\r\n";
			 array<String ^> ^ ar = tx->Split(delimStr->ToCharArray() ,StringSplitOptions::RemoveEmptyEntries);	
			 // To view HashSet we must add the reference to system::Core
			 HashSet<String ^> ^ diphones = gcnew HashSet<String^>();
			 Random ^ r = gcnew Random();
			 HashSet<int> ^ temp = gcnew HashSet<int>();
			 int reiterations=0;
			 while(true)
			 {
				 int seed1 = r->Next(0, ar->Length);
				 int seed2 = r->Next(0, ar->Length);
				 temp->Clear();
				 temp->Add(seed1);
				 temp->Add(seed2);			 
				 if (temp->Count == 2)
				 {			 
				   array<int> ^ arrtemp= gcnew array<int>(2);
				   temp->CopyTo(arrtemp);			 
				   int precount = diphones->Count;
				   diphones->Add(ar[arrtemp[0]] + L" " + ar[arrtemp[1]]);
				   int postcount=diphones->Count;
				   if(postcount==precount)  reiterations++;
				   if(reiterations==accuracy) break;                          
				 }	 
			 }
		   array<String ^> ^ diphonesArray = gcnew array<String^>(diphones->Count); 
		   diphones->CopyTo(diphonesArray);
		   Array::Sort(diphonesArray);
		   return diphonesArray;
	}
	void LowLevelWrapper::Playphoneme(BYTE * paudio, int sizeaudio, int m_samplerate)
	{
		HWAVEOUT hw;	
		WAVEHDR headwav0; // this is for playing the speech
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
		waveOutUnprepareHeader(hw,&headwav0,sizeof(WAVEHDR));
		//waveOutReset(hw); it seems not good
		waveOutClose(hw);	
		HeapFree(GetProcessHeap(), 0, headwav0.lpData);	
	
	}
	void LowLevelWrapper::PlayM(array<short>^ signal, int size, int srate)
	{
		pin_ptr<short> pinnedvector = &signal[0];   // pin pointer to first element in array
		short * signalunmanaged = pinnedvector;   // pointer to the first element in arr
		Playphoneme((BYTE*)signalunmanaged,size,srate);


	}
	array<short>^ LowLevelWrapper::HammingWindow(array<short>^ signal, int size)
	{
		pin_ptr<short> pinnedvector = &signal[0];  
		short * signalunmanaged = pinnedvector;   
		for(int c =0 ; c< size; c++)
		{
			signalunmanaged[c] = (short)((double)signalunmanaged[c] * (0.53836 - 0.46164 * Math::Cos( (2*Math::PI * c ) /(size-1) ) ));			
		}
		array<short>^ signalwindowed = gcnew array<short>(size);
		Marshal::Copy( IntPtr( (void*) signalunmanaged ), signalwindowed, 0, size);
		return signalwindowed;
	}


}

