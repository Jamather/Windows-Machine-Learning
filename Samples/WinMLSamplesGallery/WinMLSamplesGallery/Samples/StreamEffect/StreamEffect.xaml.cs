﻿using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.IO;
using WinMLSamplesGalleryNative;
using System.Runtime.InteropServices;
using System.Threading;
using Windows.UI.Core;

namespace WinMLSamplesGallery.Samples
{
    delegate IntPtr WndProc(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

    public sealed partial class StreamEffect : Page
    {
        string modelPath;
        bool isPreviewing = false;
        IntPtr currentHwnd;
        IntPtr demoHwnd;
        Task windTask;

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern IntPtr GetForegroundWindow();
        [System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
        static extern bool DestroyWindow(IntPtr hWnd);

        public StreamEffect()
        {
            this.InitializeComponent();
            // TODO: Currently the WinMLSamplesGalleryNative component will load
            // the wrong version of the Microsoft.AI.MachineLearning.dll.
            // To work around this, we make a dummy call to the builder to
            // ensure that the dll is loaded.
            var builder = Microsoft.AI.MachineLearning.Experimental.LearningModelBuilder.Create(11);
            demoHwnd = (IntPtr)WinMLSamplesGalleryNative.StreamEffect.CreateInferenceWindow();

            currentHwnd = GetForegroundWindow();
            //var modelName = "mosaic.onnx";
            modelPath = Path.Join(Windows.ApplicationModel.Package.Current.InstalledLocation.Path, "Models");
            
        }

        public void CloseInferenceWindow()
        {
            // if have a windtask running and it's not complete, destroy the window
            if (windTask != null && !windTask.IsCompleted)
            {
                DestroyWindow(demoHwnd);
            }
        }

        async private void ToggleInference(object sender, RoutedEventArgs e)
        {
            isPreviewing = !isPreviewing;
            if(isPreviewing)
            {
                ToggleInferenceBtn.Visibility = Visibility.Visible;
                ToggleInferenceBtnText.Text = "Close Streaming Demo";
                ToggleInferenceBtnIcon.Symbol = Symbol.Stop;
                var tok = new CancellationTokenSource();
                windTask = new Task(
                        () => WinMLSamplesGalleryNative.StreamEffect.LaunchNewWindow(modelPath), tok.Token);
                windTask.Start();
            }
            
            else if (!isPreviewing)
            {
                ToggleInferenceBtnText.Text = "Launch Streaming Demo";
                ToggleInferenceBtnIcon.Symbol = Symbol.NewWindow;
                ToggleInferenceBtn.Visibility = Visibility.Visible;

                CloseInferenceWindow();
                // Close this inference window and set up a new one to use in the future
                demoHwnd = (IntPtr)WinMLSamplesGalleryNative.StreamEffect.CreateInferenceWindow();

            }
        }


    }
    
}
