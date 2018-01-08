using System;using System.Collections.Generic;using System.Linq;using System.Text;using System.Threading.Tasks;using System.Windows;
using System.Windows.Controls;using System.Windows.Data;using System.Windows.Documents;using System.Windows.Input;using System.Windows.Media;
using System.Windows.Media.Imaging;using System.Windows.Navigation;using System.Windows.Shapes;using System.IO;using SpeechUtil;
using System.Collections.Specialized;using System.Data.SqlClient;

namespace Fft
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {

        public void SWAP<T>(ref T a, ref T  b)
        {
          T tempr;
            tempr=a;
            a=b;
            b=tempr;
        }

        bool mouseDown = false; // Set to 'true' when mouse is held down.
        Point mouseDownPos; // The point where the mouse button was clicked down.
        private short[] signal;
        private double[] fftresult;
        private int freqrange;
        private int sizeaudio;
        private int fftsize;
        private double coeffX;
        private double coeffY;
        private TransformGroup  groupFFT;
        private LowLevelWrapper lw;
        private bool iszoomed = false;
        private double firstvalue;
        private double secondvalue;
        private double coeffXunscaled;
     
        public MainWindow()
        {
            InitializeComponent();
            groupFFT = new TransformGroup(); // Prepare the groupFFT transform for setting coordinates system           
            lw = new LowLevelWrapper();
        }

        private void showFft_Click(object sender, RoutedEventArgs e)
        {                              
            if (fftresult != null) fftresult = null;
            ComboBoxItem cbi = (ComboBoxItem)cmbfreq.SelectedItem;
            freqrange = 0; bool res = false;
            if (cbi != null) res = int.TryParse(cbi.Content.ToString(), out freqrange); else freqrange = 9000;
            mycanvasFFT.Children.Clear();
            selectionBox.Visibility = Visibility.Collapsed;
            mycanvasFFT.Children.Add(selectionBox);
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.FileName = "Raw Audio Data"; // Default file name 
            dlg.DefaultExt = ".*"; // Default file extension 
            dlg.Filter = "All files (.*)|*.*"; // Filter files by extension
            dlg.FileName="hello.pcm";
            // Show open file dialog box 
            Nullable<bool> result = dlg.ShowDialog();
            // Process open file dialog box results 
            if (result == true)
            {
                FileStream f1 = new FileStream(dlg.FileName, FileMode.Open, FileAccess.Read);
                byte[] bt1 = new byte[(int)(f1.Length)];
                f1.Read(bt1, 0, (int)(f1.Length));
                signal = new short[((int)(f1.Length)) / 2];
                f1.Close();
                Buffer.BlockCopy(bt1, 0, signal, 0, bt1.Length);
                sizeaudio = signal.Length;
                DrawTimeDomain(signal, sizeaudio);

                fftsize = 0;
                if (sizeaudio < 32768) { fftsize = 32768; } // We want to use at least an fftsize of 32768 for small portions of data
                // if the audio sample is greater than 32768 we must choose an appropriate fftsize that covers the whole signal
                else
                {
                    int tempF = 0; int tempB = 2;
                    int p = 1;
                    while (true)
                    {
                        tempF = (int)(Math.Pow(tempB, p));
                        if (tempF > sizeaudio) break;
                        p++;
                    }
                    fftsize = tempF;
                }
             
                fftresult = lw.WrapFFT(sizeaudio, fftsize, signal, 1);
                DrawFFT(fftresult, fftsize, freqrange);      
            }
        }
        private void DrawFFT(double[] data, int fftsize, int freqrange)
        {
            double m_maxyfft = 0;
            double m_tempyfft;
            double coeff = 44100.00 / fftsize;
            int freqrange1 = (int)((1 / coeff) * (double)(freqrange));
            coeffX = mycanvasFFT.ActualWidth / (freqrange);
            for (int n = 0; n < (freqrange1); n++)
            {
                m_tempyfft = Math.Sqrt(Math.Pow(fftresult[2 * n], 2) + Math.Pow(fftresult[2 * (n + 1)], 2));
                if (m_tempyfft > m_maxyfft) m_maxyfft = m_tempyfft;             
            }
            coeffY = mycanvasFFT.ActualHeight / m_maxyfft;  
            double m_tempyfftprev, m_tempyfftpost;
            PathFigure myPathFigure = new PathFigure();
            PathSegmentCollection myPathSegmentCollection = new PathSegmentCollection();
            for (int n = 0; n < freqrange1; n++)
            {             
                m_tempyfftprev = (float)Math.Sqrt(Math.Pow(fftresult[2 * (n)], 2) + Math.Pow(fftresult[2 * (n + 1)], 2));
                n++;
                m_tempyfftpost = (float)Math.Sqrt(Math.Pow(fftresult[2 * (n)], 2) + Math.Pow(fftresult[2 * (n + 1)], 2));
                if (n == 1) myPathFigure.StartPoint = new Point((n - 1) * coeffX * coeff, coeffY * m_tempyfftprev);
                else
                {
                    LineSegment myLineSegment = new LineSegment();
                    myLineSegment.IsSmoothJoin = true;
                    myLineSegment.Point = new Point((n - 1) * coeffX * coeff, coeffY * m_tempyfftprev);
                    myPathSegmentCollection.Add(myLineSegment);

                }              
                LineSegment myLineSegment1 = new LineSegment();
                myLineSegment1.IsSmoothJoin = true;
                myLineSegment1.Point = new Point(n * coeffX * coeff, coeffY * m_tempyfftpost);
                myPathSegmentCollection.Add(myLineSegment1);      
            }          
            myPathFigure.Segments = myPathSegmentCollection;
            PathFigureCollection myPathFigureCollection = new PathFigureCollection();
            myPathFigureCollection.Add(myPathFigure);
            PathGeometry myPathGeometry = new PathGeometry();
            myPathGeometry.Figures = myPathFigureCollection;
            System.Windows.Shapes.Path myPath = new System.Windows.Shapes.Path();
            myPath.Stroke = Brushes.White;
            myPath.StrokeThickness = 1;
            myPath.Data = myPathGeometry;
            mycanvasFFT.Children.Add(myPath);        
        } 



      private void  DrawTimeDomain(short[] signal, int sizeaudio)
        {
          // We empty the canvas 
          mycanvasDT.Children.Clear();
          // We calculate the coefficient to fit the Whole data in the Canvas' extent Width.
          double coeffXDT = mycanvasDT.ActualWidth / (sizeaudio);
          // We calculate the maximum and minimum from the signal so that we can be aware of the Maximum value of the Y axis extent
          short t1 = Math.Abs(signal.Max()); short t2 = Math.Abs(signal.Min());
          // We calculate the max extent of the signal
          short maxSignal = t1 > t2 ? t1 : t2;
          // We calculate the coefficient to fit the Whole data in the Canvas' extent Height.
          double coeffYDT = (mycanvasDT.ActualHeight /2 ) / (maxSignal);  // Half of the Canvas height  
          // We create the Time domain representation which is made up by segments
          PathFigure timeDomainfigure = new PathFigure();
          PathSegmentCollection timeDomainSegmentCollection = new PathSegmentCollection();
         // We define the startPoint of the Time Domain Representation
          timeDomainfigure.StartPoint = new Point(0, signal[0] * coeffYDT );
          for (int n = 1; n < sizeaudio; n++)
          {
              // We generate all the segments
              LineSegment timeDomainLineSegment = new LineSegment();
              timeDomainLineSegment.IsSmoothJoin = true;
              timeDomainLineSegment.Point = new Point(n * coeffXDT, signal[n] * coeffYDT );
              timeDomainSegmentCollection.Add(timeDomainLineSegment);
          }
          // We add all the segments to Figure that makes up the time domain
          timeDomainfigure.Segments = timeDomainSegmentCollection;
          PathFigureCollection timeDomainPathFigureCollection = new PathFigureCollection();
          timeDomainPathFigureCollection.Add(timeDomainfigure);
          GeometryGroup timeDomainGeometryGroup = new GeometryGroup();
          PathGeometry timedomainPathGeometry = new PathGeometry();
          timedomainPathGeometry.Figures = timeDomainPathFigureCollection;
          timeDomainGeometryGroup.Children.Add(timedomainPathGeometry);
          LineGeometry lineAxisGeometry = new LineGeometry();
          lineAxisGeometry.StartPoint = new Point(0, 0);
          lineAxisGeometry.EndPoint = new Point(sizeaudio * coeffXDT, 0); 
          // We create a Path for the time domain representation
          System.Windows.Shapes.Path myPath = new System.Windows.Shapes.Path();
          myPath.Stroke = Brushes.Red;
          myPath.StrokeThickness = 1;
          myPath.Data = timeDomainGeometryGroup;
          // We create a distinct Path for drawing the X axis of time domain using a different color and line thickness
          System.Windows.Shapes.Path myPathAxis = new System.Windows.Shapes.Path();
          myPathAxis.Stroke = Brushes.White;
          myPathAxis.StrokeThickness = 0.5;
          myPathAxis.Data = lineAxisGeometry;
          //**********************************************************************************************************************
          // We create the transformation and the transformgroup suitable either for time domain Path and Axis PAth
          ScaleTransform st = new ScaleTransform(1, -1);
          TranslateTransform ts = new TranslateTransform(0, mycanvasDT.ActualHeight / 2);
          TransformGroup groupTD = new TransformGroup(); groupTD.Children.Add(st);
          groupTD.Children.Add(ts);
          // We assign the transformgroup to both Paths
          myPath.RenderTransform = groupTD;
          myPathAxis.RenderTransform = groupTD;
          //*****************************************************************************************************************************         
          // We add the two paths to the canvas
          mycanvasDT.Children.Add(myPath);
          mycanvasDT.Children.Add(myPathAxis);
      
      }

      private void DrawFFTZoom(double[] data, int fftsize, int freqstart,int freqend) // freqstart and freqend example 4500 and 5000
      {
          freqrange = freqend-freqstart ;  
          double m_maxyfft = 0;
          double m_tempyfft;
          double coeff = 44100.00 / fftsize;
          int freqrange1 = (int)((1 / coeff) * (double)(freqrange));
          int freqstart1 = (int)((1 / coeff) * (double)(freqstart));
          int freqend1 = (int)((1 / coeff) * (double)(freqend));
          coeffX = mycanvasFFT.ActualWidth / (freqrange1);
          coeffXunscaled = mycanvasFFT.ActualWidth / freqrange;
          for (int n = freqstart1; n < freqend1; n++)
          {
              m_tempyfft = Math.Sqrt(Math.Pow(fftresult[2 * n],2) + Math.Pow(fftresult[2 * (n + 1)],2));
              if (m_tempyfft > m_maxyfft) m_maxyfft = m_tempyfft;
          }
          coeffY = mycanvasFFT.ActualHeight / m_maxyfft;
          double m_tempyfftprev, m_tempyfftpost;
          PathFigure myPathFigure = new PathFigure();
          PathSegmentCollection myPathSegmentCollection = new PathSegmentCollection();
          for (int n = freqstart1,m=0; n < freqend1; n++,++m)
          {
              m_tempyfftprev = (float)Math.Sqrt(Math.Pow(fftresult[2 * (n)],2) + Math.Pow(fftresult[2 * (n + 1)],2));
              n++; m++;
              m_tempyfftpost = (float)Math.Sqrt(Math.Pow(fftresult[2 * (n)],2) + Math.Pow(fftresult[2 * (n + 1)],2));
              if (m == 1) myPathFigure.StartPoint = new Point((m - 1) * coeffX , coeffY * m_tempyfftprev);
              else
              {
                  LineSegment myLineSegment = new LineSegment();
                  myLineSegment.IsSmoothJoin = true;
                  myLineSegment.Point = new Point((m - 1) * coeffX , coeffY * m_tempyfftprev);
                  myPathSegmentCollection.Add(myLineSegment);

              }
              LineSegment myLineSegment1 = new LineSegment();
              myLineSegment1.IsSmoothJoin = true;
              myLineSegment1.Point = new Point(m * coeffX , coeffY * m_tempyfftpost);
              myPathSegmentCollection.Add(myLineSegment1);
          }
          myPathFigure.Segments = myPathSegmentCollection;
          PathFigureCollection myPathFigureCollection = new PathFigureCollection();
          myPathFigureCollection.Add(myPathFigure);
          PathGeometry myPathGeometry = new PathGeometry();
          myPathGeometry.Figures = myPathFigureCollection;
          System.Windows.Shapes.Path myPath = new System.Windows.Shapes.Path();
          myPath.Stroke = Brushes.White;
          myPath.StrokeThickness = 1;
          myPath.Data = myPathGeometry;
          mycanvasFFT.Children.Add(myPath);
          
        }


        private void cmbfreq_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (fftresult != null)
            {
                ComboBoxItem cbi = (ComboBoxItem)cmbfreq.SelectedItem;
                freqrange = 0; bool res = false;
                if (cbi != null) res = int.TryParse(cbi.Content.ToString(), out freqrange); else freqrange = 9000;
                mycanvasFFT.Children.Clear();
                selectionBox.Visibility = Visibility.Collapsed;
                mycanvasFFT.Children.Add(selectionBox);       
                DrawFFT(fftresult, fftsize, freqrange);
                iszoomed = false;
            }
        }
       

        private void mycanvasFFT_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (fftresult != null)
            {
                mycanvasFFT.Children.Clear();
                selectionBox.Visibility = Visibility.Collapsed;
                mycanvasFFT.Children.Add(selectionBox);
                mycanvasFFT.RenderTransform = null;
                groupFFT = new TransformGroup();
                ApplyTransformFFT(); 
                DrawFFT(fftresult, fftsize, freqrange);
                DrawTimeDomain(signal, sizeaudio);
            }
        }


        private void mycanvasFFT_MouseMove(object sender, MouseEventArgs e)
        {
            if (fftresult == null) return;
            Point p = e.GetPosition(mycanvasFFT);
            if (iszoomed)
            {               
                txtfreq.Text = (p.X / coeffXunscaled + firstvalue).ToString("f0") + " / " + (p.Y / coeffY).ToString("f0");
                return;            
            }           
            txtfreq.Text = (p.X / coeffX).ToString("f0") + " / " + (p.Y / coeffY).ToString("f0");           
            if (mouseDown)
            {
                // When the mouse is held down, reposition the drag selection box.
                Point mousePos = e.GetPosition(mycanvasFFT);
                if (mouseDownPos.X < mousePos.X)
                {
                    Canvas.SetLeft(selectionBox, mouseDownPos.X);
                    selectionBox.Width = mousePos.X - mouseDownPos.X;

                }
                else
                {
                    Canvas.SetLeft(selectionBox, mousePos.X);
                    selectionBox.Width = mouseDownPos.X - mousePos.X;
                }

                if (mouseDownPos.Y < mousePos.Y)
                {
                    Canvas.SetTop(selectionBox, 0);
                    selectionBox.Height = mycanvasFFT.ActualHeight;
                }
                else
                {
                    Canvas.SetTop(selectionBox, 0);
                    selectionBox.Height = mycanvasFFT.ActualHeight;
                }
            }
        }

        private void mycanvasFFT_MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (fftresult == null) return;
            if (iszoomed) return;
            mouseDown = false;
            Point mouseUpPos = e.GetPosition(mycanvasFFT);
            firstvalue = mouseDownPos.X/coeffX;
            secondvalue = mouseUpPos.X/coeffX;          
            mycanvasFFT.Children.Clear();
            selectionBox.Visibility = Visibility.Collapsed;
            mycanvasFFT.Children.Add(selectionBox);
            iszoomed = true;
            if (firstvalue > secondvalue) SWAP(ref firstvalue, ref secondvalue);  
            DrawFFTZoom(fftresult, fftsize, (int)firstvalue, (int)secondvalue);
            iszoomed = true;
        }

        private void mycanvasFFT_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (fftresult == null) return;
            if (iszoomed) return;
            // Capture and track the mouse.
            mouseDown = true;
            mouseDownPos = e.GetPosition(mycanvasFFT);
            Canvas.SetTop(selectionBox, 0);
            Canvas.SetLeft(selectionBox, mouseDownPos.X);
            selectionBox.Width = 0;
            selectionBox.Height = mycanvasFFT.ActualHeight;
            selectionBox.Visibility = Visibility.Visible;          

        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            ApplyTransformFFT();
           
        }


        private void ApplyTransformFFT()
        {
            groupFFT = new TransformGroup();
            ScaleTransform st = new ScaleTransform(1, -1);        
            groupFFT.Children.Add(st);           
            mycanvasFFT.LayoutTransform = groupFFT;     
        
        }

        private void PlayPhone(object sender, RoutedEventArgs e)
        {
           if(signal != null)  lw.PlayM(signal,2*sizeaudio, 44100);

        }

       


       

      



      


       
            

       

        
    }
}
