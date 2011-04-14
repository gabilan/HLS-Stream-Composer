using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO;

namespace HlsStreamComposer
{
    /// <summary>
    /// Interaction logic for EncodingOptionsPage.xaml
    /// </summary>
    public partial class AudioOptionsPage : Page, ISaveablePage
    {
        public AudioOptionsPage()
        {
            InitializeComponent();

            cbAudioBitRate.Items.Add(new ComboBoxItem() { Content = "192 kbps", Tag = "192k", IsSelected = true });
            cbAudioBitRate.Items.Add(new ComboBoxItem() { Content = "128 kbps", Tag = "128k" });
            cbAudioBitRate.Items.Add(new ComboBoxItem() { Content = "96 kbps", Tag = "96k" });
            cbAudioBitRate.Items.Add(new ComboBoxItem() { Content = "64 kbps", Tag = "64k" });

            cbAudioSampleRate.Items.Add(new ComboBoxItem() { Content = "48000Hz", Tag = "48000", IsSelected = true });
            cbAudioSampleRate.Items.Add(new ComboBoxItem() { Content = "44100Hz", Tag = "44100", });

            cbAudioChannels.Items.Add(new ComboBoxItem() { Content = "Stereo", Tag = "2", IsSelected = true });
            cbAudioChannels.Items.Add(new ComboBoxItem() { Content = "Mono", Tag = "1" });
        }

      

        public void Save()
        {
            int audioChannels = 0;
            int audioSamplerate = 0;

            EncodingProcess.Current.EncodingOptions.AudioBitrate = (cbAudioBitRate.SelectedItem as ComboBoxItem).Tag as string;

            if (int.TryParse((cbAudioSampleRate.SelectedItem as ComboBoxItem).Tag as string, out audioSamplerate))
                EncodingProcess.Current.EncodingOptions.AudioSampleRate = audioSamplerate;

            if (int.TryParse((cbAudioChannels.SelectedItem as ComboBoxItem).Tag as string, out audioChannels))
                EncodingProcess.Current.EncodingOptions.AudioChannels = audioChannels;
        }
    }
}
