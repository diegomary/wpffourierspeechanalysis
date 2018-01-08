#include "Stdafx.h"

#define SWAP(a,b) {tempr=(a);(a)=(b);(b)=tempr;}
using namespace System;
using namespace System::Runtime::InteropServices;
using namespace System::Data;
using namespace System::Data::SqlClient;
using namespace System::IO;
using namespace System::Collections;
using namespace System::Collections::Specialized;
using namespace System::Collections::Generic;
using namespace System::Linq;

namespace SpeechUtil {

	public ref class LowLevelWrapper
	{
		public: LowLevelWrapper(){}
		private: void ComplexFFT(double *vector, unsigned int sample_rate, int sign);
		public: void LowLevelWrapper::FFTnew(double* data, unsigned long nn);
		/*public: array<double>^ WrapFFT(int numaudiosamples,array<double>^ vector);*/
	    public: array<double>^ WrapFFT(int numaudiosamples,int fftsize, array<short>^ signal, int sign);
		public:	array<double>^ LowLevelWrapper:: WrapNewFFT(int numaudiosamples,int fftsize, array<short>^ signal, int sign);
		public:double Factorial(double n);
		public:int LowLevelWrapper::Permutation(double population, double grouping);
		public: array<String^> ^ LowLevelWrapper::ComputeDiphonesInPhonesSet(int accuracy); // The accuracy is empirically estimate to the cube of the number of phones in the set so for n=42 accuracy= 74088
		private:void LowLevelWrapper:: Playphoneme(BYTE * paudio, int sizeaudio, int m_samplerate); // Sizeaudio in Bytes
	    public:void LowLevelWrapper::PlayM(array<short>^ signal, int size, int srate); // Size in BYTES
	    public:array<short>^ LowLevelWrapper::HammingWindow(array<short>^ signal, int size);
	};
}
